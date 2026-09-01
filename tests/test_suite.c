#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "../include/core_math.h"
#include "../include/symplectic_ode.h"
#include "../include/quantum_network.h"
#include "../include/dark_matter_halo.h"
#include "../include/reverse_qm.h"
#include "../include/mss_chaos_bound.h"
#include "../include/renderer_3d.h"
#include "../include/terminal_ui.h"

int tests_passed = 0;
int tests_failed = 0;

void assert_test(bool condition, const char *test_name)
{
    if (condition) {
        printf("  [PASS] %s\n", test_name);
        tests_passed++;
    } else {
        printf("  [FAIL] %s\n", test_name);
        tests_failed++;
    }
}

void test_core_math(void)
{
    printf("%s--- Testing Core Math & Quantum Algebra ---%s\n", ANSI_YELLOW, ANSI_RESET);
    CMatrix id2 = cmat_identity(2);
    assert_test(fabs(creal(cmat_trace(&id2)) - 2.0) < 1e-9, "Identity trace is 2.0");
    CMatrix sx = pauli_x();
    CMatrix sy = pauli_y();
    CMatrix sz = pauli_z();
    CMatrix sx_sy = cmat_mul(&sx, &sy);
    CMatrix isz = cmat_scale(&sz, 0.0 + 1.0 * I);
    assert_test(cmat_hilbert_schmidt_dist_sq(&sx_sy, &isz) < 1e-9, "Pauli algebra: sigma_x * sigma_y = i * sigma_z");
    CMatrix rho_pure = cmat_create(2, 2);
    cmat_set(&rho_pure, 0, 0, 1.0 + 0.0 * I);
    assert_test(fabs(von_neumann_entropy(&rho_pure)) < 1e-9, "Pure state entropy is zero");
    CMatrix rho_mixed = cmat_create(2, 2);
    cmat_set(&rho_mixed, 0, 0, 0.5 + 0.0 * I);
    cmat_set(&rho_mixed, 1, 1, 0.5 + 0.0 * I);
    assert_test(fabs(von_neumann_entropy(&rho_mixed) - log(2.0)) < 1e-7, "Maximally mixed state entropy is ln(2)");
    cmat_free(&id2);
    cmat_free(&sx);
    cmat_free(&sy);
    cmat_free(&sz);
    cmat_free(&sx_sy);
    cmat_free(&isz);
    cmat_free(&rho_pure);
    cmat_free(&rho_mixed);
}

void test_quantum_network(void)
{
    printf("%s--- Testing Paper 1: Quantum Network Coding ---%s\n", ANSI_YELLOW, ANSI_RESET);
    int kc1 = calculate_kraus_cirac_number(0.3, 0.0, 0.0, 1e-6);
    int kc2 = calculate_kraus_cirac_number(0.3, 0.2, 0.0, 1e-6);
    int kc3 = calculate_kraus_cirac_number(0.3, 0.2, 0.1, 1e-6);
    assert_test(kc1 == 1, "KC# is 1 for 1 non-zero parameter");
    assert_test(kc2 == 2, "KC# is 2 for 2 non-zero parameters");
    assert_test(kc3 == 3, "KC# is 3 for 3 non-zero parameters");
    assert_test(check_ladder_implementability(0.3, 0.0, 0.0, 1) == true, "Theorem 3: KC#=1 implementable on 1-bridge ladder");
    assert_test(check_ladder_implementability(0.3, 0.2, 0.0, 1) == false, "Theorem 3: KC#=2 NOT implementable on 1-bridge ladder");
    assert_test(check_ladder_implementability(0.3, 0.2, 0.0, 2) == true, "Theorem 3: KC#=2 implementable on 2-bridge ladder");
    assert_test(check_ladder_implementability(0.3, 0.2, 0.1, 2) == false, "Lemma 3: SWAP/KC#=3 NOT implementable on 2-bridge ladder");
    CMatrix in_rho = cmat_create(4, 4);
    cmat_set(&in_rho, 0, 0, 1.0 + 0.0 * I);
    ButterflyLOCCResult locc = run_butterfly_protocol(0.35, 0.20, 0.15, &in_rho, 0);
    assert_test(locc.deterministic_success == true, "Theorem 1: Arbitrary 2-qubit unitary implementable over Butterfly network");
    cmat_free(&in_rho);
    cmat_free(&locc.output_state);
}

void test_dark_matter_halo(void)
{
    printf("%s--- Testing Paper 2: Triaxial Dark Matter Halo Dynamics ---%s\n", ANSI_YELLOW, ANSI_RESET);
    HaloModel h_sph = halo_create_nfw(1e12, 10.0, 1.0, 1.0);
    assert_test(h_sph.is_triaxial == false, "Spherical halo created correctly");
    HaloModel h_tri = halo_create_nfw(1e12, 10.0, 0.8, 0.6);
    assert_test(h_tri.is_triaxial == true, "Triaxial halo created correctly");
    assert_test(h_tri.T > 0.5 && h_tri.T < 0.8, "Triaxiality T computed correctly (T ~ 0.667)");
    Vec3 pos = {100.0, 0.0, 0.0};
    Vec3 accel;
    halo_accel(0.0, &pos, &accel, &h_sph);
    assert_test(accel.x < 0.0 && fabs(accel.y) < 1e-9 && fabs(accel.z) < 1e-9, "Radial inward acceleration on x-axis");
}

void test_reverse_qm(void)
{
    printf("%s--- Testing Paper 3: Reverse Quantum Mechanics ---%s\n", ANSI_YELLOW, ANSI_RESET);
    CMatrix rho_plus = cmat_create(2, 2);
    cmat_set(&rho_plus, 0, 0, 0.5 + 0.0 * I);
    cmat_set(&rho_plus, 0, 1, 0.5 + 0.0 * I);
    cmat_set(&rho_plus, 1, 0, 0.5 + 0.0 * I);
    cmat_set(&rho_plus, 1, 1, 0.5 + 0.0 * I);
    CMatrix sz = pauli_z();
    LindbladEquilibrationResult eq = lindblad_equilibrate(&rho_plus, &sz, 0.02, 300);
    assert_test(eq.is_spectral_equilibrium == true, "Theorem 9: Nonselective measurement = Lindblad spectral equilibration");
    assert_test(eq.final_entropy > eq.initial_entropy, "Theorem 9: Entropy increases during measurement decoherence");
    MoyalPoissonLimitResult mp = test_moyal_poisson_limit(1.2, 0.8, 1e-5);
    assert_test(mp.deformation_error < 1e-8, "Theorem 11: Moyal bracket -> Poisson bracket as hbar -> 0");
    StateEquilibriumTriad triad = state_equilibrium_triad_create(&rho_plus, 1.0);
    assert_test(triad.H_dynamic.rows == 2 && triad.X_spectral.rows == 2 && triad.H_thermodynamic.rows == 2, "Theorem 12: Quantum state tri-equilibria constructed");
    cmat_free(&rho_plus);
    cmat_free(&sz);
    state_equilibrium_triad_free(&triad);
}

void test_mss_chaos_bound(void)
{
    printf("%s--- Testing Paper 4: Maldacena-Shenker-Stanford Bound on Chaos ---%s\n", ANSI_YELLOW, ANSI_RESET);
    MSSChaosResult res_valid = mss_evaluate_bound(2.0, 3.0, 16.0);
    assert_test(res_valid.satisfies_bound == true, "MSS Bound: lambda_L = 3.0 <= 2*pi/2.0 ~ 3.14159 satisfies bound");
    MSSChaosResult res_viol = mss_evaluate_bound(2.0, 3.5, 16.0);
    assert_test(res_viol.satisfies_bound == false, "MSS Bound: lambda_L = 3.5 > 2*pi/2.0 violates bound");
    cdouble z = schwarz_pick_map(2.0, 0.2, 2.0);
    assert_test(cabs(z) <= 1.0 + 1e-7, "Schwarz-Pick mapping sends thermal strip to Poincare unit disc");
    OTOCProfile prof = compute_spin_chain_otoc(1.5, 1.0, 0.5, 16, 4.0);
    assert_test(prof.num_points == 16, "OTOC profile computed with 16 points");
    assert_test(prof.otoc_F_vals[15] < prof.otoc_F_vals[0], "OTOC F(t) decays monotonically with time due to scrambling");
    otoc_profile_free(&prof);
}

void test_renderer_3d(void)
{
    printf("%s--- Testing 3D Software Rasterizer & Perspective Engine ---%s\n", ANSI_YELLOW, ANSI_RESET);
    FrameBuffer3D fb = fb_create(40, 20);
    assert_test(fb.width == 40 && fb.height == 20, "FrameBuffer3D initialized with dimensions 40x20");
    Camera3D cam = camera_create(45.0, 30.0, 100.0, 1.0);
    Vec3 p0 = {0.0, 0.0, 0.0};
    Vec3 p1 = {10.0, 10.0, 10.0};
    fb_draw_line_3d(&fb, &p0, &p1, &cam, '*', 32);
    int non_empty = 0;
    for (int y = 0; y < fb.height; y++) {
        for (int x = 0; x < fb.width; x++) {
            if (fb.chars[y][x] != ' ') non_empty++;
        }
    }
    assert_test(non_empty > 0, "3D line rasterized into framebuffer");
}

int main(void)
{
    ui_print_banner();
    printf("%s=== RUNNING COMPREHENSIVE VERIFICATION TEST SUITE (4 PAPERS + 3D) ===%s\n\n", ANSI_CYAN, ANSI_RESET);
    test_core_math();
    test_quantum_network();
    test_dark_matter_halo();
    test_reverse_qm();
    test_mss_chaos_bound();
    test_renderer_3d();
    printf("\n%s================================================================================%s\n", ANSI_CYAN, ANSI_RESET);
    printf("Test Summary: %s%d Passed%s, %s%d Failed%s\n", ANSI_GREEN, tests_passed, ANSI_RESET, tests_failed > 0 ? ANSI_RED : ANSI_GREEN, tests_failed, ANSI_RESET);
    printf("%s================================================================================%s\n\n", ANSI_CYAN, ANSI_RESET);
    return tests_failed > 0 ? 1 : 0;
}
