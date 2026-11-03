/* trainer.c — generic T-menu for game-supplied trainer toggles. */

#include "trainer.h"
#include "overlay_draw.h"

#include <stdlib.h>
#include <string.h>

static const char *const *s_labels;
static int      s_count;
static const char *s_choice_title;
static const char *const *s_choice_values;
static int      s_choice_count;
static int      s_choice;
static int      s_open;
static int      s_selected;
static uint32_t s_mask;

static int row_count(void)
{
    return s_count + (s_choice_count > 0 ? 1 : 0);
}

void trainer_init(const char *const *labels, int count)
{
    s_labels = labels;
    s_count = labels ? count : 0;
    if (s_count > TRAINER_MAX_ITEMS) s_count = TRAINER_MAX_ITEMS;
    if (s_count < 0) s_count = 0;
    s_choice_title = NULL;
    s_choice_values = NULL;
    s_choice_count = 0;
    s_choice = 0;
    s_open = 0;
    s_selected = 0;
    s_mask = 0;
    /* Test seam: preload toggles without driving the menu. */
    const char *m = getenv("GENESIS_TRAINER_MASK");
    if (m && s_count > 0)
        s_mask = (uint32_t)strtoul(m, NULL, 0) & ((1u << s_count) - 1u);
}

int trainer_menu_open(void) { return s_open && s_count > 0; }

void trainer_menu_set_open(int open)
{
    s_open = open && s_count > 0;
}

void trainer_set_choice(const char *title, const char *const *values, int count)
{
    s_choice_title = title;
    s_choice_values = values;
    s_choice_count = (title && values) ? count : 0;
    if (s_choice_count < 0) s_choice_count = 0;
    s_choice = 0;
    /* Test seam: preload the choice row like GENESIS_TRAINER_MASK. */
    const char *c = getenv("GENESIS_TRAINER_CHOICE");
    if (c && s_choice_count > 0)
        s_choice = (int)(strtoul(c, NULL, 0) % (unsigned)s_choice_count);
}

int trainer_choice(void)
{
    return s_choice_count > 0 ? s_choice : 0;
}

void trainer_menu_move(int delta)
{
    int rows = row_count();
    if (rows <= 0) return;
    s_selected = (s_selected + delta) % rows;
    if (s_selected < 0) s_selected += rows;
}

void trainer_menu_adjust(int delta)
{
    if (s_choice_count <= 0 || s_selected != s_count) return;
    s_choice = (s_choice + delta) % s_choice_count;
    if (s_choice < 0) s_choice += s_choice_count;
}

void trainer_toggle_selected(void)
{
    if (s_selected < 0 || s_selected >= s_count) return;
    s_mask ^= 1u << s_selected;
}

uint32_t trainer_mask(void) { return s_mask; }

void trainer_draw(uint32_t *dst, int stride, int height,
                  int screen_w, int screen_h)
{
    (void)height;
    if (!trainer_menu_open()) return;

    const uint32_t bg     = 0xE8101A31u;
    const uint32_t border = 0xFFB66937u;
    const uint32_t gold   = 0xFFF6C66Bu;
    const uint32_t white  = 0xFFF2F1EAu;

    int w = 288;
    int h = 34 + row_count() * 18;
    int x = (screen_w - w) / 2;
    int y = (screen_h - h) / 2;
    if (x < 0) x = 0;
    if (y < 0) y = 0;

    gen_ovl_fill_rect(dst, stride, screen_h, x, y, w, h, bg);
    gen_ovl_stroke_rect(dst, stride, screen_h, x, y, w, h, border);
    gen_ovl_draw_text(dst, stride, screen_h, x + 10, y + 8, "TRAINER", gold, 1);

    for (int i = 0; i < s_count; i++) {
        int iy = y + 26 + i * 18;
        int on = (s_mask >> i) & 1u;
        if (i == s_selected)
            gen_ovl_draw_text(dst, stride, screen_h, x + 8, iy, ">", gold, 1);
        gen_ovl_draw_text(dst, stride, screen_h, x + 18, iy,
                          s_labels[i], i == s_selected ? gold : white, 1);
        gen_ovl_draw_text(dst, stride, screen_h, x + w - 50, iy,
                          on ? "(ON)" : "(OFF)", on ? gold : white, 1);
    }

    if (s_choice_count > 0) {
        int iy = y + 26 + s_count * 18;
        int sel = (s_selected == s_count);
        /* Right-align the value block so long fruit names never run into
         * the arrows or the border. */
        int vw = (int)strlen(s_choice_values[s_choice]) * 8;
        int vx = x + w - 20 - vw;
        if (sel)
            gen_ovl_draw_text(dst, stride, screen_h, x + 8, iy, ">", gold, 1);
        gen_ovl_draw_text(dst, stride, screen_h, x + 18, iy,
                          s_choice_title, sel ? gold : white, 1);
        gen_ovl_draw_text(dst, stride, screen_h, vx - 16, iy,
                          "<", sel ? gold : white, 1);
        gen_ovl_draw_text(dst, stride, screen_h, vx, iy,
                          s_choice_values[s_choice], sel ? gold : white, 1);
        gen_ovl_draw_text(dst, stride, screen_h, vx + vw + 8, iy,
                          ">", sel ? gold : white, 1);
    }
}
