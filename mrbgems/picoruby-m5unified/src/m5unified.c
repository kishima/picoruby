#include "m5unified.h"

void mrb_picoruby_m5unified_gem_init(struct VM *vm)
{
  // M5 main class
  mrbc_class *m5_class = mrbc_define_class(vm, "M5", mrbc_class_object);
  
  // M5 class methods
  mrbc_define_method(vm, m5_class, "begin", mrb_m5_begin);
  mrbc_define_method(vm, m5_class, "update", mrb_m5_update);

  // Display class
  mrbc_class *display_class = mrbc_define_class_under(vm, m5_class, "Display", mrbc_class_object);
  mrbc_define_method(vm, display_class, "clear", mrb_m5_display_clear);
  mrbc_define_method(vm, display_class, "print", mrb_m5_display_print);
  mrbc_define_method(vm, display_class, "println", mrb_m5_display_println);
  mrbc_define_method(vm, display_class, "draw_pixel", mrb_m5_display_draw_pixel);
  mrbc_define_method(vm, display_class, "fill_screen", mrb_m5_display_fill_screen);

  // Button classes
  mrbc_class *btn_a_class = mrbc_define_class_under(vm, m5_class, "BtnA", mrbc_class_object);
  mrbc_define_method(vm, btn_a_class, "pressed?", mrb_m5_btn_a_is_pressed);

  mrbc_class *btn_b_class = mrbc_define_class_under(vm, m5_class, "BtnB", mrbc_class_object);
  mrbc_define_method(vm, btn_b_class, "pressed?", mrb_m5_btn_b_is_pressed);

  mrbc_class *btn_c_class = mrbc_define_class_under(vm, m5_class, "BtnC", mrbc_class_object);
  mrbc_define_method(vm, btn_c_class, "pressed?", mrb_m5_btn_c_is_pressed);

  // Color constants
  mrbc_set_const(vm, display_class, "BLACK", &mrbc_fixnum_value(0x0000));
  mrbc_set_const(vm, display_class, "WHITE", &mrbc_fixnum_value(0xFFFF));
  mrbc_set_const(vm, display_class, "RED", &mrbc_fixnum_value(0xF800));
  mrbc_set_const(vm, display_class, "GREEN", &mrbc_fixnum_value(0x07E0));
  mrbc_set_const(vm, display_class, "BLUE", &mrbc_fixnum_value(0x001F));
  mrbc_set_const(vm, display_class, "YELLOW", &mrbc_fixnum_value(0xFFE0));
  mrbc_set_const(vm, display_class, "CYAN", &mrbc_fixnum_value(0x07FF));
  mrbc_set_const(vm, display_class, "MAGENTA", &mrbc_fixnum_value(0xF81F));
}