#ifndef REVERSE_QM_H
#define REVERSE_QM_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <complex.h>
#include "core_math.h"

typedef struct {
    double initial_entropy;
    double final_entropy;
    double hilbert_schmidt_dist_to_projector;
    double trace_norm_diff;
    bool is_spectral_equilibrium;
} LindbladEquilibrationResult;

typedef struct {
    int num_substeps;
    double unitary_fidelity;
    double total_time;
} ZenoProjectiveLimitResult;

typedef struct {
    double hbar_value;
    double quantum_moyal_bracket;
    double classical_poisson_bracket;
    double deformation_error;
} MoyalPoissonLimitResult;

typedef struct {
    CMatrix rho;
    CMatrix H_dynamic;
    CMatrix X_spectral;
    CMatrix H_thermodynamic;
    double beta;
    double entropy;
} StateEquilibriumTriad;

LindbladEquilibrationResult lindblad_equilibrate(const CMatrix *rho_initial, const CMatrix *observable_X, double dt, int max_steps)
{
    LindbladEquilibrationResult res;
    CMatrix rho = cmat_copy(rho_initial);
    res.initial_entropy = von_neumann_entropy(&rho);
    CMatrix x_sq = cmat_mul(observable_X, observable_X);
    for (int step = 0; step < max_steps; step++) {
        CMatrix x_rho = cmat_mul(observable_X, &rho);
        CMatrix x_rho_x = cmat_mul(&x_rho, observable_X);
        CMatrix x2_rho = cmat_mul(&x_sq, &rho);
        CMatrix rho_x2 = cmat_mul(&rho, &x_sq);
        CMatrix anticommutator = cmat_add(&x2_rho, &rho_x2);
        CMatrix half_anticommutator = cmat_scale(&anticommutator, 0.5 + 0.0 * I);
        CMatrix neg_half = cmat_scale(&half_anticommutator, -1.0 + 0.0 * I);
        CMatrix drho_dt = cmat_add(&x_rho_x, &neg_half);
        CMatrix step_drho = cmat_scale(&drho_dt, dt + 0.0 * I);
        CMatrix rho_next = cmat_add(&rho, &step_drho);
        cmat_free(&x_rho);
        cmat_free(&x_rho_x);
        cmat_free(&x2_rho);
        cmat_free(&rho_x2);
        cmat_free(&anticommutator);
        cmat_free(&half_anticommutator);
        cmat_free(&neg_half);
        cmat_free(&drho_dt);
        cmat_free(&step_drho);
        cmat_free(&rho);
        rho = rho_next;
    }
    res.final_entropy = von_neumann_entropy(&rho);
    CMatrix proj_rho = cmat_create(2, 2);
    cmat_set(&proj_rho, 0, 0, cmat_get(rho_initial, 0, 0));
    cmat_set(&proj_rho, 1, 1, cmat_get(rho_initial, 1, 1));
    res.hilbert_schmidt_dist_to_projector = cmat_hilbert_schmidt_dist_sq(&rho, &proj_rho);
    res.trace_norm_diff = cabs(cmat_trace(&rho) - 1.0);
    res.is_spectral_equilibrium = (res.hilbert_schmidt_dist_to_projector < 1e-4);
    cmat_free(&rho);
    cmat_free(&x_sq);
    cmat_free(&proj_rho);
    return res;
}

MoyalPoissonLimitResult test_moyal_poisson_limit(double q, double p, double hbar)
{
    MoyalPoissonLimitResult res;
    res.hbar_value = hbar;
    double df_dq = cos(q);
    double df_dp = 1.0;
    double dg_dq = -sin(q);
    double dg_dp = 2.0 * p;
    res.classical_poisson_bracket = df_dq * dg_dp - df_dp * dg_dq;
    double higher_order_term = (hbar * hbar / 24.0) * (-sin(q) * 2.0 - 0.0);
    res.quantum_moyal_bracket = res.classical_poisson_bracket + higher_order_term;
    res.deformation_error = fabs(res.quantum_moyal_bracket - res.classical_poisson_bracket);
    return res;
}

StateEquilibriumTriad state_equilibrium_triad_create(const CMatrix *rho, double beta)
{
    StateEquilibriumTriad triad;
    triad.rho = cmat_copy(rho);
    triad.beta = beta > 0 ? beta : 1.0;
    triad.entropy = von_neumann_entropy(rho);
    triad.H_dynamic = cmat_copy(rho);
    triad.X_spectral = cmat_copy(rho);
    triad.H_thermodynamic = cmat_create(2, 2);
    cdouble tr = cmat_trace(rho);
    cdouble det = cmat_get(rho, 0, 0) * cmat_get(rho, 1, 1) - cmat_get(rho, 0, 1) * cmat_get(rho, 1, 0);
    cdouble disc = csqrt(tr * tr / 4.0 - det);
    double l1 = creal(tr / 2.0 + disc);
    double l2 = creal(tr / 2.0 - disc);
    if (l1 < 1e-12) l1 = 1e-12;
    if (l2 < 1e-12) l2 = 1e-12;
    cmat_set(&triad.H_thermodynamic, 0, 0, -log(l1) / triad.beta + 0.0 * I);
    cmat_set(&triad.H_thermodynamic, 1, 1, -log(l2) / triad.beta + 0.0 * I);
    return triad;
}

void state_equilibrium_triad_free(StateEquilibriumTriad *triad)
{
    if (triad) {
        cmat_free(&triad->rho);
        cmat_free(&triad->H_dynamic);
        cmat_free(&triad->X_spectral);
        cmat_free(&triad->H_thermodynamic);
    }
}

#endif
