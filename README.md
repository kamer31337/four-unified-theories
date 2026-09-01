# Unified Research Engine & Interactive Terminal (C11)

**A Mathematical, Numerical, and Graphical 3D Unification of 4 Seminal Papers in Quantum Information, Galactic Dynamics, Reverse Physics, and Quantum Chaos**

---

## 1. The Four Unified Papers

1. **Akibue & Murao (2016)**: *Network coding for distributed quantum computation over cluster and butterfly networks* ([arXiv:1503.07740v2](https://arxiv.org/abs/1503.07740))
   - Canonical Kraus-Cirac decomposition $U = (u \otimes u') e^{i(x X\otimes X + y Y\otimes Y + z Z\otimes Z)} (w \otimes w')$ and Kraus-Cirac number $KC\#(U)$.
   - LOCC protocols on $(k,N)$-cluster, Butterfly, and Grail networks with Bell state resource matrices $|\Phi^+\rangle$.
   - 7-step quantum network coding protocol for arbitrary 2-qubit gates on the Butterfly network.
   - Deterministic (Theorem 2, Theorem 3) and SLOCC probabilistic (Theorem 4, Lemma 3) implementability criteria.

2. **Chiang, van den Bosch, & Keim (2026)**: *Scatter, bias, and chaos of satellite orbits in triaxial dark matter haloes* ([arXiv:2608.26249v1](https://arxiv.org/abs/2608.26249))
   - Triaxial NFW halo model $\rho(R) = \frac{\rho_0}{(R/R_s)(1+R/R_s)^2}$ with ellipsoidal radius $R = \sqrt{x^2 + y^2/q^2 + z^2/s^2}$, triaxiality $T = \frac{1-q^2}{1-s^2}$, volume-preserving rescaling $(qs)^{1/3}$, and renormalization constant $C$.
   - Symplectic 4th-order (Forest-Ruth) and 8th-order Runge-Kutta numerical orbit integration.
   - Cosmological satellite infall velocity sampler (Li et al. 2020) and energy partitioning ($R_E \le 0.9, 0.9-1.2, 1.2-1.5$).
   - Per-orbit pericentre scatter $\sigma_{\text{peri}}/\langle R_{\text{peri}}\rangle$, minimum pericentre $R_{\text{peri}}^{\text{min}}$ deepening, and box vs tube circulation classification.
   - Shadow-trajectory Benettin Lyapunov exponent extraction with drift-corrected regression:
     $$\lambda_{\text{FT}}(t) = \lambda_{\text{chaos}} + \frac{A \ln t + B}{t}$$
   - Backward orbit reconstruction of Milky Way dwarf Triangulum II (Pace et al. 2022 / McMillan 2017).

3. **Carcassi, Thrien, & Aidala (2026)**: *Reverse Quantum Mechanics* ([arXiv:2608.27543v1](https://arxiv.org/abs/2608.27543))
   - Reverse Physics methodology establishing logical equivalences between mathematical axioms and physical principles.
   - Symplectic manifolds, Liouville volume conservation, and Hamiltonian mechanics ($HM-1D \leftrightarrow DI-SYMP \leftrightarrow DR-DIV \leftrightarrow DR-INFO$).
   - Ensemble convex spaces: classical Choquet simplex vs quantum non-unique density operator decompositions.
   - Equivalence of the Born rule, Von Neumann entropy, and mutual exclusivity ($[B]\text{MUTEX} \leftrightarrow BR\text{-BORN} \leftrightarrow BR\text{-ENT} \leftrightarrow BR\text{-ORME}$).
   - Nonselective projective measurements as purely dissipative Lindblad equilibration processes (Theorem 9):
     $$\frac{d\rho}{dt} = X\rho X - \frac{1}{2}\{X^2, \rho\}$$
   - Classical mechanics as the high-entropy / deformation limit ($\hbar \to 0$, Moyal bracket $\to$ Poisson bracket) (Theorem 11).
   - Quantum states as dynamic, spectral, and thermodynamic tri-equilibria ($EE\text{-DYN}, EE\text{-MEAS}, EE\text{-THER}$) with $H = -\beta^{-1}\ln\rho$ (Theorem 12).

4. **Maldacena, Shenker, & Stanford (2015)**: *A bound on chaos* ([arXiv:1503.01409v1](https://arxiv.org/abs/1503.01409))
   - Universal upper bound on the quantum Lyapunov exponent $\lambda_L$ in thermal quantum systems with large degrees of freedom:
     $$\lambda_L \le \frac{2\pi k_B T}{\hbar} = \frac{2\pi}{\beta}$$
   - Out-of-time-order correlators (OTOC) $F(t) = \text{tr}[y V y W(t) y V y W(t)]$ and operator growth $C(t) = -\langle [W(t), V(0)]^2 \rangle_\beta$.
   - Conformal mapping of the thermal strip to the Poincaré unit disc via Schwarz-Pick hyperbolic geometry:
     $$z = \frac{1 - \sinh\left(\frac{2\pi}{\beta}(t + i\tau)\right)}{1 + \sinh\left(\frac{2\pi}{\beta}(t + i\tau)\right)}$$
   - Dissipation timescale $t_d \sim \beta$, scrambling time $t_* = \frac{\beta}{2\pi} \ln N^2$, and connection to classical collisionless chaos in the high-entropy limit ($\hbar \to 0 \implies \lambda_L \le \infty$).

---

## 2. Native WinAPI 3D Graphical Window Frame

In addition to the terminal ANSI/ASCII renderer, the application includes a **native WinAPI graphical 3D window frame** (`include/win32_window_3d.h`):
- **Double-Buffered 32-bit ARGB DIBSection**: High performance 60 FPS rendering without third-party graphics engine dependencies.
- **Hardware-smooth 3D Z-buffering**: Real-time depth testing and hidden line sorting.
- **Interactive Mouse Controls**:
  - Left click + drag: Rotate camera in 3D (Yaw / Pitch).
  - Mouse wheel: Zoom in / out.
- **Interactive Scene Switching**:
  - `Key 1`: 3D Triaxial Dark Matter Halo & Orbit path with live RK4/symplectic integration.
  - `Key 2`: 3D Quantum Bloch Sphere with state vector $\vec{r}$, lat/long circles, and purity classification.
  - `Key 3`: 3D Poincaré Unit Ball with OTOC scrambling geodesics.
  - `Key R`: Toggle automatic orbital rotation.
  - `Key ESC`: Exit window.

---

## 3. Directory Structure

```
d:\4-HO-MET\unified-rrqm-dark-matter-halo\
├── Makefile                     # C11 build rules with MinGW & GDI32/USER32/GDI+ flags
├── README.md                    # System and theoretical documentation
├── include/
│   ├── core_math.h              # Complex linear algebra, tensor products, eigensolvers, entropy
│   ├── symplectic_ode.h         # 4th-order symplectic & RK8 numerical integrators, Lyapunov fitting
│   ├── quantum_network.h        # Paper 1: Butterfly/Grail LOCC protocols, Kraus-Cirac Weyl chamber
│   ├── dark_matter_halo.h       # Paper 2: Triaxial NFW potential, satellite infall, chaos census
│   ├── reverse_qm.h             # Paper 3: Reverse QM theorems, Lindblad solver, Moyal-Poisson limit
│   ├── mss_chaos_bound.h        # Paper 4: MSS bound on chaos, OTOC simulation, Schwarz-Pick mapping
│   ├── renderer_3d.h            # Pure C11 software 3D rasterizer with z-buffer & perspective
│   ├── win32_window_3d.h        # Native WinAPI 3D Window Frame (DIBSection, 60fps, mouse drag)
│   ├── terminal_ui.h            # ANSI Terminal REPL, ASCII 2D orbit plots, OTOC graphs, Lyapunov fits
│   └── unified_engine.h         # Master unification layer bridging all 4 domains
├── src/
│   └── main.c                   # Interactive CLI prompt & command dispatcher
└── tests/
    └── test_suite.c             # Automated verification test suite across all 4 papers
```

---

## 4. Building & Running

### Build
```bash
make
```

### Launch Native WinAPI 3D Window
```powershell
# Launch WinAPI 3D window showing Triaxial Halo & Orbit
.\unified_terminal.exe --gui

# Launch WinAPI 3D window showing Quantum Bloch Sphere
.\unified_terminal.exe --gui bloch

# Launch WinAPI 3D window showing MSS Poincare Ball
.\unified_terminal.exe --gui mss
```

### Run Tests & Grand Demo
```powershell
# Run automated unit test suite (27/27 tests passing)
.\test_suite.exe

# Run Grand Tour terminal demo
.\unified_terminal.exe --demo
```
