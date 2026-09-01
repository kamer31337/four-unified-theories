#ifndef TERMINAL_UI_H
#define TERMINAL_UI_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "core_math.h"
#include "symplectic_ode.h"
#include "mss_chaos_bound.h"

#define ANSI_RESET   "\033[0m"
#define ANSI_BOLD    "\033[1m"
#define ANSI_DIM     "\033[2m"
#define ANSI_CYAN    "\033[36m"
#define ANSI_GREEN   "\033[32m"
#define ANSI_YELLOW  "\033[33m"
#define ANSI_MAGENTA "\033[35m"
#define ANSI_BLUE    "\033[34m"
#define ANSI_RED     "\033[31m"
#define ANSI_WHITE   "\033[37m"

void ui_print_banner(void)
{
    printf("%s%s", ANSI_CYAN, ANSI_BOLD);
    printf("================================================================================\n");
    printf("  UNIFIED C11 TERMINAL : RRQM - DM HALOS - QNET CODING - MSS QUANTUM CHAOS BOUND\n");
    printf("================================================================================\n");
    printf("%s", ANSI_RESET);
    printf("%s[4 Papers Unified in Theory & Engine]%s\n", ANSI_YELLOW, ANSI_RESET);
    printf("  1. Akibue & Murao (2016): Distributed Quantum Network Coding over Cluster/Butterfly\n");
    printf("  2. Chiang, van den Bosch, Keim (2026): Scatter, Bias, & Chaos in Triaxial DM Haloes\n");
    printf("  3. Carcassi, Thrien, Aidala (2026): Reverse Quantum Mechanics & Foundational Equiv.\n");
    printf("  4. Maldacena, Shenker, Stanford (2015): Universal Bound on Chaos (lambda_L <= 2*pi*T)\n");
    printf("--------------------------------------------------------------------------------\n\n");
}

void ui_print_matrix_2x2(const char *name, const CMatrix *m)
{
    printf("%s%s%s =\n", ANSI_MAGENTA, name, ANSI_RESET);
    for (int r = 0; r < m->rows; r++) {
        printf("  [ ");
        for (int c = 0; c < m->cols; c++) {
            cdouble v = cmat_get(m, r, c);
            if (fabs(cimag(v)) < 1e-6) {
                printf("%8.4f        ", creal(v));
            } else {
                printf("%7.4f%+7.4fi ", creal(v), cimag(v));
            }
        }
        printf("]\n");
    }
    printf("\n");
}

void ui_plot_orbit_ascii(const OrbitTrajectory *tr, int width, int height)
{
    if (!tr || tr->count == 0) return;
    char grid[32][80];
    if (height > 30) height = 30;
    if (width > 78) width = 78;
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            grid[i][j] = ' ';
        }
        grid[i][width] = '\0';
    }
    double min_x = tr->points[0].x;
    double max_x = tr->points[0].x;
    double min_y = tr->points[0].y;
    double max_y = tr->points[0].y;
    for (int i = 1; i < tr->count; i++) {
        if (tr->points[i].x < min_x) min_x = tr->points[i].x;
        if (tr->points[i].x > max_x) max_x = tr->points[i].x;
        if (tr->points[i].y < min_y) min_y = tr->points[i].y;
        if (tr->points[i].y > max_y) max_y = tr->points[i].y;
    }
    double span_x = (max_x - min_x) > 1e-4 ? (max_x - min_x) : 1.0;
    double span_y = (max_y - min_y) > 1e-4 ? (max_y - min_y) : 1.0;
    for (int i = 0; i < tr->count; i++) {
        int gx = (int)((tr->points[i].x - min_x) / span_x * (width - 1));
        int gy = (int)((tr->points[i].y - min_y) / span_y * (height - 1));
        gy = height - 1 - gy;
        if (gx >= 0 && gx < width && gy >= 0 && gy < height) {
            grid[gy][gx] = '*';
        }
    }
    grid[height / 2][width / 2] = '+';
    printf("%s--- Orbit Trajectory 2D Projection (X vs Y) [kpc] ---%s\n", ANSI_CYAN, ANSI_RESET);
    printf("   Y_max = %+6.1f kpc\n", max_y);
    for (int i = 0; i < height; i++) {
        printf(" | %s |\n", grid[i]);
    }
    printf("   Y_min = %+6.1f kpc\n", min_y);
    printf("   X_min = %+6.1f kpc                     X_max = %+6.1f kpc\n\n", min_x, max_x);
}

void ui_plot_lyapunov_fit_ascii(double lambda_chaos, double A, double B)
{
    printf("%s--- Lyapunov Drift-Corrected Fit: lambda_FT(t) = lambda_chaos + (A ln t + B)/t ---%s\n", ANSI_YELLOW, ANSI_RESET);
    printf("  %sFitted lambda_chaos%s = %10.6e Gyr^-1\n", ANSI_GREEN, ANSI_RESET, lambda_chaos);
    printf("  Fitted A = %10.6e, B = %10.6e\n", A, B);
    printf("  t [Gyr]      lambda_FT (model)\n");
    double times[6] = {1.0, 5.0, 14.0, 50.0, 200.0, 1000.0};
    for (int i = 0; i < 6; i++) {
        double t = times[i];
        double lft = lambda_chaos + (A * log(t) + B) / t;
        printf("  %8.1f     %12.6e  ", t, lft);
        int bar_len = (int)(lft * 4000.0);
        if (bar_len > 40) bar_len = 40;
        if (bar_len < 0) bar_len = 0;
        for (int b = 0; b < bar_len; b++) printf("#");
        printf("\n");
    }
    printf("\n");
}

void ui_plot_otoc_ascii(const OTOCProfile *prof)
{
    if (!prof || prof->num_points == 0) return;
    printf("%s--- Out-of-Time-Order Correlator F(t)/Fd & Commutator C(t) Decay ---%s\n", ANSI_CYAN, ANSI_RESET);
    printf("  t [time]      F(t)/Fd (Scrambling Decay)      C(t) = -<[W(t),V]^2>\n");
    printf("----------------------------------------------------------------------\n");
    int stride = prof->num_points / 8;
    if (stride < 1) stride = 1;
    for (int i = 0; i < prof->num_points; i += stride) {
        double t = prof->t_vals[i];
        double f = prof->otoc_F_vals[i] / (prof->F_d > 1e-9 ? prof->F_d : 1.0);
        double c = prof->commutator_C_vals[i];
        printf("  %6.2f       %6.4f  ", t, f);
        int bar_len = (int)(f * 20.0);
        if (bar_len > 20) bar_len = 20;
        if (bar_len < 0) bar_len = 0;
        for (int b = 0; b < bar_len; b++) printf("=");
        for (int b = bar_len; b < 20; b++) printf(" ");
        printf("   %6.4f  ", c);
        int c_bar = (int)(c * 10.0);
        if (c_bar > 20) c_bar = 20;
        if (c_bar < 0) c_bar = 0;
        for (int b = 0; b < c_bar; b++) printf("#");
        printf("\n");
    }
    printf("\n");
}

void ui_print_butterfly_circuit_ascii(void)
{
    printf("%s--- Quantum Network Coding Butterfly LOCC Circuit (Akibue & Murao 2016) ---%s\n", ANSI_CYAN, ANSI_RESET);
    printf("  Wire 1 (i1 -> o1): --|H|---*------|H|--|X|-------*----------------[ X ]--> (o1)\n");
    printf("                             |                     |                   ^\n");
    printf("  Wire 2 (n1 -> n2): |0>-----|C1,3;2|--|H|--|u(x)|-|C'1,3;2|-[ M ]-(k)-+\n");
    printf("                             |                     |                   v\n");
    printf("  Wire 3 (i2 -> o2): --|H|---*------|H|--|X|-------*----------------[ X ]--> (o2)\n");
    printf("                      (i)   (ii)    (iii)   (iv)  (v)      (vi)       (vii)\n\n");
}

#endif
