#ifndef CORE_MATH_H
#define CORE_MATH_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <complex.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef double complex cdouble;

typedef struct {
    double x;
    double y;
    double z;
} Vec3;

typedef struct {
    double m[3][3];
} Mat3;

typedef struct {
    int rows;
    int cols;
    cdouble *data;
} CMatrix;

typedef struct {
    int dim;
    cdouble *data;
} CVec;

typedef struct {
    double x;
    double y;
    double z;
    int kraus_cirac_rank;
    CMatrix u;
    CMatrix u_prime;
    CMatrix w;
    CMatrix w_prime;
} KrausCiracDecomp;

typedef struct {
    int rank;
    double coeffs[4];
} SchmidtDecomp;

CMatrix cmat_create(int rows, int cols)
{
    CMatrix m;
    m.rows = rows;
    m.cols = cols;
    m.data = (cdouble *)calloc((size_t)(rows * cols), sizeof(cdouble));
    return m;
}

void cmat_free(CMatrix *m)
{
    if (m && m->data) {
        free(m->data);
        m->data = NULL;
        m->rows = 0;
        m->cols = 0;
    }
}

cdouble cmat_get(const CMatrix *m, int r, int c)
{
    return m->data[r * m->cols + c];
}

void cmat_set(CMatrix *m, int r, int c, cdouble val)
{
    m->data[r * m->cols + c] = val;
}

CMatrix cmat_identity(int dim)
{
    CMatrix m = cmat_create(dim, dim);
    for (int i = 0; i < dim; i++) {
        cmat_set(&m, i, i, 1.0 + 0.0 * I);
    }
    return m;
}

CMatrix cmat_copy(const CMatrix *src)
{
    CMatrix m = cmat_create(src->rows, src->cols);
    for (int i = 0; i < src->rows * src->cols; i++) {
        m.data[i] = src->data[i];
    }
    return m;
}

CMatrix cmat_zero(int rows, int cols)
{
    return cmat_create(rows, cols);
}

CMatrix cmat_add(const CMatrix *a, const CMatrix *b)
{
    CMatrix res = cmat_create(a->rows, a->cols);
    for (int i = 0; i < a->rows * a->cols; i++) {
        res.data[i] = a->data[i] + b->data[i];
    }
    return res;
}

CMatrix cmat_scale(const CMatrix *a, cdouble s)
{
    CMatrix res = cmat_create(a->rows, a->cols);
    for (int i = 0; i < a->rows * a->cols; i++) {
        res.data[i] = a->data[i] * s;
    }
    return res;
}

CMatrix cmat_mul(const CMatrix *a, const CMatrix *b)
{
    CMatrix res = cmat_create(a->rows, b->cols);
    for (int i = 0; i < a->rows; i++) {
        for (int j = 0; j < b->cols; j++) {
            cdouble sum = 0.0 + 0.0 * I;
            for (int k = 0; k < a->cols; k++) {
                sum += cmat_get(a, i, k) * cmat_get(b, k, j);
            }
            cmat_set(&res, i, j, sum);
        }
    }
    return res;
}

CMatrix cmat_hermitian(const CMatrix *a)
{
    CMatrix res = cmat_create(a->cols, a->rows);
    for (int i = 0; i < a->rows; i++) {
        for (int j = 0; j < a->cols; j++) {
            cmat_set(&res, j, i, conj(cmat_get(a, i, j)));
        }
    }
    return res;
}

cdouble cmat_trace(const CMatrix *a)
{
    cdouble tr = 0.0 + 0.0 * I;
    int n = (a->rows < a->cols) ? a->rows : a->cols;
    for (int i = 0; i < n; i++) {
        tr += cmat_get(a, i, i);
    }
    return tr;
}

CMatrix cmat_tensor(const CMatrix *a, const CMatrix *b)
{
    int rows = a->rows * b->rows;
    int cols = a->cols * b->cols;
    CMatrix res = cmat_create(rows, cols);
    for (int i = 0; i < a->rows; i++) {
        for (int j = 0; j < a->cols; j++) {
            cdouble aval = cmat_get(a, i, j);
            for (int k = 0; k < b->rows; k++) {
                for (int l = 0; l < b->cols; l++) {
                    int r = i * b->rows + k;
                    int c = j * b->cols + l;
                    cmat_set(&res, r, c, aval * cmat_get(b, k, l));
                }
            }
        }
    }
    return res;
}

CMatrix cmat_partial_trace_b(const CMatrix *rho, int dim_a, int dim_b)
{
    CMatrix res = cmat_create(dim_a, dim_a);
    for (int i = 0; i < dim_a; i++) {
        for (int j = 0; j < dim_a; j++) {
            cdouble sum = 0.0 + 0.0 * I;
            for (int k = 0; k < dim_b; k++) {
                int r = i * dim_b + k;
                int c = j * dim_b + k;
                sum += cmat_get(rho, r, c);
            }
            cmat_set(&res, i, j, sum);
        }
    }
    return res;
}

CMatrix cmat_partial_trace_a(const CMatrix *rho, int dim_a, int dim_b)
{
    CMatrix res = cmat_create(dim_b, dim_b);
    for (int k = 0; k < dim_b; k++) {
        for (int l = 0; l < dim_b; l++) {
            cdouble sum = 0.0 + 0.0 * I;
            for (int i = 0; i < dim_a; i++) {
                int r = i * dim_b + k;
                int c = i * dim_b + l;
                sum += cmat_get(rho, r, c);
            }
            cmat_set(&res, k, l, sum);
        }
    }
    return res;
}

double cmat_frobenius_norm(const CMatrix *a)
{
    double sum = 0.0;
    for (int i = 0; i < a->rows * a->cols; i++) {
        sum += cabs(a->data[i]) * cabs(a->data[i]);
    }
    return sqrt(sum);
}

double cmat_hilbert_schmidt_dist_sq(const CMatrix *a, const CMatrix *b)
{
    double sum = 0.0;
    for (int i = 0; i < a->rows * a->cols; i++) {
        cdouble diff = a->data[i] - b->data[i];
        sum += cabs(diff) * cabs(diff);
    }
    return sum;
}

CMatrix pauli_i(void)
{
    return cmat_identity(2);
}

CMatrix pauli_x(void)
{
    CMatrix m = cmat_create(2, 2);
    cmat_set(&m, 0, 1, 1.0 + 0.0 * I);
    cmat_set(&m, 1, 0, 1.0 + 0.0 * I);
    return m;
}

CMatrix pauli_y(void)
{
    CMatrix m = cmat_create(2, 2);
    cmat_set(&m, 0, 1, 0.0 - 1.0 * I);
    cmat_set(&m, 1, 0, 0.0 + 1.0 * I);
    return m;
}

CMatrix pauli_z(void)
{
    CMatrix m = cmat_create(2, 2);
    cmat_set(&m, 0, 0, 1.0 + 0.0 * I);
    cmat_set(&m, 1, 1, -1.0 + 0.0 * I);
    return m;
}

CMatrix hadamard_gate(void)
{
    CMatrix m = cmat_create(2, 2);
    double inv_sqrt2 = 1.0 / sqrt(2.0);
    cmat_set(&m, 0, 0, inv_sqrt2 + 0.0 * I);
    cmat_set(&m, 0, 1, inv_sqrt2 + 0.0 * I);
    cmat_set(&m, 1, 0, inv_sqrt2 + 0.0 * I);
    cmat_set(&m, 1, 1, -inv_sqrt2 + 0.0 * I);
    return m;
}

CMatrix cmat_exp_hermitian_2x2(const CMatrix *h, double t)
{
    cdouble tr = cmat_trace(h);
    cdouble det = cmat_get(h, 0, 0) * cmat_get(h, 1, 1) - cmat_get(h, 0, 1) * cmat_get(h, 1, 0);
    cdouble disc = csqrt(tr * tr / 4.0 - det);
    cdouble l1 = tr / 2.0 + disc;
    cdouble l2 = tr / 2.0 - disc;
    CMatrix res = cmat_create(2, 2);
    if (cabs(l1 - l2) < 1e-12) {
        cdouble e1 = cexp(I * t * creal(l1));
        cmat_set(&res, 0, 0, e1);
        cmat_set(&res, 1, 1, e1);
        return res;
    }
    cdouble e1 = cexp(I * t * creal(l1));
    cdouble e2 = cexp(I * t * creal(l2));
    CMatrix id = cmat_identity(2);
    for (int r = 0; r < 2; r++) {
        for (int c = 0; c < 2; c++) {
            cdouble val = ((cmat_get(h, r, c) - l2 * cmat_get(&id, r, c)) * e1 - 
                           (cmat_get(h, r, c) - l1 * cmat_get(&id, r, c)) * e2) / (l1 - l2);
            cmat_set(&res, r, c, val);
        }
    }
    cmat_free(&id);
    return res;
}

double von_neumann_entropy(const CMatrix *rho)
{
    cdouble tr = cmat_trace(rho);
    cdouble det = cmat_get(rho, 0, 0) * cmat_get(rho, 1, 1) - cmat_get(rho, 0, 1) * cmat_get(rho, 1, 0);
    cdouble disc = csqrt(tr * tr / 4.0 - det);
    double l1 = creal(tr / 2.0 + disc);
    double l2 = creal(tr / 2.0 - disc);
    double ent = 0.0;
    if (l1 > 1e-14) {
        ent -= l1 * log(l1);
    }
    if (l2 > 1e-14) {
        ent -= l2 * log(l2);
    }
    return ent;
}

#endif
