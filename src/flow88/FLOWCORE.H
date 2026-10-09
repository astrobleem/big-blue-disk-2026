/* Original Gemini/Claude solver; safety corrections by Codex for Chad.
 * C89, pure host-testable 16-bit storage and widened arithmetic. GPLv3. */
#ifndef FLOW_CORE_H
#define FLOW_CORE_H
#include <stdlib.h>
#include <string.h>
/* ----------------------------------------------------------------------------
 * CONSTANTS & CONFIGURATION
 * ---------------------------------------------------------------------------- */
#define GRID_W        78
#define GRID_H        23
#define GRID_SIZE     (GRID_W * GRID_H)

#define VIEW_X        1
#define VIEW_Y        1

#define VRAM_SEGMENT  0xB800

/* Signed 8.8 values use an explicitly 16-bit storage type on host and DOS.
   Widen before sums, differences and products; saturate before narrowing. */
typedef short Fix;
#define FIX_SHIFT 8
#define FIX_ONE 256
#define FIX_HALF 128
static Fix fix_sat(long x) { return (Fix)(x>32767L?32767L:x< -32767L?-32767L:x); }
static Fix fix_mul(Fix a,Fix b) { return fix_sat((long)a*b/256L); }
static long fix_abs(Fix x) { return x<0?-(long)x:(long)x; }
#define INT_TO_FIX(x) fix_sat((long)(x)*256L)
#define FIX_TO_INT(x) ((int)((x)/256))
#define FIX_MUL(a,b) fix_mul((Fix)(a),(Fix)(b))
static int density_slot(Fix x) { return x<=0?0:1+(int)((long)x*5L/32767L); }

/* Simulation Modes */
#define MODE_DENSITY  0
#define MODE_VELOCITY 1
#define MODE_OBSTACLE 2

/* Key Codes */
#define KEY_ESC       0x1B
#define KEY_SPACE     0x20
#define KEY_ENTER     0x0D
#define KEY_UP        72
#define KEY_LEFT      75
#define KEY_RIGHT     77
#define KEY_DOWN      80

/* ----------------------------------------------------------------------------
 * GLOBAL SIMULATION DATA
 * ---------------------------------------------------------------------------- */
/* Velocity fields (8.8 fixed point) */
Fix u[GRID_W][GRID_H];
Fix v[GRID_W][GRID_H];
Fix u_prev[GRID_W][GRID_H];
Fix v_prev[GRID_W][GRID_H];

/* Density fields (8.8 fixed point) */
Fix dens[GRID_W][GRID_H];
Fix dens_prev[GRID_W][GRID_H];

/* Pressure & Divergence buffers (8.8 fixed point) */
Fix pressure[GRID_W][GRID_H];
Fix div_grid[GRID_W][GRID_H];

/* Obstacle map (0 = fluid, 1 = solid barrier) */
unsigned char obstacle[GRID_W][GRID_H];

/* UI & Interactive state */
int cursor_x = GRID_W / 2;
int cursor_y = GRID_H / 2;
int current_mode = MODE_DENSITY;
int show_velocity = 0;
int is_paused = 0;
int inject_fluid = 0;
int sound_enabled = 1;
int is_tandy = 0;

/* ASCII density ramp and corresponding color attributes */
const char density_chars[7] = {' ', '.', ':', '+', '*', '#', '@'};
const unsigned char density_attrs[7] = {
    0x07, /* Dark Gray / Low */
    0x01, /* Blue */
    0x03, /* Cyan */
    0x02, /* Green */
    0x0E, /* Yellow */
    0x0F, /* Bright White */
    0x7F  /* Inverse / Ultra Density */
};

/* ----------------------------------------------------------------------------
 * CORE PHYSICS SOLVER (FIXED-POINT NAVIER-STOKES)
 * ---------------------------------------------------------------------------- */

/* Enforce boundary condition & solid obstacle reflection */
void set_bnd(int b, Fix x_grid[GRID_W][GRID_H]) {
    int i, j;
    
    /* Domain boundary walls */
    for (i = 1; i < GRID_W - 1; i++) {
        x_grid[i][0]          = (b == 2) ? fix_sat(-(long)x_grid[i][1]) : x_grid[i][1];
        x_grid[i][GRID_H - 1] = (b == 2) ? fix_sat(-(long)x_grid[i][GRID_H - 2]) : x_grid[i][GRID_H - 2];
    }
    for (j = 1; j < GRID_H - 1; j++) {
        x_grid[0][j]          = (b == 1) ? fix_sat(-(long)x_grid[1][j]) : x_grid[1][j];
        x_grid[GRID_W - 1][j] = (b == 1) ? fix_sat(-(long)x_grid[GRID_W - 2][j]) : x_grid[GRID_W - 2][j];
    }

    /* Solid obstacle barriers */
    for (i = 1; i < GRID_W - 1; i++) {
        for (j = 1; j < GRID_H - 1; j++) {
            if (obstacle[i][j]) {
                if (b == 1) x_grid[i][j] = fix_sat(-(long)x_grid[i - 1][j]);
                else if (b == 2) x_grid[i][j] = fix_sat(-(long)x_grid[i][j - 1]);
                else x_grid[i][j] = 0;
            }
        }
    }
}

/* Inject source density/velocity from buffers */
void add_source(Fix x_grid[GRID_W][GRID_H], Fix s_grid[GRID_W][GRID_H], int dt) {
    int i, j;
    for (i = 0; i < GRID_W; i++) {
        for (j = 0; j < GRID_H; j++) {
            long sum = (long)x_grid[i][j] + FIX_MUL(s_grid[i][j], dt);
            if (sum > 32767L) sum = 32767L;
            if (sum < -32767L) sum = -32767L;
            x_grid[i][j] = fix_sat(sum);
        }
    }
}

/* Linear Interpolation Advection Solver */
void advect(int b, Fix d[GRID_W][GRID_H], Fix d0[GRID_W][GRID_H], 
            Fix u_f[GRID_W][GRID_H], Fix v_f[GRID_W][GRID_H], int dt) {
    int i, j, i0, j0, i1, j1;
    long x_fp, y_fp;
    int dt0;
    int s0, s1, t0, t1;

    dt0 = dt;
    for (i = 1; i < GRID_W - 1; i++) {
        for (j = 1; j < GRID_H - 1; j++) {
            if (obstacle[i][j]) {
                d[i][j] = 0;
                continue;
            }

            /* Trace back particle position in 8.8 fixed-point */
            x_fp = (long)INT_TO_FIX(i) - FIX_MUL(dt0, u_f[i][j]);
            y_fp = (long)INT_TO_FIX(j) - FIX_MUL(dt0, v_f[i][j]);

            /* Clamp bounds */
            if (x_fp < FIX_HALF) x_fp = FIX_HALF;
            if (x_fp > INT_TO_FIX(GRID_W - 2) + FIX_HALF) 
                x_fp = INT_TO_FIX(GRID_W - 2) + FIX_HALF;
            
            if (y_fp < FIX_HALF) y_fp = FIX_HALF;
            if (y_fp > INT_TO_FIX(GRID_H - 2) + FIX_HALF) 
                y_fp = INT_TO_FIX(GRID_H - 2) + FIX_HALF;

            i0 = (int)(x_fp/256L);
            i1 = i0 + 1;
            j0 = (int)(y_fp/256L);
            j1 = j0 + 1;

            s1 = (int)(x_fp - INT_TO_FIX(i0));
            s0 = FIX_ONE - s1;
            t1 = (int)(y_fp - INT_TO_FIX(j0));
            t0 = FIX_ONE - t1;

            /* Bilinear interpolation */
            d[i][j] = fix_sat((long)FIX_MUL(s0, FIX_MUL(t0, d0[i0][j0]) + FIX_MUL(t1, d0[i0][j1])) +
                      FIX_MUL(s1, FIX_MUL(t0, d0[i1][j0]) + FIX_MUL(t1, d0[i1][j1])));
        }
    }
    set_bnd(b, d);
}

/* Fast Gauss-Seidel Pressure Projection Routine */
void project(Fix u_f[GRID_W][GRID_H], Fix v_f[GRID_W][GRID_H], 
             Fix p_f[GRID_W][GRID_H], Fix div_f[GRID_W][GRID_H]) {
    int i, j, k;

    /* Compute velocity divergence field */
    for (i = 1; i < GRID_W - 1; i++) {
        for (j = 1; j < GRID_H - 1; j++) {
            if (obstacle[i][j]) {
                div_f[i][j] = 0;
                p_f[i][j] = 0;
                continue;
            }
            div_f[i][j] = fix_sat(-(((long)u_f[i + 1][j] - u_f[i - 1][j]) +
                            ((long)v_f[i][j + 1] - v_f[i][j - 1])) / 2L);
            p_f[i][j] = 0;
        }
    }
    set_bnd(0, div_f);
    set_bnd(0, p_f);

    /* 4-Iteration Gauss-Seidel Relaxation */
    for (k = 0; k < 4; k++) {
        for (i = 1; i < GRID_W - 1; i++) {
            for (j = 1; j < GRID_H - 1; j++) {
                if (obstacle[i][j]) continue;
                
                p_f[i][j] = fix_sat(((long)div_f[i][j] + p_f[i - 1][j] + p_f[i + 1][j] +
                             p_f[i][j - 1] + p_f[i][j + 1]) / 4L);
            }
        }
        set_bnd(0, p_f);
    }

    /* Subtract pressure gradient to resolve divergence-free velocity */
    for (i = 1; i < GRID_W - 1; i++) {
        for (j = 1; j < GRID_H - 1; j++) {
            if (obstacle[i][j]) {
                u_f[i][j] = 0;
                v_f[i][j] = 0;
                continue;
            }
            u_f[i][j] = fix_sat((long)u_f[i][j] - ((long)p_f[i + 1][j] - p_f[i - 1][j]) / 2L);
            v_f[i][j] = fix_sat((long)v_f[i][j] - ((long)p_f[i][j + 1] - p_f[i][j - 1]) / 2L);
        }
    }
    set_bnd(1, u_f);
    set_bnd(2, v_f);
}

/* Step simulation forward */
int fluid_step(int dt) {
    int i, j;
    long total_vel = 0;

    /* Velocity Step */
    add_source(u, u_prev, dt);
    add_source(v, v_prev, dt);
    
    /* Advect velocity */
    advect(1, u_prev, u, u, v, dt);
    advect(2, v_prev, v, u, v, dt);
    
    /* Project to enforce mass conservation */
    project(u_prev, v_prev, pressure, div_grid);

    /* Copy back velocity fields */
    for (i = 0; i < GRID_W; i++) {
        for (j = 0; j < GRID_H; j++) {
            u[i][j] = u_prev[i][j];
            v[i][j] = v_prev[i][j];
            u_prev[i][j] = 0;
            v_prev[i][j] = 0;

            /* Accumulate average kinetic energy for audio feedback */
            total_vel += fix_abs(u[i][j]) + fix_abs(v[i][j]);
        }
    }

    /* Density Step */
    add_source(dens, dens_prev, dt);
    advect(0, dens_prev, dens, u, v, dt);

    for (i = 0; i < GRID_W; i++) {
        for (j = 0; j < GRID_H; j++) {
            dens[i][j] = dens_prev[i][j];
            dens_prev[i][j] = 0;
        }
    }

    return (int)fix_sat(total_vel / GRID_SIZE);
}

/* ----------------------------------------------------------------------------
 * OBSTACLE PRESETS & SCENARIOS
 * ---------------------------------------------------------------------------- */

void clear_simulation(void) {
    memset(u, 0, sizeof(u));
    memset(v, 0, sizeof(v));
    memset(u_prev, 0, sizeof(u_prev));
    memset(v_prev, 0, sizeof(v_prev));
    memset(dens, 0, sizeof(dens));
    memset(dens_prev, 0, sizeof(dens_prev));
    memset(pressure, 0, sizeof(pressure));
    memset(div_grid, 0, sizeof(div_grid));
}

void load_preset_obstacle(int type) {
    int i, j;
    memset(obstacle, 0, sizeof(obstacle));

    if (type == 1) {
        /* NACA Airfoil profile approximation */
        for (i = 25; i < 45; i++) {
            int thickness = (45 - i) / 4;
            for (j = 12 - thickness; j <= 12 + thickness; j++) {
                if (j >= 1 && j < GRID_H - 1) obstacle[i][j] = 1;
            }
        }
    } else if (type == 2) {
        /* Square Barrier in flow channel */
        for (i = 35; i <= 40; i++) {
            for (j = 8; j <= 16; j++) {
                obstacle[i][j] = 1;
            }
        }
    }
}


#endif
