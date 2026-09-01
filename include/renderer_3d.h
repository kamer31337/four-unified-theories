#ifndef RENDERER_3D_H
#define RENDERER_3D_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include "core_math.h"
#include "symplectic_ode.h"

#define FB_MAX_WIDTH 120
#define FB_MAX_HEIGHT 45

typedef struct {
    int width;
    int height;
    char chars[FB_MAX_HEIGHT][FB_MAX_WIDTH];
    double zbuf[FB_MAX_HEIGHT][FB_MAX_WIDTH];
    int colors[FB_MAX_HEIGHT][FB_MAX_WIDTH];
} FrameBuffer3D;

typedef struct {
    double yaw;
    double pitch;
    double roll;
    double dist;
    double fov_scale;
    Vec3 target;
} Camera3D;

FrameBuffer3D fb_create(int width, int height)
{
    FrameBuffer3D fb;
    fb.width = width <= FB_MAX_WIDTH ? width : FB_MAX_WIDTH;
    fb.height = height <= FB_MAX_HEIGHT ? height : FB_MAX_HEIGHT;
    for (int y = 0; y < fb.height; y++) {
        for (int x = 0; x < fb.width; x++) {
            fb.chars[y][x] = ' ';
            fb.zbuf[y][x] = 1e9;
            fb.colors[y][x] = 0;
        }
    }
    return fb;
}

void fb_clear(FrameBuffer3D *fb)
{
    for (int y = 0; y < fb->height; y++) {
        for (int x = 0; x < fb->width; x++) {
            fb->chars[y][x] = ' ';
            fb->zbuf[y][x] = 1e9;
            fb->colors[y][x] = 0;
        }
    }
}

Camera3D camera_create(double yaw_deg, double pitch_deg, double dist, double fov_scale)
{
    Camera3D cam;
    cam.yaw = yaw_deg * M_PI / 180.0;
    cam.pitch = pitch_deg * M_PI / 180.0;
    cam.roll = 0.0;
    cam.dist = dist > 0.1 ? dist : 500.0;
    cam.fov_scale = fov_scale > 0.1 ? fov_scale : 1.0;
    cam.target.x = 0.0;
    cam.target.y = 0.0;
    cam.target.z = 0.0;
    return cam;
}

Vec3 transform_world_to_camera(const Vec3 *p, const Camera3D *cam)
{
    double tx = p->x - cam->target.x;
    double ty = p->y - cam->target.y;
    double tz = p->z - cam->target.z;
    double cos_y = cos(cam->yaw);
    double sin_y = sin(cam->yaw);
    double x1 = cos_y * tx - sin_y * tz;
    double y1 = ty;
    double z1 = sin_y * tx + cos_y * tz;
    double cos_p = cos(cam->pitch);
    double sin_p = sin(cam->pitch);
    double x2 = x1;
    double y2 = cos_p * y1 + sin_p * z1;
    double z2 = -sin_p * y1 + cos_p * z1;
    Vec3 res = {x2, y2, z2 + cam->dist};
    return res;
}

bool project_camera_to_screen(const Vec3 *cam_pos, const Camera3D *cam, int width, int height, int *sx, int *sy, double *depth)
{
    if (cam_pos->z <= 0.1) return false;
    double aspect = 2.0;
    double f = (double)height * 0.85 * cam->fov_scale;
    double px = (cam_pos->x * f) / cam_pos->z;
    double py = (cam_pos->y * f) / cam_pos->z;
    int scr_x = (int)(width / 2.0 + px * aspect);
    int scr_y = (int)(height / 2.0 - py);
    if (scr_x < 0 || scr_x >= width || scr_y < 0 || scr_y >= height) return false;
    *sx = scr_x;
    *sy = scr_y;
    *depth = cam_pos->z;
    return true;
}

void fb_draw_point_3d(FrameBuffer3D *fb, const Vec3 *p, const Camera3D *cam, char sym, int color_code)
{
    Vec3 cam_pos = transform_world_to_camera(p, cam);
    int sx = 0;
    int sy = 0;
    double depth = 0.0;
    if (project_camera_to_screen(&cam_pos, cam, fb->width, fb->height, &sx, &sy, &depth)) {
        if (depth < fb->zbuf[sy][sx]) {
            fb->zbuf[sy][sx] = depth;
            fb->chars[sy][sx] = sym;
            fb->colors[sy][sx] = color_code;
        }
    }
}

void fb_draw_line_3d(FrameBuffer3D *fb, const Vec3 *p0, const Vec3 *p1, const Camera3D *cam, char sym, int color_code)
{
    Vec3 cam0 = transform_world_to_camera(p0, cam);
    Vec3 cam1 = transform_world_to_camera(p1, cam);
    int sx0 = 0;
    int sy0 = 0;
    int sx1 = 0;
    int sy1 = 0;
    double d0 = 0.0;
    double d1 = 0.0;
    bool v0 = project_camera_to_screen(&cam0, cam, fb->width, fb->height, &sx0, &sy0, &d0);
    bool v1 = project_camera_to_screen(&cam1, cam, fb->width, fb->height, &sx1, &sy1, &d1);
    if (!v0 && !v1) return;
    if (!v0 && v1) {
        sx0 = sx1;
        sy0 = sy1;
        d0 = d1;
    } else if (v0 && !v1) {
        sx1 = sx0;
        sy1 = sy0;
        d1 = d0;
    }
    int dx = abs(sx1 - sx0);
    int dy = abs(sy1 - sy0);
    int steps = dx > dy ? dx : dy;
    if (steps == 0) {
        if (d0 < fb->zbuf[sy0][sx0]) {
            fb->zbuf[sy0][sx0] = d0;
            fb->chars[sy0][sx0] = sym;
            fb->colors[sy0][sx0] = color_code;
        }
        return;
    }
    for (int i = 0; i <= steps; i++) {
        double t = (double)i / (double)steps;
        int sx = (int)(sx0 + t * (sx1 - sx0));
        int sy = (int)(sy0 + t * (sy1 - sy0));
        double depth = d0 + t * (d1 - d0);
        if (sx >= 0 && sx < fb->width && sy >= 0 && sy < fb->height) {
            if (depth < fb->zbuf[sy][sx]) {
                fb->zbuf[sy][sx] = depth;
                fb->chars[sy][sx] = sym;
                fb->colors[sy][sx] = color_code;
            }
        }
    }
}

void fb_draw_axes_3d(FrameBuffer3D *fb, const Camera3D *cam, double length)
{
    Vec3 o = {0.0, 0.0, 0.0};
    Vec3 x = {length, 0.0, 0.0};
    Vec3 y = {0.0, length, 0.0};
    Vec3 z = {0.0, 0.0, length};
    fb_draw_line_3d(fb, &o, &x, cam, '-', 31);
    fb_draw_line_3d(fb, &o, &y, cam, '|', 32);
    fb_draw_line_3d(fb, &o, &z, cam, '/', 34);
    fb_draw_point_3d(fb, &x, cam, 'X', 31);
    fb_draw_point_3d(fb, &y, cam, 'Y', 32);
    fb_draw_point_3d(fb, &z, cam, 'Z', 34);
}

void fb_draw_ellipsoid_wireframe_3d(FrameBuffer3D *fb, const Camera3D *cam, double a, double b, double c, int rings)
{
    for (int i = 0; i < rings; i++) {
        double phi0 = 2.0 * M_PI * (double)i / (double)rings;
        double phi1 = 2.0 * M_PI * (double)(i + 1) / (double)rings;
        Vec3 p_prev_xy = {a * cos(phi0), b * sin(phi0), 0.0};
        Vec3 p_curr_xy = {a * cos(phi1), b * sin(phi1), 0.0};
        fb_draw_line_3d(fb, &p_prev_xy, &p_curr_xy, cam, '.', 33);
        Vec3 p_prev_xz = {a * cos(phi0), 0.0, c * sin(phi0)};
        Vec3 p_curr_xz = {a * cos(phi1), 0.0, c * sin(phi1)};
        fb_draw_line_3d(fb, &p_prev_xz, &p_curr_xz, cam, '.', 35);
        Vec3 p_prev_yz = {0.0, b * cos(phi0), c * sin(phi0)};
        Vec3 p_curr_yz = {0.0, b * cos(phi1), c * sin(phi1)};
        fb_draw_line_3d(fb, &p_prev_yz, &p_curr_yz, cam, '.', 36);
    }
}

void fb_draw_orbit_trajectory_3d(FrameBuffer3D *fb, const OrbitTrajectory *tr, const Camera3D *cam, int color_code)
{
    if (!tr || tr->count < 2) return;
    for (int i = 0; i < tr->count - 1; i++) {
        Vec3 p0 = {tr->points[i].x, tr->points[i].y, tr->points[i].z};
        Vec3 p1 = {tr->points[i + 1].x, tr->points[i + 1].y, tr->points[i + 1].z};
        fb_draw_line_3d(fb, &p0, &p1, cam, '*', color_code);
    }
    Vec3 start_pt = {tr->points[0].x, tr->points[0].y, tr->points[0].z};
    Vec3 end_pt = {tr->points[tr->count - 1].x, tr->points[tr->count - 1].y, tr->points[tr->count - 1].z};
    fb_draw_point_3d(fb, &start_pt, cam, 'S', 32);
    fb_draw_point_3d(fb, &end_pt, cam, 'E', 31);
}

void fb_draw_bloch_sphere_3d(FrameBuffer3D *fb, const Camera3D *cam, double rx, double ry, double rz)
{
    fb_draw_axes_3d(fb, cam, 1.3);
    fb_draw_ellipsoid_wireframe_3d(fb, cam, 1.0, 1.0, 1.0, 36);
    for (int i = 0; i < 36; i++) {
        double phi0 = 2.0 * M_PI * (double)i / 36.0;
        double phi1 = 2.0 * M_PI * (double)(i + 1) / 36.0;
        double z_slice = 0.5;
        double r_slice = sqrt(1.0 - z_slice * z_slice);
        Vec3 p0 = {r_slice * cos(phi0), r_slice * sin(phi0), z_slice};
        Vec3 p1 = {r_slice * cos(phi1), r_slice * sin(phi1), z_slice};
        fb_draw_line_3d(fb, &p0, &p1, cam, ':', 37);
        Vec3 p0_neg = {r_slice * cos(phi0), r_slice * sin(phi0), -z_slice};
        Vec3 p1_neg = {r_slice * cos(phi1), r_slice * sin(phi1), -z_slice};
        fb_draw_line_3d(fb, &p0_neg, &p1_neg, cam, ':', 37);
    }
    Vec3 origin = {0.0, 0.0, 0.0};
    Vec3 bloch_vec = {rx, ry, rz};
    fb_draw_line_3d(fb, &origin, &bloch_vec, cam, '#', 32);
    fb_draw_point_3d(fb, &bloch_vec, cam, '@', 32);
}

void fb_render_to_terminal(const FrameBuffer3D *fb)
{
    printf("+");
    for (int x = 0; x < fb->width; x++) printf("-");
    printf("+\n");
    for (int y = 0; y < fb->height; y++) {
        printf("|");
        for (int x = 0; x < fb->width; x++) {
            int col = fb->colors[y][x];
            char ch = fb->chars[y][x];
            if (col > 0) {
                printf("\033[%dm%c\033[0m", col, ch);
            } else {
                putchar(ch);
            }
        }
        printf("|\n");
    }
    printf("+");
    for (int x = 0; x < fb->width; x++) printf("-");
    printf("+\n");
}

#endif
