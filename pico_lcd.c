#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "PICO_LCD.h"
#include "LCD_2in.h"

#define COLOR_BLACK_CELL 0U
#define COLOR_WHITE_CELL 1U
#define COLOR_BORDER_CELL 2U

typedef struct {
    int x;
    int y;
    int dx;
    int dy;
    uint16_t draw_color;
    UBYTE target_cell; /* If found, we paint and bounce. */
    UBYTE paint_to;
} Pixel;

static inline bool reserved_addr(uint8_t addr) {
    return (addr & 0x78U) == 0U || (addr & 0x78U) == 0x78U;
}

static inline bool in_inner_bounds(int x, int y) {
    return x > 0 && x < (LCD_2IN_WIDTH - 1) && y > 0 && y < (LCD_2IN_HEIGHT - 1);
}

static void draw_border(UBYTE image[LCD_2IN_HEIGHT][LCD_2IN_WIDTH]) {
    for (int y = 0; y < LCD_2IN_HEIGHT; ++y) {
        LCD_2IN_DisplayPoint(y, 0, RED);
        LCD_2IN_DisplayPoint(y, LCD_2IN_WIDTH - 1, RED);
        image[y][0] = COLOR_BORDER_CELL;
        image[y][LCD_2IN_WIDTH - 1] = COLOR_BORDER_CELL;
    }

    for (int x = 0; x < LCD_2IN_WIDTH; ++x) {
        LCD_2IN_DisplayPoint(0, x, RED);
        LCD_2IN_DisplayPoint(LCD_2IN_HEIGHT - 1, x, RED);
        image[0][x] = COLOR_BORDER_CELL;
        image[LCD_2IN_HEIGHT - 1][x] = COLOR_BORDER_CELL;
    }
}

static void fill_halves(UBYTE image[LCD_2IN_HEIGHT][LCD_2IN_WIDTH]) {
    const int mid = LCD_2IN_WIDTH / 2;

    for (int y = 1; y < LCD_2IN_HEIGHT - 1; ++y) {
        for (int x = 1; x < mid; ++x) {
            LCD_2IN_DisplayPoint(y, x, WHITE);
            image[y][x] = COLOR_WHITE_CELL;
        }
        for (int x = mid; x < LCD_2IN_WIDTH - 1; ++x) {
            image[y][x] = COLOR_BLACK_CELL;
        }
    }
}

static void bounce_on_bounds(Pixel *p) {
    if (p->x <= 0 || p->x >= LCD_2IN_WIDTH - 1) {
        p->dx = -p->dx;
        p->x = (p->x <= 0) ? 1 : (LCD_2IN_WIDTH - 2);
    }
    if (p->y <= 0 || p->y >= LCD_2IN_HEIGHT - 1) {
        p->dy = -p->dy;
        p->y = (p->y <= 0) ? 1 : (LCD_2IN_HEIGHT - 2);
    }
}

static void paint_if_target(
    Pixel *p,
    UBYTE image[LCD_2IN_HEIGHT][LCD_2IN_WIDTH],
    int tx,
    int ty,
    bool flip_dx,
    bool flip_dy
) {
    if (!in_inner_bounds(tx, ty)) {
        return;
    }

    if (image[ty][tx] == p->target_cell) {
        image[ty][tx] = p->paint_to;
        LCD_2IN_DisplayPoint(ty, tx, p->draw_color);

        if (flip_dx) {
            p->dx = -p->dx;
        }
        if (flip_dy) {
            p->dy = -p->dy;
        }
    }
}

int PICO_LCD(void) {
    (void)reserved_addr(0U); /* Keep utility linked without warnings if currently unused. */

    UBYTE image[LCD_2IN_HEIGHT][LCD_2IN_WIDTH] = {0};

    DEV_Delay_ms(100);
    if (DEV_Module_Init() != 0) {
        return -1;
    }

    DEV_SET_PWM(50);
    LCD_2IN_Init(HORIZONTAL);
    LCD_2IN_Clear(BLACK);

    draw_border(image);
    fill_halves(image);

    DEV_Delay_ms(5000);

    Pixel p1 = {
        .x = LCD_2IN_WIDTH / 2 + 83,
        .y = LCD_2IN_HEIGHT / 2 + 3,
        .dx = 1,
        .dy = 1,
        .draw_color = BLACK,
        .target_cell = COLOR_WHITE_CELL,
        .paint_to = COLOR_BLACK_CELL,
    };

    Pixel p2 = {
        .x = LCD_2IN_WIDTH / 2 - 27,
        .y = LCD_2IN_HEIGHT / 2 - 7,
        .dx = -1,
        .dy = 1,
        .draw_color = WHITE,
        .target_cell = COLOR_BLACK_CELL,
        .paint_to = COLOR_WHITE_CELL,
    };

    while (1) {
        p1.x += p1.dx;
        p1.y += p1.dy;
        p2.x += p2.dx;
        p2.y += p2.dy;

        if (abs(p1.x - p2.x) < 3 && abs(p1.y - p2.y) < 3) {
            p1.dx = -p1.dx;
            p2.dy = -p2.dy;
            p1.x += p1.dx;
            p1.y += p1.dy;
            p2.x += p2.dx;
            p2.y += p2.dy;
        }

        bounce_on_bounds(&p1);
        bounce_on_bounds(&p2);

        paint_if_target(&p1, image, p1.x + p1.dx, p1.y, true, false);
        paint_if_target(&p1, image, p1.x, p1.y + p1.dy, false, true);
        paint_if_target(&p1, image, p1.x + p1.dx, p1.y + p1.dy, true, true);
        paint_if_target(&p1, image, p1.x, p1.y, true, true);

        paint_if_target(&p2, image, p2.x + p2.dx, p2.y, true, false);
        paint_if_target(&p2, image, p2.x, p2.y + p2.dy, false, true);
        paint_if_target(&p2, image, p2.x + p2.dx, p2.y + p2.dy, true, true);
        paint_if_target(&p2, image, p2.x, p2.y, true, true);
    }

    DEV_Module_Exit();
    return 0;
}
