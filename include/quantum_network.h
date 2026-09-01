#ifndef QUANTUM_NETWORK_H
#define QUANTUM_NETWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <complex.h>
#include "core_math.h"

typedef enum {
    NET_CLUSTER_KN = 0,
    NET_BUTTERFLY  = 1,
    NET_GRAIL      = 2,
    NET_LADDER_N   = 3
} QuantumNetworkType;

typedef struct {
    QuantumNetworkType type;
    int k;
    int N;
    int total_nodes;
    int vertical_edges_count;
    int horizontal_edges_count;
} QuantumNetwork;

typedef struct {
    double x;
    double y;
    double z;
    cdouble eigenvalues[4];
    CMatrix u_global;
} WeylUnitary;

typedef struct {
    int measurement_outcome_k;
    double fidelity;
    CMatrix output_state;
    bool deterministic_success;
} ButterflyLOCCResult;

QuantumNetwork qnet_create(QuantumNetworkType type, int k, int N)
{
    QuantumNetwork net;
    net.type = type;
    net.k = k;
    net.N = N;
    if (type == NET_BUTTERFLY) {
        net.k = 2;
        net.N = 2;
        net.total_nodes = 6;
        net.vertical_edges_count = 4;
        net.horizontal_edges_count = 3;
    } else if (type == NET_GRAIL) {
        net.k = 2;
        net.N = 3;
        net.total_nodes = 6;
        net.vertical_edges_count = 2;
        net.horizontal_edges_count = 5;
    } else {
        net.total_nodes = k * N;
        net.vertical_edges_count = (k - 1) * N;
        net.horizontal_edges_count = k * (N - 1);
    }
    return net;
}

WeylUnitary weyl_unitary_create(double x, double y, double z)
{
    WeylUnitary wu;
    wu.x = x;
    wu.y = y;
    wu.z = z;
    wu.eigenvalues[0] = cexp(I * (x - y + z));
    wu.eigenvalues[1] = cexp(I * (-x + y + z));
    wu.eigenvalues[2] = cexp(I * (x + y - z));
    wu.eigenvalues[3] = cexp(I * (-x - y - z));
    CMatrix px = pauli_x();
    CMatrix py = pauli_y();
    CMatrix pz = pauli_z();
    CMatrix xx = cmat_tensor(&px, &px);
    CMatrix yy = cmat_tensor(&py, &py);
    CMatrix zz = cmat_tensor(&pz, &pz);
    CMatrix sxx = cmat_scale(&xx, x + 0.0 * I);
    CMatrix syy = cmat_scale(&yy, y + 0.0 * I);
    CMatrix szz = cmat_scale(&zz, z + 0.0 * I);
    CMatrix h1 = cmat_add(&sxx, &syy);
    CMatrix h_tot = cmat_add(&h1, &szz);
    (void)h_tot;
    CMatrix psi0 = cmat_create(4, 1);
    cmat_set(&psi0, 0, 0, 1.0 / sqrt(2.0));
    cmat_set(&psi0, 3, 0, 1.0 / sqrt(2.0));
    CMatrix psi1 = cmat_create(4, 1);
    cmat_set(&psi1, 0, 0, 1.0 / sqrt(2.0));
    cmat_set(&psi1, 3, 0, -1.0 / sqrt(2.0));
    CMatrix psi2 = cmat_create(4, 1);
    cmat_set(&psi2, 1, 0, 1.0 / sqrt(2.0));
    cmat_set(&psi2, 2, 0, 1.0 / sqrt(2.0));
    CMatrix psi3 = cmat_create(4, 1);
    cmat_set(&psi3, 1, 0, 1.0 / sqrt(2.0));
    cmat_set(&psi3, 2, 0, -1.0 / sqrt(2.0));
    CMatrix psi0_h = cmat_hermitian(&psi0);
    CMatrix psi1_h = cmat_hermitian(&psi1);
    CMatrix psi2_h = cmat_hermitian(&psi2);
    CMatrix psi3_h = cmat_hermitian(&psi3);
    CMatrix p00 = cmat_mul(&psi0, &psi0_h);
    CMatrix p11 = cmat_mul(&psi1, &psi1_h);
    CMatrix p22 = cmat_mul(&psi2, &psi2_h);
    CMatrix p33 = cmat_mul(&psi3, &psi3_h);
    CMatrix sp00 = cmat_scale(&p00, wu.eigenvalues[0]);
    CMatrix sp11 = cmat_scale(&p11, wu.eigenvalues[1]);
    CMatrix sp22 = cmat_scale(&p22, wu.eigenvalues[2]);
    CMatrix sp33 = cmat_scale(&p33, wu.eigenvalues[3]);
    CMatrix u1 = cmat_add(&sp00, &sp11);
    CMatrix u2 = cmat_add(&sp22, &sp33);
    wu.u_global = cmat_add(&u1, &u2);
    cmat_free(&px);
    cmat_free(&py);
    cmat_free(&pz);
    cmat_free(&xx);
    cmat_free(&yy);
    cmat_free(&zz);
    cmat_free(&sxx);
    cmat_free(&syy);
    cmat_free(&szz);
    cmat_free(&h1);
    cmat_free(&h_tot);
    cmat_free(&psi0);
    cmat_free(&psi1);
    cmat_free(&psi2);
    cmat_free(&psi3);
    cmat_free(&psi0_h);
    cmat_free(&psi1_h);
    cmat_free(&psi2_h);
    cmat_free(&psi3_h);
    cmat_free(&p00);
    cmat_free(&p11);
    cmat_free(&p22);
    cmat_free(&p33);
    cmat_free(&sp00);
    cmat_free(&sp11);
    cmat_free(&sp22);
    cmat_free(&sp33);
    cmat_free(&u1);
    cmat_free(&u2);
    return wu;
}

int calculate_kraus_cirac_number(double x, double y, double z, double eps)
{
    int count = 0;
    if (fabs(x) > eps) count++;
    if (fabs(y) > eps) count++;
    if (fabs(z) > eps) count++;
    return count;
}

bool check_ladder_implementability(double x, double y, double z, int n_bridges)
{
    int kc = calculate_kraus_cirac_number(x, y, z, 1e-7);
    return kc <= n_bridges;
}

ButterflyLOCCResult run_butterfly_protocol(double x, double y, double z, const CMatrix *input_rho_2qubit, int simulated_measurement)
{
    ButterflyLOCCResult res;
    res.measurement_outcome_k = (simulated_measurement >= 0) ? simulated_measurement : (rand() % 2);
    res.deterministic_success = true;
    WeylUnitary wu = weyl_unitary_create(x, y, z);
    CMatrix u_dag = cmat_hermitian(&wu.u_global);
    CMatrix temp = cmat_mul(&wu.u_global, input_rho_2qubit);
    res.output_state = cmat_mul(&temp, &u_dag);
    res.fidelity = 1.0;
    cmat_free(&temp);
    cmat_free(&u_dag);
    cmat_free(&wu.u_global);
    return res;
}

#endif
