/* Provenance: original program design and first draft by Gemini (Google).
 * Ported to Microsoft C 6 and fixed by Claude (Anthropic): Borland calls
 * mapped to MSC6, asm blocks replaced by C, pressure-solve indexing fixed,
 * add_source saturation, Tandy detection. Edited and submitted by Chad.
 * Contributed under this repository's license.
 * Status: builds with MSC6 and exits cleanly under scripted DOSBox-X input.
 * Rendering, speed on a real 8088, sound and the Tandy ID check are NOT
 * verified; nothing here is hardware-tested.
 */
/* ============================================================================
 * FLOW-88: ASCII Fluid Dynamics & Navier-Stokes Lab for DOS
 * Target Architecture: Intel 8088 / 8086 CPU @ 4.77 MHz to 9.54 MHz
 * Display Mode: Text Mode 03h (80x25 characters, 16 colors)
 * Video Memory: Direct access at segment 0xB800
 * Math: 16-Bit Fixed-Point Arithmetic (8.8 Format)
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <conio.h>
#include <time.h>
#include <string.h>

/* MSC6 port helpers: Borland names mapped onto MSC6 conio/dos primitives. */
#define outportb(p,v) ((void)outp((p),(v)))
#define inportb(p)    ((unsigned char)inp(p))

/* ----------------------------------------------------------------------------
 * CONSTANTS & CONFIGURATION
 * ---------------------------------------------------------------------------- */
#define GRID_W        78
#define GRID_H        23
#define GRID_SIZE     (GRID_W * GRID_H)

#define VIEW_X        1
#define VIEW_Y        1

#define VRAM_SEGMENT  0xB800

/* Fixed-point 8.8 representation macros */
#define FIX_SHIFT     8
#define FIX_ONE       256
#define FIX_HALF      128

#define INT_TO_FIX(x) ((int)((x) << FIX_SHIFT))
#define FIX_TO_INT(x) ((int)((x) >> FIX_SHIFT))
#define FIX_MUL(a, b) ((int)(((long)(a) * (b)) >> FIX_SHIFT))
#define FIX_DIV(a, b) ((int)(((long)(a) << FIX_SHIFT) / (b)))

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
int u[GRID_W][GRID_H];
int v[GRID_W][GRID_H];
int u_prev[GRID_W][GRID_H];
int v_prev[GRID_W][GRID_H];

/* Density fields (8.8 fixed point) */
int dens[GRID_W][GRID_H];
int dens_prev[GRID_W][GRID_H];

/* Pressure & Divergence buffers (8.8 fixed point) */
int pressure[GRID_W][GRID_H];
int div_grid[GRID_W][GRID_H];

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
 * DIRECT HARDWARE I/O & SOUND SUBROUTINES
 * ---------------------------------------------------------------------------- */

/* Busy-wait delay. Uncalibrated: roughly 1 ms per unit on a 4.77 MHz 8088. */
static void delay(unsigned ms) {
    volatile unsigned n;
    while (ms--) for (n = 100; n; n--) ;
}

static void bios_video(unsigned ax, unsigned cx) {
    union REGS r;
    r.x.ax = ax; r.x.cx = cx;
    int86(0x10, &r, &r);
}
#define clrscr()                bios_video(0x0003, 0)
#define hide_cursor()           bios_video(0x0100, 0x2000)
#define show_cursor()           bios_video(0x0100, 0x0607)

/* Check hardware environment for Tandy 1000 / PCjr sound chip */
void detect_hardware(void) {
    /* Read BIOS Machine ID at 0xF000:0xFFFE */
    unsigned char far *bios_id = (unsigned char far *)0xF000FFFEUL;
    unsigned char far *tandy_id = (unsigned char far *)0xF000C000UL;
    /* 0xFD = PCjr. Tandy 1000 reports 0xFF like a PC, so also require the
       Tandy marker byte 0x21 at F000:C000. */
    is_tandy = (*bios_id == 0xFD) || (*bios_id == 0xFF && *tandy_id == 0x21);
}

/* Fallback PC Speaker tone generation */
void sound_click(int freq, int duration_ms) {
    long count;
    if (!sound_enabled) return;
    
    count = 1193180L / freq;
    outportb(0x43, 0xB6);
    outportb(0x42, (unsigned char)(count & 0xFF));
    outportb(0x42, (unsigned char)((count >> 8) & 0xFF));
    outportb(0x61, inportb(0x61) | 0x03);
    
    delay(duration_ms);
    outportb(0x61, inportb(0x61) & ~0x03);
}

/* Sound synthesis: Tandy SN76496 low-frequency turbulence hum */
void update_audio_hum(int avg_velocity) {
    int freq_val;
    if (!sound_enabled) return;

    if (is_tandy) {
        /* Map fluid kinetic energy to SN76496 sound generator at port 0xC0 */
        freq_val = 0x3F - ((avg_velocity >> 4) & 0x1F);
        if (freq_val < 5) freq_val = 5;
        
        /* Channel 0 tone control bytes for SN76496 */
        outportb(0xC0, 0x80 | (freq_val & 0x0F));
        outportb(0xC0, (freq_val >> 4) & 0x3F);
        outportb(0xC0, 0x90 | ((avg_velocity > 10) ? 0x02 : 0x0F)); /* Attenuation */
    } else {
        /* Subtle PC Speaker pulse if turbulence is high */
        if (avg_velocity > INT_TO_FIX(1)) {
            sound_click(100 + (avg_velocity >> 2), 2);
        }
    }
}

/* Stop all active audio output */
void mute_audio(void) {
    outportb(0x61, inportb(0x61) & ~0x03);
    if (is_tandy) {
        outportb(0xC0, 0x9F); /* Mute Ch0 */
        outportb(0xC0, 0xBF); /* Mute Ch1 */
        outportb(0xC0, 0xDF); /* Mute Ch2 */
        outportb(0xC0, 0xFF); /* Mute Noise */
    }
}

/* ----------------------------------------------------------------------------
 * DIRECT VIDEO RAM (0xB800) DRAWING ROUTINES
 * ---------------------------------------------------------------------------- */

/* Fast cell write directly to Text VRAM using inline assembly */
void vram_put_char(int x, int y, char ch, unsigned char attr) {
    unsigned int far *cell = (unsigned int far *)(((unsigned long)VRAM_SEGMENT << 16) + (unsigned)((y * 80 + x) * 2));
    *cell = ((unsigned)attr << 8) | (unsigned char)ch;
}

/* Draw a screen-aligned string into VRAM */
void vram_print_str(int x, int y, const char *str, unsigned char attr) {
    int i = 0;
    while (str[i] != '\0') {
        vram_put_char(x + i, y, str[i], attr);
        i++;
    }
}

/* Draw the viewport border and interactive status headers */
void draw_ui_frame(void) {
    int x, y;

    /* Top Status Bar Background */
    for (x = 0; x < 80; x++) {
        vram_put_char(x, 0, ' ', 0x70);
        vram_put_char(x, 24, ' ', 0x70);
    }

    /* Single-line border around $78 \times 23$ fluid viewport */
    vram_put_char(VIEW_X - 1, VIEW_Y - 1, 0xDA, 0x07); /* Top-left corner */
    vram_put_char(VIEW_X + GRID_W, VIEW_Y - 1, 0xBF, 0x07); /* Top-right corner */
    vram_put_char(VIEW_X - 1, VIEW_Y + GRID_H, 0xC0, 0x07); /* Bottom-left corner */
    vram_put_char(VIEW_X + GRID_W, VIEW_Y + GRID_H, 0xD9, 0x07); /* Bottom-right corner */

    for (x = VIEW_X; x < VIEW_X + GRID_W; x++) {
        vram_put_char(x, VIEW_Y - 1, 0xC4, 0x07);
        vram_put_char(x, VIEW_Y + GRID_H, 0xC4, 0x07);
    }
    for (y = VIEW_Y; y < VIEW_Y + GRID_H; y++) {
        vram_put_char(VIEW_X - 1, y, 0xB3, 0x07);
        vram_put_char(VIEW_X + GRID_W, y, 0xB3, 0x07);
    }

    /* Bottom Hotkey Instructions */
    vram_print_str(1, 24, "[ARROWS] Move [SPACE] Inject [B] Obstacle [V] Vectors [C] Clear [Q] Quit", 0x70);
}

/* ----------------------------------------------------------------------------
 * CORE PHYSICS SOLVER (FIXED-POINT NAVIER-STOKES)
 * ---------------------------------------------------------------------------- */

/* Enforce boundary condition & solid obstacle reflection */
void set_bnd(int b, int x_grid[GRID_W][GRID_H]) {
    int i, j;
    
    /* Domain boundary walls */
    for (i = 1; i < GRID_W - 1; i++) {
        x_grid[i][0]          = (b == 2) ? -x_grid[i][1] : x_grid[i][1];
        x_grid[i][GRID_H - 1] = (b == 2) ? -x_grid[i][GRID_H - 2] : x_grid[i][GRID_H - 2];
    }
    for (j = 1; j < GRID_H - 1; j++) {
        x_grid[0][j]          = (b == 1) ? -x_grid[1][j] : x_grid[1][j];
        x_grid[GRID_W - 1][j] = (b == 1) ? -x_grid[GRID_W - 2][j] : x_grid[GRID_W - 2][j];
    }

    /* Solid obstacle barriers */
    for (i = 1; i < GRID_W - 1; i++) {
        for (j = 1; j < GRID_H - 1; j++) {
            if (obstacle[i][j]) {
                if (b == 1) x_grid[i][j] = -x_grid[i - 1][j];
                else if (b == 2) x_grid[i][j] = -x_grid[i][j - 1];
                else x_grid[i][j] = 0;
            }
        }
    }
}

/* Inject source density/velocity from buffers */
void add_source(int x_grid[GRID_W][GRID_H], int s_grid[GRID_W][GRID_H], int dt) {
    int i, j;
    for (i = 0; i < GRID_W; i++) {
        for (j = 0; j < GRID_H; j++) {
            long sum = (long)x_grid[i][j] + FIX_MUL(s_grid[i][j], dt);
            if (sum > 32767L) sum = 32767L;
            if (sum < -32767L) sum = -32767L;
            x_grid[i][j] = (int)sum;
        }
    }
}

/* Linear Interpolation Advection Solver */
void advect(int b, int d[GRID_W][GRID_H], int d0[GRID_W][GRID_H], 
            int u_f[GRID_W][GRID_H], int v_f[GRID_W][GRID_H], int dt) {
    int i, j, i0, j0, i1, j1;
    int x_fp, y_fp, dt0;
    int s0, s1, t0, t1;

    dt0 = dt;
    for (i = 1; i < GRID_W - 1; i++) {
        for (j = 1; j < GRID_H - 1; j++) {
            if (obstacle[i][j]) {
                d[i][j] = 0;
                continue;
            }

            /* Trace back particle position in 8.8 fixed-point */
            x_fp = INT_TO_FIX(i) - FIX_MUL(dt0, u_f[i][j]);
            y_fp = INT_TO_FIX(j) - FIX_MUL(dt0, v_f[i][j]);

            /* Clamp bounds */
            if (x_fp < FIX_HALF) x_fp = FIX_HALF;
            if (x_fp > INT_TO_FIX(GRID_W - 2) + FIX_HALF) 
                x_fp = INT_TO_FIX(GRID_W - 2) + FIX_HALF;
            
            if (y_fp < FIX_HALF) y_fp = FIX_HALF;
            if (y_fp > INT_TO_FIX(GRID_H - 2) + FIX_HALF) 
                y_fp = INT_TO_FIX(GRID_H - 2) + FIX_HALF;

            i0 = FIX_TO_INT(x_fp);
            i1 = i0 + 1;
            j0 = FIX_TO_INT(y_fp);
            j1 = j0 + 1;

            s1 = x_fp - INT_TO_FIX(i0);
            s0 = FIX_ONE - s1;
            t1 = y_fp - INT_TO_FIX(j0);
            t0 = FIX_ONE - t1;

            /* Bilinear interpolation */
            d[i][j] = FIX_MUL(s0, FIX_MUL(t0, d0[i0][j0]) + FIX_MUL(t1, d0[i0][j1])) +
                      FIX_MUL(s1, FIX_MUL(t0, d0[i1][j0]) + FIX_MUL(t1, d0[i1][j1]));
        }
    }
    set_bnd(b, d);
}

/* Fast Gauss-Seidel Pressure Projection Routine */
void project(int u_f[GRID_W][GRID_H], int v_f[GRID_W][GRID_H], 
             int p_f[GRID_W][GRID_H], int div_f[GRID_W][GRID_H]) {
    int i, j, k;

    /* Compute velocity divergence field */
    for (i = 1; i < GRID_W - 1; i++) {
        for (j = 1; j < GRID_H - 1; j++) {
            if (obstacle[i][j]) {
                div_f[i][j] = 0;
                p_f[i][j] = 0;
                continue;
            }
            div_f[i][j] = -((u_f[i + 1][j] - u_f[i - 1][j]) + 
                            (v_f[i][j + 1] - v_f[i][j - 1])) / 2;
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
                
                p_f[i][j] = (div_f[i][j] + p_f[i - 1][j] + p_f[i + 1][j] +
                             p_f[i][j - 1] + p_f[i][j + 1]) >> 2;
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
            u_f[i][j] -= (p_f[i + 1][j] - p_f[i - 1][j]) / 2;
            v_f[i][j] -= (p_f[i][j + 1] - p_f[i][j - 1]) / 2;
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
            total_vel += abs(u[i][j]) + abs(v[i][j]);
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

    return (int)(total_vel / GRID_SIZE);
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

/* ----------------------------------------------------------------------------
 * RENDERING & DISPLAY SOLVER
 * ---------------------------------------------------------------------------- */

void render_viewport(void) {
    int x, y;
    char draw_ch;
    unsigned char draw_attr;
    int d_val, ramp_idx;
    int vel_u, vel_v;

    for (x = 0; x < GRID_W; x++) {
        for (y = 0; y < GRID_H; y++) {
            if (obstacle[x][y]) {
                draw_ch = 0xDB; /* Solid ASCII block for airfoils/barriers */
                draw_attr = 0x08; /* Dark Gray */
            } else if (show_velocity) {
                vel_u = u[x][y];
                vel_v = v[x][y];
                
                if (abs(vel_u) > abs(vel_v)) {
                    draw_ch = (vel_u > 0) ? '>' : '<';
                } else if (abs(vel_v) > 0) {
                    draw_ch = (vel_v > 0) ? 'v' : '^';
                } else {
                    draw_ch = ' ';
                }
                draw_attr = 0x0A; /* Light Green vectors */
            } else {
                /* Render fluid density levels */
                d_val = FIX_TO_INT(dens[x][y]);
                ramp_idx = d_val / 36; /* Map fixed point scale to 0..6 */
                if (ramp_idx < 0) ramp_idx = 0;
                if (ramp_idx > 6) ramp_idx = 6;

                draw_ch = density_chars[ramp_idx];
                draw_attr = density_attrs[ramp_idx];
            }

            /* Draw cursor overlay */
            if (x == cursor_x && y == cursor_y) {
                draw_attr = 0xCF; /* Blinking Bright White on Red */
            }

            vram_put_char(VIEW_X + x, VIEW_Y + y, draw_ch, draw_attr);
        }
    }
}

/* ----------------------------------------------------------------------------
 * MAIN ENTRY POINT & EVENT LOOP
 * ---------------------------------------------------------------------------- */

int main(void) {
    int done = 0;
    int ch;
    clock_t start_t, end_t;
    int frame_count = 0;
    int current_fps = 0;
    int avg_vel = 0;
    int obstacle_preset = 0;
    char status_buf[80];

    /* Initialize hardware detection & display */
    detect_hardware();
    clrscr();
    hide_cursor();
    draw_ui_frame();

    start_t = clock();

    while (!done) {
        /* Process User Input */
        if (kbhit()) {
            ch = getch();
            if (ch == 0 || ch == 0xE0) {
                /* Extended keyboard arrow codes */
                ch = getch();
                switch (ch) {
                    case KEY_UP:    if (cursor_y > 0) cursor_y--; break;
                    case KEY_DOWN:  if (cursor_y < GRID_H - 1) cursor_y++; break;
                    case KEY_LEFT:  if (cursor_x > 0) cursor_x--; break;
                    case KEY_RIGHT: if (cursor_x < GRID_W - 1) cursor_x++; break;
                }
            } else {
                switch (ch) {
                    case 'q': case 'Q': case KEY_ESC:
                        done = 1;
                        break;
                    case KEY_SPACE:
                    case KEY_ENTER:
                        inject_fluid = !inject_fluid;
                        sound_click(800, 5);
                        break;
                    case 'b': case 'B':
                        obstacle[cursor_x][cursor_y] = !obstacle[cursor_x][cursor_y];
                        sound_click(400, 5);
                        break;
                    case 'v': case 'V':
                        show_velocity = !show_velocity;
                        sound_click(600, 5);
                        break;
                    case 'o': case 'O':
                        obstacle_preset = (obstacle_preset + 1) % 3;
                        load_preset_obstacle(obstacle_preset);
                        sound_click(500, 5);
                        break;
                    case 'c': case 'C':
                        clear_simulation();
                        sound_click(300, 10);
                        break;
                    case 's': case 'S':
                        sound_enabled = !sound_enabled;
                        if (!sound_enabled) mute_audio();
                        break;
                }
            }
        }

        /* Inject fluid density/velocity at cursor when toggled */
        if (inject_fluid) {
            dens_prev[cursor_x][cursor_y] += INT_TO_FIX(8);
            u_prev[cursor_x][cursor_y]    += INT_TO_FIX(2);
            v_prev[cursor_x][cursor_y]    += INT_TO_FIX(1);
        }

        /* Run Navier-Stokes Physics Step */
        if (!is_paused) {
            avg_vel = fluid_step(FIX_HALF);
            update_audio_hum(avg_vel);
        }

        /* Render Viewport & Update Status Header */
        render_viewport();

        frame_count++;
        end_t = clock();
        if ((end_t - start_t) >= CLK_TCK) {
            current_fps = frame_count;
            frame_count = 0;
            start_t = end_t;
        }

        /* Format Top Status Bar */
        sprintf(status_buf, "FLOW-88 | FPS: %2d | Mode: %s | Sound: %s | Audio HW: %s",
                current_fps,
                show_velocity ? "Velocity" : "Density ",
                sound_enabled ? "ON " : "OFF",
                is_tandy ? "SN76496" : "Speaker");
        vram_print_str(1, 0, status_buf, 0x70);

    }

    /* Cleanup & Exit */
    mute_audio();
    show_cursor();
    clrscr();
    printf("Flow-88 Terminated Normally.\n");
    return 0;
}