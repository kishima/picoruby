// Include M5Unified C++ headers first
#include "M5Unified.h"

// Then include the C header with extern "C"
extern "C" {
#include "m5unified.h"
}

// Initialize M5 hardware
void mrb_m5_begin(struct VM *vm, mrb_value *v, int argc)
{
  auto cfg = M5.config();
  M5.begin(cfg);

  SET_NIL_RETURN();
}

// Update M5 state (should be called in main loop)
void mrb_m5_update(struct VM *vm, mrb_value *v, int argc)
{
  M5.update();
  SET_NIL_RETURN();
}

// Display methods
void mrb_m5_display_clear(struct VM *vm, mrb_value *v, int argc)
{
  M5.Display.clear();
  SET_NIL_RETURN();
}

void mrb_m5_display_set_text_size(struct VM *vm, mrb_value *v, int argc)
{
  if (argc != 1 || GET_TT_ARG(1) != MRBC_TT_INTEGER) {
    printf("input error");
    SET_NIL_RETURN();
    return;
  }
  int size = (int)GET_INT_ARG(1);
  //printf("setTextSize=%d",size);
  M5.Display.setTextSize(size);
  SET_NIL_RETURN();
}

void mrb_m5_display_print(struct VM *vm, mrb_value *v, int argc)
{
  if (argc != 1 || GET_TT_ARG(1) != MRBC_TT_STRING) {
    printf("input error");
    SET_NIL_RETURN();
    return;
  }
  
  const char* text = (const char*)GET_STRING_ARG(1);
  M5.Display.print(text);
  
  SET_NIL_RETURN();
}

void mrb_m5_display_println(struct VM *vm, mrb_value *v, int argc)
{
  if (argc != 1 || GET_TT_ARG(1) != MRBC_TT_STRING) {
    SET_NIL_RETURN();
    return;
  }
  
  const char* text = (const char*)GET_STRING_ARG(1);
  M5.Display.println(text);
  
  SET_NIL_RETURN();
}

void mrb_m5_display_draw_pixel(struct VM *vm, mrb_value *v, int argc)
{
  if (argc != 3 || GET_TT_ARG(1) != MRBC_TT_INTEGER || 
      GET_TT_ARG(2) != MRBC_TT_INTEGER || GET_TT_ARG(3) != MRBC_TT_INTEGER) {
    SET_NIL_RETURN();
    return;
  }
  
  int x = GET_INT_ARG(1);
  int y = GET_INT_ARG(2);
  int color = GET_INT_ARG(3);
  
  M5.Display.drawPixel(x, y, color);
  
  SET_NIL_RETURN();
}

void mrb_m5_display_fill_screen(struct VM *vm, mrb_value *v, int argc)
{
  if (argc != 1 || GET_TT_ARG(1) != MRBC_TT_INTEGER) {
    SET_NIL_RETURN();
    return;
  }
  
  int color = GET_INT_ARG(1);
  printf("fillscreen=%x\n",color);
  M5.Display.fillScreen(color);
  
  SET_NIL_RETURN();
}

// Button methods
void mrb_m5_btn_a_is_pressed(struct VM *vm, mrb_value *v, int argc)
{
  if (M5.BtnA.isPressed()) {
    SET_TRUE_RETURN();
  } else {
    SET_FALSE_RETURN();
  }
}

void mrb_m5_btn_b_is_pressed(struct VM *vm, mrb_value *v, int argc)
{
  if (M5.BtnB.isPressed()) {
    SET_TRUE_RETURN();
  } else {
    SET_FALSE_RETURN();
  }
}

void mrb_m5_btn_c_is_pressed(struct VM *vm, mrb_value *v, int argc)
{
  if (M5.BtnC.isPressed()) {
    SET_TRUE_RETURN();
  } else {
    SET_FALSE_RETURN();
  }
}

// C++ implementation file - no need for closing extern "C"