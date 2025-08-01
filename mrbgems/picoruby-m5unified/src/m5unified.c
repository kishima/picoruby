#include "m5unified.h"


#if defined(PICORB_VM_MRUBY)

#error "m5unidied mrbgem doesn't support mruby VM"

#elif defined(PICORB_VM_MRUBYC)

#include "mrubyc/m5unified.c"

#endif
