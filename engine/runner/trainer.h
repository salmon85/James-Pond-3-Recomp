#ifndef GENESIS_TRAINER_H
#define GENESIS_TRAINER_H

#include <stdint.h>

#define TRAINER_MAX_ITEMS 8

/* Opt-in trainer menu, toggled with T. The game supplies the item labels and
 * applies the enabled bitmask to its own RAM; the runner owns the menu,
 * selection and overlay. State lives outside guest RAM so toggles survive
 * level changes and save loads until the process exits. */
void     trainer_init(const char *const *labels, int count);
/* Optional single-choice row drawn after the toggles; Left/Right cycle it. */
void     trainer_set_choice(const char *title, const char *const *values,
                            int count);
int      trainer_choice(void);
int      trainer_menu_open(void);
void     trainer_menu_set_open(int open);
void     trainer_menu_move(int delta);
void     trainer_menu_adjust(int delta);
void     trainer_toggle_selected(void);
uint32_t trainer_mask(void);
void     trainer_draw(uint32_t *dst, int stride, int height,
                      int screen_w, int screen_h);

#endif
