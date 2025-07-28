#include "m5unified.h"

void mrbc_m5unified_init(mrbc_vm *vm)
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
  mrbc_value black_val = mrbc_fixnum_value(0x0000);
  mrbc_set_class_const(display_class, mrbc_str_to_symid("BLACK"), &black_val);
  
  mrbc_value white_val = mrbc_fixnum_value(0xFFFF);
  mrbc_set_class_const(display_class, mrbc_str_to_symid("WHITE"), &white_val);
  
  mrbc_value red_val = mrbc_fixnum_value(0xF800);
  mrbc_set_class_const(display_class, mrbc_str_to_symid("RED"), &red_val);
  
  mrbc_value green_val = mrbc_fixnum_value(0x07E0);
  mrbc_set_class_const(display_class, mrbc_str_to_symid("GREEN"), &green_val);
  
  mrbc_value blue_val = mrbc_fixnum_value(0x001F);
  mrbc_set_class_const(display_class, mrbc_str_to_symid("BLUE"), &blue_val);
  
  mrbc_value yellow_val = mrbc_fixnum_value(0xFFE0);
  mrbc_set_class_const(display_class, mrbc_str_to_symid("YELLOW"), &yellow_val);
  
  mrbc_value cyan_val = mrbc_fixnum_value(0x07FF);
  mrbc_set_class_const(display_class, mrbc_str_to_symid("CYAN"), &cyan_val);
  
  mrbc_value magenta_val = mrbc_fixnum_value(0xF81F);
  mrbc_set_class_const(display_class, mrbc_str_to_symid("MAGENTA"), &magenta_val);
}