#ifndef DARK_MATTER_HALO_H
#define DARK_MATTER_HALO_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "core_math.h"
#include "symplectic_ode.h"

typedef struct {
    double M_vir;
    double R_vir;
    double c_vir;
    double R_s;
    double rho_0;
    double V_vir;
    double q;
    double s;
    double T;
    double C_renorm;
    double R_s_eff;
    bool is_triaxial;
    Mat3 rotation;
} HaloModel;

typedef struct {
    HaloModel halo;
    double M_bulge;
    double r_bulge;
    double Sigma_0_thin;
    double R_d_thin;
    double z_d_thin;
    double Sigma_0_thick;
    double R_d_thick;
    double z_d_thick;
} MWModel;

typedef struct {
    double u_infall;
    double cos2_theta;
    double R_E;
    int energy_bin;
    Vec3 pos_infall;
    Vec3 vel_infall;
} SatelliteInfall;

typedef struct {
    int total_orbits;
    double median_sigma_peri_pct;
    double mean_r_peri_min;
    double fraction_chaotic;
    double median_t_chaos_gyr;
    double median_r_peri_kpc;
    double median_r_apo_kpc;
    double median_T_r_gyr;
} PopulationStats;

HaloModel halo_create_nfw(double M_vir_msun, double c_vir, double q, double s)
{
    HaloModel h;
    h.M_vir = M_vir_msun > 0 ? M_vir_msun : 1.0e12;
    h.c_vir = c_vir > 0 ? c_vir : 10.0;
    h.R_vir = 262.0;
    h.R_s = h.R_vir / h.c_vir;
    h.V_vir = 128.0;
    h.q = q;
    h.s = s;
    if (fabs(1.0 - s * s) > 1e-6) {
        h.T = (1.0 - q * q) / (1.0 - s * s);
    } else {
        h.T = 0.0;
    }
    h.is_triaxial = (fabs(q - 1.0) > 1e-4 || fabs(s - 1.0) > 1e-4);
    double shape_factor = pow(q * s, 1.0 / 3.0);
    h.R_s_eff = (h.is_triaxial && shape_factor > 0.01) ? (h.R_s / shape_factor) : h.R_s;
    h.C_renorm = 1.0 + 0.09 * (1.0 - s);
    h.rho_0 = h.M_vir / (4.0 * M_PI * pow(h.R_s, 3) * (log(1.0 + h.c_vir) - h.c_vir / (1.0 + h.c_vir)));
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            h.rotation.m[i][j] = (i == j) ? 1.0 : 0.0;
        }
    }
    return h;
}

void halo_accel(double t, const Vec3 *pos, Vec3 *accel, void *user_data)
{
    (void)t;
    HaloModel *h = (HaloModel *)user_data;
    double G_const = 4.30091e-6;
    double x = pos->x;
    double y = pos->y;
    double z = pos->z;
    if (!h->is_triaxial) {
        double r = sqrt(x * x + y * y + z * z);
        if (r < 1e-4) {
            accel->x = 0.0;
            accel->y = 0.0;
            accel->z = 0.0;
            return;
        }
        double u = r / h->R_s;
        double m_enc = 4.0 * M_PI * h->rho_0 * pow(h->R_s, 3) * (log(1.0 + u) - u / (1.0 + u));
        double f = -G_const * m_enc / (r * r * r);
        accel->x = f * x;
        accel->y = f * y;
        accel->z = f * z;
    } else {
        double q = h->q;
        double s = h->s;
        double R = sqrt(x * x + (y * y) / (q * q) + (z * z) / (s * s));
        if (R < 1e-4) {
            accel->x = 0.0;
            accel->y = 0.0;
            accel->z = 0.0;
            return;
        }
        double u = R / h->R_s_eff;
        double m_enc = 4.0 * M_PI * h->rho_0 * h->C_renorm * pow(h->R_s_eff, 3) * q * s * (log(1.0 + u) - u / (1.0 + u));
        double f = -G_const * m_enc / (R * R * R);
        accel->x = f * x;
        accel->y = f * (y / (q * q));
        accel->z = f * (z / (s * s));
    }
}

double halo_potential(const Vec3 *pos, void *user_data)
{
    HaloModel *h = (HaloModel *)user_data;
    double G_const = 4.30091e-6;
    double x = pos->x;
    double y = pos->y;
    double z = pos->z;
    if (!h->is_triaxial) {
        double r = sqrt(x * x + y * y + z * z);
        if (r < 1e-4) r = 1e-4;
        double u = r / h->R_s;
        return -4.0 * M_PI * G_const * h->rho_0 * pow(h->R_s, 2) * log(1.0 + u) / u;
    } else {
        double q = h->q;
        double s = h->s;
        double R = sqrt(x * x + (y * y) / (q * q) + (z * z) / (s * s));
        if (R < 1e-4) R = 1e-4;
        double u = R / h->R_s_eff;
        return -4.0 * M_PI * G_const * h->rho_0 * h->C_renorm * pow(h->R_s_eff, 2) * q * s * log(1.0 + u) / u;
    }
}

SatelliteInfall sample_satellite_infall(const HaloModel *halo, double seed_val)
{
    SatelliteInfall sat;
    double theta = (fmod(seed_val * 1.618033, 1.0)) * M_PI;
    double phi = (fmod(seed_val * 2.718281, 1.0)) * 2.0 * M_PI;
    sat.pos_infall.x = halo->R_vir * sin(theta) * cos(phi);
    sat.pos_infall.y = halo->R_vir * sin(theta) * sin(phi);
    sat.pos_infall.z = halo->R_vir * cos(theta);
    double u_rnd = 0.6 + 0.8 * fmod(seed_val * 3.14159, 1.0);
    sat.u_infall = u_rnd;
    sat.cos2_theta = 0.2 + 0.6 * fmod(seed_val * 0.707106, 1.0);
    double v_mag = u_rnd * halo->V_vir;
    double vr = -v_mag * sqrt(sat.cos2_theta);
    double vt = v_mag * sqrt(1.0 - sat.cos2_theta);
    Vec3 r_unit = {sat.pos_infall.x / halo->R_vir, sat.pos_infall.y / halo->R_vir, sat.pos_infall.z / halo->R_vir};
    Vec3 t_unit = {-sin(phi), cos(phi), 0.0};
    sat.vel_infall.x = vr * r_unit.x + vt * t_unit.x;
    sat.vel_infall.y = vr * r_unit.y + vt * t_unit.y;
    sat.vel_infall.z = vr * r_unit.z + vt * t_unit.z;
    sat.R_E = 0.8 + 0.5 * (u_rnd - 0.6);
    if (sat.R_E <= 0.9) {
        sat.energy_bin = 0;
    } else if (sat.R_E <= 1.2) {
        sat.energy_bin = 1;
    } else {
        sat.energy_bin = 2;
    }
    return sat;
}

#endif
