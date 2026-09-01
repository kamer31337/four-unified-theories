#ifndef SYMPLECTIC_ODE_H
#define SYMPLECTIC_ODE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "core_math.h"

typedef struct {
    double t;
    double x;
    double y;
    double z;
    double vx;
    double vy;
    double vz;
    double r;
    double vr;
    double E;
    double L_mag;
} OrbitPoint;

typedef struct {
    int count;
    int capacity;
    OrbitPoint *points;
} OrbitTrajectory;

typedef struct {
    double r_peri_first;
    double r_apo_first;
    double T_r;
    double r_peri_last;
    double r_apo_last;
    double r_peri_min;
    double sigma_peri_rel;
    double L_z_mean;
    bool is_box;
    bool is_tube;
    int pericentre_count;
    int apocentre_count;
    double pericentres[64];
    double apocentres[64];
} OrbitStats;

typedef struct {
    double lambda_chaos;
    double coeff_A;
    double coeff_B;
    double r_squared;
    bool is_chaotic;
    double t_chaos_gyr;
} LyapunovResult;

typedef void (*AccelFunc)(double t, const Vec3 *pos, Vec3 *accel, void *user_data);

typedef double (*PotentialFunc)(const Vec3 *pos, void *user_data);

OrbitTrajectory orbit_create(int initial_capacity)
{
    OrbitTrajectory tr;
    tr.count = 0;
    tr.capacity = initial_capacity > 0 ? initial_capacity : 1024;
    tr.points = (OrbitPoint *)malloc((size_t)tr.capacity * sizeof(OrbitPoint));
    return tr;
}

void orbit_free(OrbitTrajectory *tr)
{
    if (tr && tr->points) {
        free(tr->points);
        tr->points = NULL;
        tr->count = 0;
        tr->capacity = 0;
    }
}

void orbit_append(OrbitTrajectory *tr, const OrbitPoint *pt)
{
    if (tr->count >= tr->capacity) {
        tr->capacity *= 2;
        tr->points = (OrbitPoint *)realloc(tr->points, (size_t)tr->capacity * sizeof(OrbitPoint));
    }
    tr->points[tr->count++] = *pt;
}

void symplectic4_step(Vec3 *pos, Vec3 *vel, double t, double dt, AccelFunc accel_func, void *user_data)
{
    static const double cr3 = 1.2599210498948731647672106;
    double c1 = 1.0 / (2.0 * (2.0 - cr3));
    double c2 = (1.0 - cr3) / (2.0 * (2.0 - cr3));
    double c3 = c2;
    double c4 = c1;
    double d1 = 1.0 / (2.0 - cr3);
    double d2 = -cr3 / (2.0 - cr3);
    double d3 = d1;
    Vec3 a;
    pos->x += c1 * vel->x * dt;
    pos->y += c1 * vel->y * dt;
    pos->z += c1 * vel->z * dt;
    accel_func(t + c1 * dt, pos, &a, user_data);
    vel->x += d1 * a.x * dt;
    vel->y += d1 * a.y * dt;
    vel->z += d1 * a.z * dt;
    pos->x += c2 * vel->x * dt;
    pos->y += c2 * vel->y * dt;
    pos->z += c2 * vel->z * dt;
    accel_func(t + (c1 + c2) * dt, pos, &a, user_data);
    vel->x += d2 * a.x * dt;
    vel->y += d2 * a.y * dt;
    vel->z += d2 * a.z * dt;
    pos->x += c3 * vel->x * dt;
    pos->y += c3 * vel->y * dt;
    pos->z += c3 * vel->z * dt;
    accel_func(t + (c1 + c2 + c3) * dt, pos, &a, user_data);
    vel->x += d3 * a.x * dt;
    vel->y += d3 * a.y * dt;
    vel->z += d3 * a.z * dt;
    pos->x += c4 * vel->x * dt;
    pos->y += c4 * vel->y * dt;
    pos->z += c4 * vel->z * dt;
}

void rk8_step(Vec3 *pos, Vec3 *vel, double t, double dt, AccelFunc accel_func, void *user_data)
{
    Vec3 k1_x = *vel;
    Vec3 k1_v;
    accel_func(t, pos, &k1_v, user_data);
    Vec3 p2 = {pos->x + 0.5 * dt * k1_x.x, pos->y + 0.5 * dt * k1_x.y, pos->z + 0.5 * dt * k1_x.z};
    Vec3 v2 = {vel->x + 0.5 * dt * k1_v.x, vel->y + 0.5 * dt * k1_v.y, vel->z + 0.5 * dt * k1_v.z};
    Vec3 k2_x = v2;
    Vec3 k2_v;
    accel_func(t + 0.5 * dt, &p2, &k2_v, user_data);
    Vec3 p3 = {pos->x + 0.5 * dt * k2_x.x, pos->y + 0.5 * dt * k2_x.y, pos->z + 0.5 * dt * k2_x.z};
    Vec3 v3 = {vel->x + 0.5 * dt * k2_v.x, vel->y + 0.5 * dt * k2_v.y, vel->z + 0.5 * dt * k2_v.z};
    Vec3 k3_x = v3;
    Vec3 k3_v;
    accel_func(t + 0.5 * dt, &p3, &k3_v, user_data);
    Vec3 p4 = {pos->x + dt * k3_x.x, pos->y + dt * k3_x.y, pos->z + dt * k3_x.z};
    Vec3 v4 = {vel->x + dt * k3_v.x, vel->y + dt * k3_v.y, vel->z + dt * k3_v.z};
    Vec3 k4_x = v4;
    Vec3 k4_v;
    accel_func(t + dt, &p4, &k4_v, user_data);
    pos->x += (dt / 6.0) * (k1_x.x + 2.0 * k2_x.x + 2.0 * k3_x.x + k4_x.x);
    pos->y += (dt / 6.0) * (k1_x.y + 2.0 * k2_x.y + 2.0 * k3_x.y + k4_x.y);
    pos->z += (dt / 6.0) * (k1_x.z + 2.0 * k2_x.z + 2.0 * k3_x.z + k4_x.z);
    vel->x += (dt / 6.0) * (k1_v.x + 2.0 * k2_v.x + 2.0 * k3_v.x + k4_v.x);
    vel->y += (dt / 6.0) * (k1_v.y + 2.0 * k2_v.y + 2.0 * k3_v.y + k4_v.y);
    vel->z += (dt / 6.0) * (k1_v.z + 2.0 * k2_v.z + 2.0 * k3_v.z + k4_v.z);
}

#endif
