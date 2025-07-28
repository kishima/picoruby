#ifndef PICORUBY_M5UNIFIED_H_
#define PICORUBY_M5UNIFIED_H_

#include <mrubyc.h>

#ifdef __cplusplus
extern "C" {
#endif

void mrb_picoruby_m5unified_gem_init(struct VM *vm);

// M5 class methods
void mrb_m5_begin(struct VM *vm, mrb_value *v, int argc);
void mrb_m5_update(struct VM *vm, mrb_value *v, int argc);

// Display methods
void mrb_m5_display_clear(struct VM *vm, mrb_value *v, int argc);
void mrb_m5_display_print(struct VM *vm, mrb_value *v, int argc);
void mrb_m5_display_println(struct VM *vm, mrb_value *v, int argc);
void mrb_m5_display_draw_pixel(struct VM *vm, mrb_value *v, int argc);
void mrb_m5_display_fill_screen(struct VM *vm, mrb_value *v, int argc);

// Button methods
void mrb_m5_btn_a_is_pressed(struct VM *vm, mrb_value *v, int argc);
void mrb_m5_btn_b_is_pressed(struct VM *vm, mrb_value *v, int argc);
void mrb_m5_btn_c_is_pressed(struct VM *vm, mrb_value *v, int argc);

#ifdef __cplusplus
}
#endif

#endif /* PICORUBY_M5UNIFIED_H_ */