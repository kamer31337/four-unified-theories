#ifndef WIN32_WINDOW_3D_H
#define WIN32_WINDOW_3D_H

#ifdef _WIN32
#include <windows.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "core_math.h"
#include "symplectic_ode.h"
#include "quantum_network.h"
#include "dark_matter_halo.h"
#include "reverse_qm.h"
#include "mss_chaos_bound.h"

#define WIN_WIDTH 1024
#define WIN_HEIGHT 720

typedef enum {
    SCENE_HALO_3D = 1,
    SCENE_BLOCH_3D = 2,
    SCENE_MSS_3D = 3
} Win32SceneType;

typedef struct {
    HWND hwnd;
    HDC hdc_mem;
    HBITMAP hbm_mem;
    HBITMAP hbm_old;
    uint32_t *pixels;
    float *zbuffer;
    int width;
    int height;
    Camera3D camera;
    Win32SceneType current_scene;
    bool is_dragging;
    POINT last_mouse_pos;
    bool auto_rotate;
    double sim_time;
    HaloModel halo;
    SatelliteInfall sat;
    OrbitTrajectory orbit;
    double bloch_rx;
    double bloch_ry;
    double bloch_rz;
    double mss_beta;
    double mss_lambda;
} Win32App3D;

static Win32App3D g_app3d;

void win32_fb_clear(Win32App3D *app, uint32_t bg_color)
{
    int total_pixels = app->width * app->height;
    for (int i = 0; i < total_pixels; i++) {
        app->pixels[i] = bg_color;
        app->zbuffer[i] = 1e9f;
    }
}

void win32_draw_pixel_3d(Win32App3D *app, int x, int y, float depth, uint32_t color)
{
    if (x < 0 || x >= app->width || y < 0 || y >= app->height) return;
    int idx = y * app->width + x;
    if (depth < app->zbuffer[idx]) {
        app->zbuffer[idx] = depth;
        app->pixels[idx] = color;
    }
}

bool win32_project_point(const Win32App3D *app, const Vec3 *world_p, int *sx, int *sy, float *depth)
{
    Vec3 cam_p = transform_world_to_camera(world_p, &app->camera);
    if (cam_p.z <= 1.0) return false;
    double f = (double)app->height * 0.95 * app->camera.fov_scale;
    double px = (cam_p.x * f) / cam_p.z;
    double py = (cam_p.y * f) / cam_p.z;
    int x = (int)(app->width / 2.0 + px);
    int y = (int)(app->height / 2.0 - py);
    if (x < 0 || x >= app->width || y < 0 || y >= app->height) return false;
    *sx = x;
    *sy = y;
    *depth = (float)cam_p.z;
    return true;
}

void win32_draw_line_3d(Win32App3D *app, const Vec3 *p0, const Vec3 *p1, uint32_t color)
{
    int x0 = 0;
    int y0 = 0;
    int x1 = 0;
    int y1 = 0;
    float d0 = 0.0f;
    float d1 = 0.0f;
    bool v0 = win32_project_point(app, p0, &x0, &y0, &d0);
    bool v1 = win32_project_point(app, p1, &x1, &y1, &d1);
    if (!v0 && !v1) return;
    if (!v0 && v1) {
        x0 = x1;
        y0 = y1;
        d0 = d1;
    } else if (v0 && !v1) {
        x1 = x0;
        y1 = y0;
        d1 = d0;
    }
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int steps = dx > dy ? dx : dy;
    if (steps == 0) {
        win32_draw_pixel_3d(app, x0, y0, d0, color);
        return;
    }
    for (int i = 0; i <= steps; i++) {
        float t = (float)i / (float)steps;
        int x = (int)(x0 + t * (x1 - x0));
        int y = (int)(y0 + t * (y1 - y0));
        float d = d0 + t * (d1 - d0);
        win32_draw_pixel_3d(app, x, y, d, color);
    }
}

void win32_draw_axes(Win32App3D *app, double length)
{
    Vec3 o = {0.0, 0.0, 0.0};
    Vec3 x = {length, 0.0, 0.0};
    Vec3 y = {0.0, length, 0.0};
    Vec3 z = {0.0, 0.0, length};
    win32_draw_line_3d(app, &o, &x, 0x00FF4444);
    win32_draw_line_3d(app, &o, &y, 0x0044FF44);
    win32_draw_line_3d(app, &o, &z, 0x004488FF);
}

void win32_draw_ellipsoid_wireframe(Win32App3D *app, double a, double b, double c, int rings, uint32_t color)
{
    for (int i = 0; i < rings; i++) {
        double phi0 = 2.0 * M_PI * (double)i / (double)rings;
        double phi1 = 2.0 * M_PI * (double)(i + 1) / (double)rings;
        Vec3 p0_xy = {a * cos(phi0), b * sin(phi0), 0.0};
        Vec3 p1_xy = {a * cos(phi1), b * sin(phi1), 0.0};
        win32_draw_line_3d(app, &p0_xy, &p1_xy, color);
        Vec3 p0_xz = {a * cos(phi0), 0.0, c * sin(phi0)};
        Vec3 p1_xz = {a * cos(phi1), 0.0, c * sin(phi1)};
        win32_draw_line_3d(app, &p0_xz, &p1_xz, color);
        Vec3 p0_yz = {0.0, b * cos(phi0), c * sin(phi0)};
        Vec3 p1_yz = {0.0, b * sin(phi1), c * sin(phi1)};
        win32_draw_line_3d(app, &p0_yz, &p1_yz, color);
        for (int l = 1; l <= 3; l++) {
            double h_frac = 0.25 * l;
            double z_lvl = c * h_frac;
            double rad_lvl = sqrt(1.0 - h_frac * h_frac);
            Vec3 p0_lat_p = {a * rad_lvl * cos(phi0), b * rad_lvl * sin(phi0), z_lvl};
            Vec3 p1_lat_p = {a * rad_lvl * cos(phi1), b * rad_lvl * sin(phi1), z_lvl};
            win32_draw_line_3d(app, &p0_lat_p, &p1_lat_p, color);
            Vec3 p0_lat_n = {a * rad_lvl * cos(phi0), b * rad_lvl * sin(phi0), -z_lvl};
            Vec3 p1_lat_n = {a * rad_lvl * cos(phi1), b * rad_lvl * sin(phi1), -z_lvl};
            win32_draw_line_3d(app, &p0_lat_n, &p1_lat_n, color);
        }
    }
}

void win32_draw_halo_scene(Win32App3D *app)
{
    win32_draw_axes(app, 320.0);
    double a = app->halo.R_vir;
    double b = app->halo.R_vir * app->halo.q;
    double c = app->halo.R_vir * app->halo.s;
    win32_draw_ellipsoid_wireframe(app, a, b, c, 36, 0x00445577);
    win32_draw_ellipsoid_wireframe(app, app->halo.R_s, app->halo.R_s * app->halo.q, app->halo.R_s * app->halo.s, 24, 0x008899AA);
    if (app->orbit.count >= 2) {
        for (int i = 0; i < app->orbit.count - 1; i++) {
            Vec3 p0 = {app->orbit.points[i].x, app->orbit.points[i].y, app->orbit.points[i].z};
            Vec3 p1 = {app->orbit.points[i + 1].x, app->orbit.points[i + 1].y, app->orbit.points[i + 1].z};
            float frac = (float)i / (float)app->orbit.count;
            uint8_t red = (uint8_t)(255 * (1.0f - frac * 0.5f));
            uint8_t green = (uint8_t)(180 * (1.0f - frac * 0.3f));
            uint8_t blue = (uint8_t)(50 + 200 * frac);
            uint32_t color = (red << 16) | (green << 8) | blue;
            win32_draw_line_3d(app, &p0, &p1, color);
        }
        int cur = app->orbit.count - 1;
        Vec3 sat_head = {app->orbit.points[cur].x, app->orbit.points[cur].y, app->orbit.points[cur].z};
        int hx;
        int hy;
        float hd;
        if (win32_project_point(app, &sat_head, &hx, &hy, &hd)) {
            for (int dy = -3; dy <= 3; dy++) {
                for (int dx = -3; dx <= 3; dx++) {
                    if (dx * dx + dy * dy <= 9) {
                        win32_draw_pixel_3d(app, hx + dx, hy + dy, hd - 0.1f, 0x00FFFF00);
                    }
                }
            }
        }
    }
}

void win32_draw_bloch_scene(Win32App3D *app)
{
    win32_draw_axes(app, 1.4);
    win32_draw_ellipsoid_wireframe(app, 1.0, 1.0, 1.0, 40, 0x00336699);
    Vec3 o = {0.0, 0.0, 0.0};
    Vec3 bvec = {app->bloch_rx, app->bloch_ry, app->bloch_rz};
    win32_draw_line_3d(app, &o, &bvec, 0x0000FF88);
    int bx;
    int by;
    float bd;
    if (win32_project_point(app, &bvec, &bx, &by, &bd)) {
        for (int dy = -4; dy <= 4; dy++) {
            for (int dx = -4; dx <= 4; dx++) {
                if (dx * dx + dy * dy <= 16) {
                    win32_draw_pixel_3d(app, bx + dx, by + dy, bd - 0.1f, 0x0000FFAA);
                }
            }
        }
    }
}

void win32_draw_mss_scene(Win32App3D *app)
{
    win32_draw_axes(app, 1.4);
    win32_draw_ellipsoid_wireframe(app, 1.0, 1.0, 1.0, 36, 0x00774433);
    for (int step = 0; step < 80; step++) {
        double t0 = (double)step * 0.05;
        double t1 = (double)(step + 1) * 0.05;
        cdouble z0 = schwarz_pick_map(t0, 0.0, app->mss_beta);
        cdouble z1 = schwarz_pick_map(t1, 0.0, app->mss_beta);
        Vec3 p0 = {creal(z0), cimag(z0), 0.0};
        Vec3 p1 = {creal(z1), cimag(z1), 0.0};
        win32_draw_line_3d(app, &p0, &p1, 0x00FF8833);
        cdouble z0_edge = schwarz_pick_map(t0, app->mss_beta / 4.0, app->mss_beta);
        cdouble z1_edge = schwarz_pick_map(t1, app->mss_beta / 4.0, app->mss_beta);
        Vec3 ep0 = {creal(z0_edge), cimag(z0_edge), 0.0};
        Vec3 ep1 = {creal(z1_edge), cimag(z1_edge), 0.0};
        win32_draw_line_3d(app, &ep0, &ep1, 0x00FF3333);
    }
}

void win32_render_frame(Win32App3D *app)
{
    win32_fb_clear(app, 0x000E1118);
    if (app->current_scene == SCENE_HALO_3D) {
        win32_draw_halo_scene(app);
    } else if (app->current_scene == SCENE_BLOCH_3D) {
        win32_draw_bloch_scene(app);
    } else if (app->current_scene == SCENE_MSS_3D) {
        win32_draw_mss_scene(app);
    }
    HDC hdc = GetDC(app->hwnd);
    BitBlt(hdc, 0, 0, app->width, app->height, app->hdc_mem, 0, 0, SRCCOPY);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 255, 255));
    char buf[128];
    sprintf(buf, "[Keys: 1=Halo 3D | 2=Bloch 3D | 3=MSS Poincare 3D | R=Auto-Rotate | Drag Mouse to Rotate]");
    TextOutA(hdc, 15, 12, buf, (int)strlen(buf));
    SetTextColor(hdc, RGB(0, 220, 255));
    if (app->current_scene == SCENE_HALO_3D) {
        sprintf(buf, "SCENE 1: Triaxial DM Halo (T=%.2f, s=%.2f, q=%.2f) | Orbits=%d | R_vir=%.0f kpc", 
                app->halo.T, app->halo.s, app->halo.q, app->orbit.count, app->halo.R_vir);
        TextOutA(hdc, 15, 32, buf, (int)strlen(buf));
    } else if (app->current_scene == SCENE_BLOCH_3D) {
        double p = sqrt(app->bloch_rx * app->bloch_rx + app->bloch_ry * app->bloch_ry + app->bloch_rz * app->bloch_rz);
        sprintf(buf, "SCENE 2: Quantum Bloch Sphere (rx=%.2f, ry=%.2f, rz=%.2f) | Purity=%.3f (%s)", 
                app->bloch_rx, app->bloch_ry, app->bloch_rz, p, p >= 0.999 ? "Pure" : "Mixed");
        TextOutA(hdc, 15, 32, buf, (int)strlen(buf));
    } else {
        sprintf(buf, "SCENE 3: MSS Hyperbolic Poincare Ball (beta=%.2f, lambda_L=%.2f <= 2pi/beta=%.2f)", 
                app->mss_beta, app->mss_lambda, 2.0 * M_PI / app->mss_beta);
        TextOutA(hdc, 15, 32, buf, (int)strlen(buf));
    }
    ReleaseDC(app->hwnd, hdc);
}

LRESULT CALLBACK WndProc3D(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_CREATE:
            SetTimer(hwnd, 1, 16, NULL);
            return 0;
        case WM_TIMER:
            if (g_app3d.auto_rotate) {
                g_app3d.camera.yaw += 0.015;
            }
            if (g_app3d.current_scene == SCENE_HALO_3D && g_app3d.orbit.count < 1200) {
                int cur = g_app3d.orbit.count - 1;
                if (cur >= 0) {
                    Vec3 pos = {g_app3d.orbit.points[cur].x, g_app3d.orbit.points[cur].y, g_app3d.orbit.points[cur].z};
                    Vec3 vel = {g_app3d.orbit.points[cur].vx, g_app3d.orbit.points[cur].vy, g_app3d.orbit.points[cur].vz};
                    for (int s = 0; s < 4; s++) {
                        symplectic4_step(&pos, &vel, g_app3d.sim_time, 0.01, halo_accel, &g_app3d.halo);
                        g_app3d.sim_time += 0.01;
                    }
                    OrbitPoint pt = {g_app3d.sim_time, pos.x, pos.y, pos.z, vel.x, vel.y, vel.z, 0, 0, 0, 0};
                    orbit_append(&g_app3d.orbit, &pt);
                }
            }
            win32_render_frame(&g_app3d);
            return 0;
        case WM_LBUTTONDOWN:
            g_app3d.is_dragging = true;
            g_app3d.last_mouse_pos.x = LOWORD(lParam);
            g_app3d.last_mouse_pos.y = HIWORD(lParam);
            SetCapture(hwnd);
            return 0;
        case WM_LBUTTONUP:
            g_app3d.is_dragging = false;
            ReleaseCapture();
            return 0;
        case WM_MOUSEMOVE:
            if (g_app3d.is_dragging) {
                int mx = LOWORD(lParam);
                int my = HIWORD(lParam);
                int dx = mx - g_app3d.last_mouse_pos.x;
                int dy = my - g_app3d.last_mouse_pos.y;
                g_app3d.camera.yaw += dx * 0.008;
                g_app3d.camera.pitch += dy * 0.008;
                g_app3d.last_mouse_pos.x = mx;
                g_app3d.last_mouse_pos.y = my;
            }
            return 0;
        case WM_MOUSEWHEEL: {
            short delta = (short)HIWORD(wParam);
            if (delta > 0) {
                g_app3d.camera.dist *= 0.9;
            } else {
                g_app3d.camera.dist *= 1.1;
            }
            return 0;
        }
        case WM_KEYDOWN:
            if (wParam == '1') {
                g_app3d.current_scene = SCENE_HALO_3D;
                g_app3d.camera.dist = 620.0;
            } else if (wParam == '2') {
                g_app3d.current_scene = SCENE_BLOCH_3D;
                g_app3d.camera.dist = 3.2;
            } else if (wParam == '3') {
                g_app3d.current_scene = SCENE_MSS_3D;
                g_app3d.camera.dist = 3.2;
            } else if (wParam == 'R' || wParam == 'r') {
                g_app3d.auto_rotate = !g_app3d.auto_rotate;
            } else if (wParam == VK_ESCAPE) {
                PostQuitMessage(0);
            }
            return 0;
        case WM_DESTROY:
            KillTimer(hwnd, 1);
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

void win32_launch_window_3d(Win32SceneType initial_scene)
{
    HINSTANCE hInstance = GetModuleHandle(NULL);
    WNDCLASSEXA wc;
    memset(&wc, 0, sizeof(wc));
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc3D;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = "Unified3DWinClass";
    RegisterClassExA(&wc);

    HWND hwnd = CreateWindowExA(
        WS_EX_APPWINDOW,
        "Unified3DWinClass",
        "Unified 3D Framework : Triaxial DM Halo | Bloch Sphere | MSS Chaos Bound",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        WIN_WIDTH, WIN_HEIGHT,
        NULL, NULL, hInstance, NULL
    );

    g_app3d.hwnd = hwnd;
    g_app3d.width = WIN_WIDTH;
    g_app3d.height = WIN_HEIGHT;
    g_app3d.current_scene = initial_scene;
    g_app3d.auto_rotate = true;
    g_app3d.is_dragging = false;
    g_app3d.sim_time = 0.0;
    g_app3d.camera = camera_create(45.0, 25.0, initial_scene == SCENE_HALO_3D ? 620.0 : 3.2, 1.0);
    g_app3d.halo = halo_create_nfw(1e12, 10.0, 0.794, 0.667);
    g_app3d.sat = sample_satellite_infall(&g_app3d.halo, 0.42);
    g_app3d.orbit = orbit_create(2048);
    OrbitPoint p0 = {0, g_app3d.sat.pos_infall.x, g_app3d.sat.pos_infall.y, g_app3d.sat.pos_infall.z, 
                    g_app3d.sat.vel_infall.x, g_app3d.sat.vel_infall.y, g_app3d.sat.vel_infall.z, 0, 0, 0, 0};
    orbit_append(&g_app3d.orbit, &p0);
    g_app3d.bloch_rx = 0.6;
    g_app3d.bloch_ry = 0.0;
    g_app3d.bloch_rz = 0.8;
    g_app3d.mss_beta = 1.5;
    g_app3d.mss_lambda = 3.8;

    HDC hdc = GetDC(hwnd);
    g_app3d.hdc_mem = CreateCompatibleDC(hdc);
    BITMAPINFO bmi;
    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = g_app3d.width;
    bmi.bmiHeader.biHeight = -g_app3d.height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    g_app3d.hbm_mem = CreateDIBSection(g_app3d.hdc_mem, &bmi, DIB_RGB_COLORS, (void **)&g_app3d.pixels, NULL, 0);
    g_app3d.hbm_old = (HBITMAP)SelectObject(g_app3d.hdc_mem, g_app3d.hbm_mem);
    g_app3d.zbuffer = (float *)malloc((size_t)(g_app3d.width * g_app3d.height) * sizeof(float));
    ReleaseDC(hwnd, hdc);

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    SelectObject(g_app3d.hdc_mem, g_app3d.hbm_old);
    DeleteObject(g_app3d.hbm_mem);
    DeleteDC(g_app3d.hdc_mem);
    if (g_app3d.zbuffer) free(g_app3d.zbuffer);
    orbit_free(&g_app3d.orbit);
}

#endif
#endif
