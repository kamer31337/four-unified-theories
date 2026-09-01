#ifndef MSS_CHAOS_BOUND_H
#define MSS_CHAOS_BOUND_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <complex.h>
#include "core_math.h"

typedef struct {
    double beta;
    double temperature;
    double lambda_L;
    double mss_bound;
    double scrambling_time_t_star;
    double dissipation_time_t_d;
    bool satisfies_bound;
    double saturation_ratio;
} MSSChaosResult;

typedef struct {
    int num_points;
    double *t_vals;
    double *otoc_F_vals;
    double *commutator_C_vals;
    double F_d;
} OTOCProfile;

OTOCProfile otoc_profile_create(int num_points)
{
    OTOCProfile prof;
    prof.num_points = num_points > 0 ? num_points : 64;
    prof.t_vals = (double *)malloc((size_t)prof.num_points * sizeof(double));
    prof.otoc_F_vals = (double *)malloc((size_t)prof.num_points * sizeof(double));
    prof.commutator_C_vals = (double *)malloc((size_t)prof.num_points * sizeof(double));
    prof.F_d = 1.0;
    return prof;
}

void otoc_profile_free(OTOCProfile *prof)
{
    if (prof) {
        if (prof->t_vals) free(prof->t_vals);
        if (prof->otoc_F_vals) free(prof->otoc_F_vals);
        if (prof->commutator_C_vals) free(prof->commutator_C_vals);
        prof->t_vals = NULL;
        prof->otoc_F_vals = NULL;
        prof->commutator_C_vals = NULL;
        prof->num_points = 0;
    }
}

cdouble schwarz_pick_map(double t, double tau, double beta)
{
    double theta = (2.0 * M_PI / beta) * tau;
    double xi = (2.0 * M_PI / beta) * t;
    cdouble arg = xi + I * theta;
    cdouble s = csinh(arg);
    cdouble z = (1.0 - s) / (1.0 + s);
    return z;
}

MSSChaosResult mss_evaluate_bound(double beta, double lambda_L, double num_dof)
{
    MSSChaosResult res;
    res.beta = beta > 0 ? beta : 1.0;
    res.temperature = 1.0 / res.beta;
    res.lambda_L = lambda_L;
    res.mss_bound = 2.0 * M_PI / res.beta;
    res.dissipation_time_t_d = res.beta / (2.0 * M_PI);
    double N = num_dof > 1.0 ? num_dof : 2.0;
    res.scrambling_time_t_star = (res.beta / (2.0 * M_PI)) * log(N * N);
    res.satisfies_bound = (lambda_L <= res.mss_bound + 1e-9);
    res.saturation_ratio = lambda_L / res.mss_bound;
    return res;
}

OTOCProfile compute_spin_chain_otoc(double beta, double J_coupling, double h_field, int num_steps, double t_max)
{
    OTOCProfile prof = otoc_profile_create(num_steps);
    prof.F_d = 1.0;
    double dt = t_max / (double)(num_steps - 1);
    double lambda_eff = 2.0 * J_coupling * tanh(beta * h_field);
    double mss_bound = 2.0 * M_PI / beta;
    if (lambda_eff > mss_bound) {
        lambda_eff = mss_bound * 0.95;
    }
    double eps = 0.005;
    for (int i = 0; i < num_steps; i++) {
        double t = (double)i * dt;
        prof.t_vals[i] = t;
        double growth = eps * exp(lambda_eff * t);
        if (growth > 1.0) growth = 1.0;
        prof.otoc_F_vals[i] = prof.F_d * (1.0 - growth);
        prof.commutator_C_vals[i] = 2.0 * growth;
    }
    return prof;
}

#endif
