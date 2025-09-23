/*
 * picoruby-psg/src/mrubyc/psg.c
 */

#include <mrubyc.h>

static void
c_driver_send_reg(mrbc_vm *vm, mrbc_value *v, int argc)
{
  if (argc < 2 || argc > 3) {
    mrbc_raise(vm, MRBC_CLASS(ArgumentError), "wrong number of arguments");
    return;
  }

  if (mrbc_type(v[1]) != MRBC_TT_INTEGER || mrbc_type(v[2]) != MRBC_TT_INTEGER) {
    mrbc_raise(vm, MRBC_CLASS(ArgumentError), "arguments must be integers");
    return;
  }

  mrbc_int_t reg = mrbc_integer(v[1]);
  mrbc_int_t val = mrbc_integer(v[2]);
  mrbc_int_t tick_delay = 0;
  
  if (argc == 3) {
    if (mrbc_type(v[3]) != MRBC_TT_INTEGER) {
      mrbc_raise(vm, MRBC_CLASS(ArgumentError), "tick_delay must be integer");
      return;
    }
    tick_delay = mrbc_integer(v[3]);
  }

  psg_packet_t p = {
    .tick = (uint32_t)tick_delay,
    .op   = PSG_PKT_REG_WRITE,
    .reg  = (uint8_t)reg,
    .val  = (uint8_t)val,
  };
  
  if (PSG_rb_push(&p)) {
    SET_TRUE_RETURN();
  } else {
    SET_FALSE_RETURN();
  }
}

static void
reset_psg_c(mrbc_vm *vm)
{
  D("PSG: Resetting...\n");
  if (rb.buf) {
    mrbc_raw_free(rb.buf);
  }
  rb.buf = mrbc_raw_alloc(sizeof(psg_packet_t) * PSG_PACKET_QUEUE_LEN);
  rb.head = 0;
  rb.tail = 0;
  psg_cs_token_t t = PSG_enter_critical();
  memset(&psg_, 0, sizeof(psg_));
  psg_.r.volume[0] = psg_.r.volume[1] = psg_.r.volume[2] = 15; // max volume. no envelope
  psg_.r.mixer = 0x38; // all noise off, all tone on
  psg_.mute_mask = 0x07; // all tracks muted. The driver must unmute first!
  psg_.pan[0] = psg_.pan[1] = psg_.pan[2] = 8; // center pan
  PSG_exit_critical(t);
}

static void
c_driver_s_select_pwm(mrbc_vm *vm, mrbc_value *v, int argc)
{
  if (argc != 2) {
    mrbc_raise(vm, MRBC_CLASS(ArgumentError), "wrong number of arguments");
    return;
  }

  if (mrbc_type(v[1]) != MRBC_TT_INTEGER || mrbc_type(v[2]) != MRBC_TT_INTEGER) {
    mrbc_raise(vm, MRBC_CLASS(ArgumentError), "arguments must be integers");
    return;
  }

  psg_drv = &psg_drv_pwm;
  mrbc_int_t left = mrbc_integer(v[1]);
  mrbc_int_t right = mrbc_integer(v[2]);
  mrbc_printf("PSG: PWM left=%d, right=%d\n", left, right);
  reset_psg_c(vm);
  PSG_tick_start_core1((uint8_t)left, (uint8_t)right);
  SET_NIL_RETURN();
}

static void
c_driver_s_select_mcp4922(mrbc_vm *vm, mrbc_value *v, int argc)
{
  if (argc != 1) {
    mrbc_raise(vm, MRBC_CLASS(ArgumentError), "wrong number of arguments");
    return;
  }

  if (mrbc_type(v[1]) != MRBC_TT_INTEGER) {
    mrbc_raise(vm, MRBC_CLASS(ArgumentError), "argument must be integer");
    return;
  }

  mrbc_int_t ldac = mrbc_integer(v[1]);
  psg_drv = &psg_drv_mcp4922;
  reset_psg_c(vm);
  PSG_tick_start_core1((uint8_t)ldac, 0);
  SET_NIL_RETURN();
}

static void
c_driver_buffer_empty_p(mrbc_vm *vm, mrbc_value *v, int argc)
{
  if (rb.head == rb.tail) {
    SET_TRUE_RETURN();
  } else {
    SET_FALSE_RETURN();
  }
}

static void
c_driver_deinit(mrbc_vm *vm, mrbc_value *v, int argc)
{
  PSG_tick_stop_core1();
  if (rb.buf) {
    mrbc_raw_free(rb.buf);
    rb.buf = NULL;
  }
  SET_NIL_RETURN();
}

static void
c_driver_set_lfo(mrbc_vm *vm, mrbc_value *v, int argc)
{
  if (argc < 3 || argc > 4) {
    mrbc_raise(vm, MRBC_CLASS(ArgumentError), "wrong number of arguments");
    return;
  }

  if (mrbc_type(v[1]) != MRBC_TT_INTEGER || mrbc_type(v[2]) != MRBC_TT_INTEGER || mrbc_type(v[3]) != MRBC_TT_INTEGER) {
    mrbc_raise(vm, MRBC_CLASS(ArgumentError), "arguments must be integers");
    return;
  }

  mrbc_int_t tr = mrbc_integer(v[1]);
  mrbc_int_t depth = mrbc_integer(v[2]);
  mrbc_int_t rate = mrbc_integer(v[3]);
  mrbc_int_t tick_delay = 0;
  
  if (argc == 4) {
    if (mrbc_type(v[4]) != MRBC_TT_INTEGER) {
      mrbc_raise(vm, MRBC_CLASS(ArgumentError), "tick_delay must be integer");
      return;
    }
    tick_delay = mrbc_integer(v[4]);
  }

  psg_packet_t p = {
    .tick = (uint32_t)tick_delay,
    .op   = PSG_PKT_LFO_SET,
    .reg  = (uint8_t)tr,
    .val  = (uint8_t)depth,
    .arg  = (uint8_t)rate,
  };
  
  if (PSG_rb_push(&p)) {
    SET_TRUE_RETURN();
  } else {
    SET_FALSE_RETURN();
  }
}

static void
c_driver_set_pan(mrbc_vm *vm, mrbc_value *v, int argc)
{
  if (argc < 2 || argc > 3) {
    mrbc_raise(vm, MRBC_CLASS(ArgumentError), "wrong number of arguments");
    return;
  }

  if (mrbc_type(v[1]) != MRBC_TT_INTEGER || mrbc_type(v[2]) != MRBC_TT_INTEGER) {
    mrbc_raise(vm, MRBC_CLASS(ArgumentError), "arguments must be integers");
    return;
  }

  mrbc_int_t tr = mrbc_integer(v[1]);
  mrbc_int_t pan = mrbc_integer(v[2]);
  mrbc_int_t tick_delay = 0;
  
  if (argc == 3) {
    if (mrbc_type(v[3]) != MRBC_TT_INTEGER) {
      mrbc_raise(vm, MRBC_CLASS(ArgumentError), "tick_delay must be integer");
      return;
    }
    tick_delay = mrbc_integer(v[3]);
  }

  if (tr < 0 || tr > 2 || pan < 0 || pan > 15) {
    mrbc_raise(vm, MRBC_CLASS(ArgumentError), "Invalid track or pan value");
    return;
  }

  psg_packet_t p = {
    .tick = (uint32_t)tick_delay,
    .op   = PSG_PKT_PAN_SET,
    .reg  = (uint8_t)tr,
    .val  = (uint8_t)pan,
  };
  
  if (PSG_rb_push(&p)) {
    SET_TRUE_RETURN();
  } else {
    SET_FALSE_RETURN();
  }
}

static void
c_driver_set_timbre(mrbc_vm *vm, mrbc_value *v, int argc)
{
  if (argc < 2 || argc > 3) {
    mrbc_raise(vm, MRBC_CLASS(ArgumentError), "wrong number of arguments");
    return;
  }

  if (mrbc_type(v[1]) != MRBC_TT_INTEGER || mrbc_type(v[2]) != MRBC_TT_INTEGER) {
    mrbc_raise(vm, MRBC_CLASS(ArgumentError), "arguments must be integers");
    return;
  }

  mrbc_int_t tr = mrbc_integer(v[1]);
  mrbc_int_t timbre = mrbc_integer(v[2]);
  mrbc_int_t tick_delay = 0;
  
  if (argc == 3) {
    if (mrbc_type(v[3]) != MRBC_TT_INTEGER) {
      mrbc_raise(vm, MRBC_CLASS(ArgumentError), "tick_delay must be integer");
      return;
    }
    tick_delay = mrbc_integer(v[3]);
  }

  if (tr < 0 || 2 < tr) {
    mrbc_raise(vm, MRBC_CLASS(ArgumentError), "Invalid track: 0-2 expected");
    return;
  }

  psg_packet_t p = {
    .tick = (uint32_t)tick_delay,
    .op   = PSG_PKT_TIMBRE_SET,
    .reg  = (uint8_t)tr,
    .val  = (uint8_t)timbre
  };
  
  if (PSG_rb_push(&p)) {
    SET_TRUE_RETURN();
  } else {
    SET_FALSE_RETURN();
  }
}

static void
c_driver_set_legato(mrbc_vm *vm, mrbc_value *v, int argc)
{
  if (argc < 2 || argc > 3) {
    mrbc_raise(vm, MRBC_CLASS(ArgumentError), "wrong number of arguments");
    return;
  }

  if (mrbc_type(v[1]) != MRBC_TT_INTEGER || mrbc_type(v[2]) != MRBC_TT_INTEGER) {
    mrbc_raise(vm, MRBC_CLASS(ArgumentError), "arguments must be integers");
    return;
  }

  mrbc_int_t tr = mrbc_integer(v[1]);
  mrbc_int_t legato = mrbc_integer(v[2]);
  mrbc_int_t tick_delay = 0;
  
  if (argc == 3) {
    if (mrbc_type(v[3]) != MRBC_TT_INTEGER) {
      mrbc_raise(vm, MRBC_CLASS(ArgumentError), "tick_delay must be integer");
      return;
    }
    tick_delay = mrbc_integer(v[3]);
  }

  if (tr < 0 || 2 < tr) {
    mrbc_raise(vm, MRBC_CLASS(ArgumentError), "Invalid track: 0-2 expected");
    return;
  }

  psg_packet_t p = {
    .tick = (uint32_t)tick_delay,
    .op   = PSG_PKT_LEGATO_SET,
    .reg  = (uint8_t)tr,
    .val  = (uint8_t)legato
  };
  
  if (PSG_rb_push(&p)) {
    SET_TRUE_RETURN();
  } else {
    SET_FALSE_RETURN();
  }
}

static void
c_driver_mute(mrbc_vm *vm, mrbc_value *v, int argc)
{
  if (argc < 2 || argc > 3) {
    mrbc_raise(vm, MRBC_CLASS(ArgumentError), "wrong number of arguments");
    return;
  }

  if (mrbc_type(v[1]) != MRBC_TT_INTEGER || mrbc_type(v[2]) != MRBC_TT_INTEGER) {
    mrbc_raise(vm, MRBC_CLASS(ArgumentError), "arguments must be integers");
    return;
  }

  mrbc_int_t tr = mrbc_integer(v[1]);
  mrbc_int_t flag = mrbc_integer(v[2]);
  mrbc_int_t tick_delay = 0;
  
  if (argc == 3) {
    if (mrbc_type(v[3]) != MRBC_TT_INTEGER) {
      mrbc_raise(vm, MRBC_CLASS(ArgumentError), "tick_delay must be integer");
      return;
    }
    tick_delay = mrbc_integer(v[3]);
  }

  psg_packet_t p = {
    .tick = (uint32_t)tick_delay,
    .op   = PSG_PKT_CH_MUTE,
    .reg  = (uint8_t)tr,
    .val  = (uint8_t)flag,
  };
  
  if (PSG_rb_push(&p)) {
    SET_TRUE_RETURN();
  } else {
    SET_FALSE_RETURN();
  }
}

void
mrbc_picoruby_psg_gem_init(void)
{
  mrbc_class *class_PSG = mrbc_define_module(0, "PSG");
  mrbc_class *class_Driver = mrbc_define_class_under(0, class_PSG, "Driver", mrbc_class_object);

  mrbc_value chip_clock = mrbc_integer_value(CHIP_CLOCK);
  mrbc_set_class_const(class_Driver, mrbc_str_to_symid("CHIP_CLOCK"), &chip_clock);
  
  mrbc_value sample_rate = mrbc_integer_value(SAMPLE_RATE);
  mrbc_set_class_const(class_Driver, mrbc_str_to_symid("SAMPLE_RATE"), &sample_rate);

  // Define TIMBRES hash
  mrbc_value timbres = mrbc_hash_new(0, 4);
  mrbc_value square_sym = mrbc_symbol_value(mrbc_str_to_symid("square"));
  mrbc_value square_val = mrbc_integer_value(PSG_TIMBRE_SQUARE);
  mrbc_hash_set(&timbres, &square_sym, &square_val);
  
  mrbc_value triangle_sym = mrbc_symbol_value(mrbc_str_to_symid("triangle"));
  mrbc_value triangle_val = mrbc_integer_value(PSG_TIMBRE_TRIANGLE);
  mrbc_hash_set(&timbres, &triangle_sym, &triangle_val);
  
  mrbc_value sawtooth_sym = mrbc_symbol_value(mrbc_str_to_symid("sawtooth"));
  mrbc_value sawtooth_val = mrbc_integer_value(PSG_TIMBRE_SAWTOOTH);
  mrbc_hash_set(&timbres, &sawtooth_sym, &sawtooth_val);
  
  mrbc_value invsawtooth_sym = mrbc_symbol_value(mrbc_str_to_symid("invsawtooth"));
  mrbc_value invsawtooth_val = mrbc_integer_value(PSG_TIMBRE_INVSAWTOOTH);
  mrbc_hash_set(&timbres, &invsawtooth_sym, &invsawtooth_val);
  
  mrbc_set_class_const(class_Driver, mrbc_str_to_symid("TIMBRES"), &timbres);

  mrbc_define_method(0, class_Driver, "select_pwm", c_driver_s_select_pwm);
  mrbc_define_method(0, class_Driver, "select_mcp4922", c_driver_s_select_mcp4922);
  mrbc_define_method(0, class_Driver, "send_reg", c_driver_send_reg);
  mrbc_define_method(0, class_Driver, "buffer_empty?", c_driver_buffer_empty_p);
  mrbc_define_method(0, class_Driver, "deinit", c_driver_deinit);
  mrbc_define_method(0, class_Driver, "set_lfo", c_driver_set_lfo);
  mrbc_define_method(0, class_Driver, "set_pan", c_driver_set_pan);
  mrbc_define_method(0, class_Driver, "set_timbre", c_driver_set_timbre);
  mrbc_define_method(0, class_Driver, "set_legato", c_driver_set_legato);
  mrbc_define_method(0, class_Driver, "mute", c_driver_mute);
}

void
mrbc_picoruby_psg_gem_final(void)
{
}

void
mrbc_psg_init(void)
{
  mrbc_picoruby_psg_gem_init();
}