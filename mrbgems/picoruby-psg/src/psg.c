#include "../include/psg.h"

#include "picoruby.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#if defined(PICORB_VM_MRUBY)

#include "mruby/psg.c"

#elif defined(PICORB_VM_MRUBYC)

#include "mrubyc/psg.c"

#endif
