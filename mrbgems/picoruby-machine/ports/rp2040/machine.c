#include "../../include/hal.h"

//#if !defined(PICO_RP2350)
#if 0
  #include "pico/sleep.h"
  #include "hardware/rosc.h"
#endif

#include "pico/stdlib.h"
#include "pico/unique_id.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "hardware/clocks.h"
#include "hardware/structs/scb.h"
#include "hardware/sync.h"
#include "pico/util/datetime.h"
#include <tusb.h>

#include "pico/aon_timer.h"

#ifndef PICO_MYPC 
#define PICO_MYPC
#endif

#if defined(PICO_MYPC)
#include <stdio.h>
#include "hardware/i2c.h"
#include "pico/binary_info.h"
#include "lcdspi/lcdspi.h"

#define CARDKB_ADDR (0x5F)  

void init_keyboard() {
    i2c_init(i2c_default, 100 * 1000);
    
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);

    gpio_disable_pulls(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_disable_pulls(PICO_DEFAULT_I2C_SCL_PIN);
    
    bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));

    printf("I2C initialized at 100kHz on GPIO%d(SDA), GPIO%d(SCL)\n", PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN);
}

#define FONT_HEIGHT 12
#define TERM_COLS 60  // 480 / 8 = 60文字
#define TERM_ROWS 26  // 320 / 12 = 26.6行

static short term_cursor_x = 0;
static short term_cursor_y = 0;
static int term_fg_color = WHITE;
static int term_bg_color = BLACK;

extern void scroll_lcd_spi(int lines);
extern void draw_rect_spi(int x1, int y1, int x2, int y2, int c);

typedef enum {
    ESCAPE_NONE,
    ESCAPE_START,
    ESCAPE_CSI
} escape_state_t;

static escape_state_t escape_state = ESCAPE_NONE;
static char escape_buffer[64];
static int escape_pos = 0;

// Response buffer for escape sequence responses
static char response_buffer[64];
static int response_pos = 0;
static int response_len = 0;

void send_response(const char *response) {
    int len = strlen(response);
    if (len < sizeof(response_buffer)) {
        strcpy(response_buffer, response);
        response_pos = 0;
        response_len = len;
        printf("[ESC] Queued response (len=%d): ", len);
        for (int i = 0; i < len; i++) {
            printf("0x%02X ", (unsigned char)response[i]);
        }
        printf("\n");
    }
}

void term_init(){
    set_line_pos(term_cursor_x, term_cursor_y);
    escape_state = ESCAPE_NONE;
    escape_pos = 0;
    response_pos = 0;
    response_len = 0;
    memset(escape_buffer, 0, sizeof(escape_buffer));
    memset(response_buffer, 0, sizeof(response_buffer));
}

void term_scroll_up() {
    printf("[SCROLL] Scrolling up one line\n");
    scroll_lcd_spi(FONT_HEIGHT);
    term_cursor_y = TERM_ROWS - 1;
    set_line_pos(term_cursor_x, term_cursor_y);
}

void term_newline() {
    term_cursor_x = 0;
    if (term_cursor_y >= TERM_ROWS - 1) {  // 最下行に達したらスクロール
        term_scroll_up();
    } else {
        term_cursor_y++;
    }
    set_line_pos(term_cursor_x, term_cursor_y);
}

void term_move_cursor(int x, int y) {
    if (x >= 0 && x < TERM_COLS) term_cursor_x = x;
    if (y >= 0 && y < TERM_ROWS) term_cursor_y = y;
    set_line_pos(term_cursor_x, term_cursor_y);
}

void term_process_csi_sequence() {
    escape_buffer[escape_pos] = '\0';
    char *ptr = escape_buffer;
    
    if (strcmp(ptr, "2J") == 0) {
        printf("[ESC] Screen clear (2J)\n");
        lcd_clear();
        term_cursor_x = 0;
        term_cursor_y = 0;
        set_line_pos(term_cursor_x, term_cursor_y);
        return;
    }
    
    if (ptr[escape_pos-1] == 'H') {
        printf("[ESC] Cursor position (%s)\n", ptr);
        int row = 1, col = 1;
        if (escape_pos > 1) {
            sscanf(ptr, "%d;%d", &row, &col);
        }
        term_move_cursor(col - 1, row - 1);
        return;
    }
    
    if (ptr[escape_pos-1] == 'A' || ptr[escape_pos-1] == 'B' || 
        ptr[escape_pos-1] == 'C' || ptr[escape_pos-1] == 'D') {
        printf("[ESC] Cursor move (%s)\n", ptr);
        int n = 1;
        if (escape_pos > 1) {
            n = atoi(ptr);
            if (n == 0) n = 1;
        }
        
        switch (ptr[escape_pos-1]) {
            case 'A': term_cursor_y = (term_cursor_y - n < 0) ? 0 : term_cursor_y - n; break;
            case 'B': term_cursor_y = (term_cursor_y + n >= TERM_ROWS) ? TERM_ROWS-1 : term_cursor_y + n; break;
            case 'C': term_cursor_x = (term_cursor_x + n >= TERM_COLS) ? TERM_COLS-1 : term_cursor_x + n; break;
            case 'D': term_cursor_x = (term_cursor_x - n < 0) ? 0 : term_cursor_x - n; break;
        }
        set_line_pos(term_cursor_x, term_cursor_y);
        return;
    }
    
    if (ptr[escape_pos-1] == 'K') {
        printf("[ESC] Erase line (%s)\n", ptr);
        if (ptr[0] == '0' || escape_pos == 1) {
            // Clear from cursor to end of line
            int start_x = term_cursor_x * 8;  // Assuming 8-pixel wide font
            int start_y = term_cursor_y * 16; // Assuming 16-pixel high font
            int end_x = (TERM_COLS - 1) * 8;
            int end_y = start_y + 15;
            draw_rect_spi(start_x, start_y, end_x, end_y, term_bg_color);
        }
        return;
    }
    
    if (ptr[escape_pos-1] == 'J') {
        printf("[ESC] Erase screen (%s)\n", ptr);
        if (ptr[0] == '0' || escape_pos == 1) {
            // Clear from cursor to end of screen
            int start_x = term_cursor_x * 8;
            int start_y = term_cursor_y * 16;
            draw_rect_spi(start_x, start_y, LCD_WIDTH-1, LCD_HEIGHT-1, term_bg_color);
        }
        return;
    }
    
    if (ptr[escape_pos-1] == 'h' || ptr[escape_pos-1] == 'l') {
        printf("[ESC] Mode setting (%s)\n", ptr);
        if (strstr(ptr, "?25") != NULL) {
            if (ptr[escape_pos-1] == 'h') {
                printf("[ESC] Show cursor (?25h)\n");
            } else {
                printf("[ESC] Hide cursor (?25l)\n");
            }
        }
        return;
    }
    
    if (ptr[escape_pos-1] == 'n') {
        printf("[ESC] Device status report (%s)\n", ptr);
        if (strcmp(ptr, "5") == 0) {
            // Device status report - terminal OK
            printf("[ESC] Processing device status request\n");
            send_response("\x1B[0n");
        } else if (strcmp(ptr, "6") == 0) {
            // Cursor position report
            char response[16];
            snprintf(response, sizeof(response), "\x1B[%d;%dR", 
                    term_cursor_y + 1, term_cursor_x + 1);
            printf("[ESC] Processing cursor position request\n");
            send_response(response);
        }
        return;
    }
    
    if (ptr[escape_pos-1] == 'F') {
        printf("[ESC] Move to previous line start (%s)\n", ptr);
        int n = 1;
        if (escape_pos > 1) {
            n = atoi(ptr);
            if (n == 0) n = 1;
        }
        term_cursor_x = 0;
        term_cursor_y = (term_cursor_y - n < 0) ? 0 : term_cursor_y - n;
        set_line_pos(term_cursor_x, term_cursor_y);
        return;
    }
    
    if (ptr[escape_pos-1] == 'G') {
        printf("[ESC] Cursor horizontal absolute (%s)\n", ptr);
        int col = 1;
        if (escape_pos > 1) {
            col = atoi(ptr);
            if (col == 0) col = 1;
        }
        term_cursor_x = col - 1;
        if (term_cursor_x >= TERM_COLS) term_cursor_x = TERM_COLS - 1;
        set_line_pos(term_cursor_x, term_cursor_y);
        return;
    }
    
    if (ptr[escape_pos-1] == 'm') {
        printf("[ESC] Set graphics mode (%s)\n", ptr);
        char *token = ptr;
        while (*token) {
            int code = atoi(token);
            
            switch (code) {
                case 0:
                    term_fg_color = GREEN;
                    term_bg_color = BLACK;
                    break;
                case 1:
                    break;
                case 30: term_fg_color = BLACK; break;
                case 31: term_fg_color = RED; break;
                case 32: term_fg_color = GREEN; break;
                case 33: term_fg_color = YELLOW; break;
                case 34: term_fg_color = BLUE; break;
                case 35: term_fg_color = MAGENTA; break;
                case 36: term_fg_color = CYAN; break;
                case 37: term_fg_color = WHITE; break;
                case 40: term_bg_color = BLACK; break;
                case 41: term_bg_color = RED; break;
                case 42: term_bg_color = GREEN; break;
                case 43: term_bg_color = YELLOW; break;
                case 44: term_bg_color = BLUE; break;
                case 45: term_bg_color = MAGENTA; break;
                case 46: term_bg_color = CYAN; break;
                case 47: term_bg_color = WHITE; break;
            }
            
            while (*token && *token != ';') token++;
            if (*token == ';') token++;
            else break;
        }
        return;
    }
    
    // Handle device attribute requests
    if (ptr[escape_pos-1] == 'c') {
        printf("[ESC] Device attributes request (%s)\n", ptr);
        if (strcmp(ptr, "") == 0 || strcmp(ptr, "0") == 0) {
            // Primary device attributes - VT100 compatible
            send_response("\x1B[?1;0c");
        }
        return;
    }
    
    // Log unsupported escape sequences
    printf("[ESC] Unsupported sequence: [%s\n", ptr);
}

void term_process_escape_sequence(char c) {
    if (escape_state == ESCAPE_START) {
        if (c == '[') {
            escape_state = ESCAPE_CSI;
            escape_pos = 0;
            return;
        } else {
            printf("[ESC] Unsupported escape: ESC %c\n", c);
            escape_state = ESCAPE_NONE;
            return;
        }
    }
    
    if (escape_state == ESCAPE_CSI) {
        if (escape_pos < sizeof(escape_buffer) - 1) {
            escape_buffer[escape_pos++] = c;
        }
        
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
            term_process_csi_sequence();
            escape_state = ESCAPE_NONE;
            escape_pos = 0;
        } else if (escape_pos >= sizeof(escape_buffer) - 1) {
            escape_state = ESCAPE_NONE;
            escape_pos = 0;
        }
    }
}

void term_putchar(char c) {
    
    if (escape_state != ESCAPE_NONE) {
        term_process_escape_sequence(c);
        return;
    }
    
    if (c == 0x1B) {
        escape_state = ESCAPE_START;
        escape_pos = 0;
        return;
    }
    
    switch (c) {
        case '\n':
            term_newline();
            break;
        case '\r':
            term_cursor_x = 0;
            set_line_pos(term_cursor_x, term_cursor_y);
            break;
        case '\t':
            term_cursor_x = (term_cursor_x + 4) & ~3;
            if (term_cursor_x >= TERM_COLS) {
                term_newline();
            }
            set_line_pos(term_cursor_x, term_cursor_y);
            break;
        case '\b':
            if (term_cursor_x > 0) {
                term_cursor_x--;
            }
            set_line_pos(term_cursor_x, term_cursor_y);
            break;
        default:
            if (c >= 32 && c <= 126) {
                if (term_cursor_x >= TERM_COLS) {
                    term_newline();
                }
                
                lcd_put_char(c, 0);
                //term_cursor_x++;
                get_line_pos(&term_cursor_x, &term_cursor_y);
            }
            break;
    }
}

void term_input(const void *buf, int nbytes){
    const char *str = (const char *)buf;

    short x,y;
    get_line_pos(&x, &y);
    printf("--------------- current=(%d,%d), term_cursor=(%d,%d)\n",x,y,term_cursor_x,term_cursor_y);
    // printf("term_input: buf=");
    // for (int i = 0; i < nbytes; i++) {
    //     printf("0x%02X ", (unsigned char)str[i]);
    // }
    printf(" (len=%d) [", nbytes);
    for (int i = 0; i < nbytes; i++) {
        char c = str[i];
        if (c >= 32 && c <= 126) {
            printf("%c", c);
        } else {
            printf("\\x%02X", (unsigned char)c);
        }
    }
    printf("]\n");
    
    for (int i = 0; i < nbytes; i++) {
        term_putchar(str[i]);
    }
}

void init_lcd(){
    lcd_init();
    lcd_clear();
    term_init();
}


#endif

/*-------------------------------------
 *
 * HAL
 *
 *------------------------------------*/

#ifdef MRBC_NO_TIMER
  #error "MRBC_NO_TIMER is not supported"
#endif

#define ALARM_NUM 0
#define ALARM_IRQ timer_hardware_alarm_get_irq_num(timer_hw, ALARM_NUM)

#ifndef MRBC_TICK_UNIT
#define MRBC_TICK_UNIT 1
#endif
#define US_PER_MS (MRBC_TICK_UNIT * 1000)

static volatile uint32_t interrupt_nesting = 0;

static volatile bool in_tick_processing = false;

#if defined(PICORB_VM_MRUBY)
static mrb_state *mrb_;
#endif

static void
alarm_handler(void)
{
  if (in_tick_processing) {
    timer_hw->alarm[ALARM_NUM] = timer_hw->timerawl + US_PER_MS;
    hw_clear_bits(&timer_hw->intr, 1u << ALARM_NUM);
    return;
  }

  in_tick_processing = true;
  __dmb();

  uint32_t current_time = timer_hw->timerawl;
  uint32_t next_time = current_time + US_PER_MS;
  timer_hw->alarm[ALARM_NUM] = next_time;
  hw_clear_bits(&timer_hw->intr, 1u << ALARM_NUM);

#if defined(PICORB_VM_MRUBY)
  mrb_tick(mrb_);
#elif defined(PICORB_VM_MRUBYC)
  mrbc_tick();
#else
#error "One of PICORB_VM_MRUBY or PICORB_VM_MRUBYC must be defined"
#endif

  __dmb();
  in_tick_processing = false;
}

static void
usb_irq_handler(void)
{
  tud_task();
}

void
#if defined(PICORB_VM_MRUBY)
hal_init(mrb_state *mrb)
#elif defined(PICORB_VM_MRUBYC)
hal_init(void)
#endif
{
#if defined(PICORB_VM_MRUBY)
  mrb_ = (mrb_state *)mrb;
#endif
  hw_set_bits(&timer_hw->inte, 1u << ALARM_NUM);
  irq_set_exclusive_handler(ALARM_IRQ, alarm_handler);
  irq_set_enabled(ALARM_IRQ, true);
  irq_set_priority(ALARM_IRQ, PICO_HIGHEST_IRQ_PRIORITY);
  timer_hw->alarm[ALARM_NUM] = timer_hw->timerawl + US_PER_MS;

  clocks_hw->sleep_en0 = 0;
#if defined(PICO_RP2040)
  clocks_hw->sleep_en1 =
  CLOCKS_SLEEP_EN1_CLK_SYS_TIMER_BITS
  | CLOCKS_SLEEP_EN1_CLK_SYS_USBCTRL_BITS
  | CLOCKS_SLEEP_EN1_CLK_USB_USBCTRL_BITS
  | CLOCKS_SLEEP_EN1_CLK_SYS_UART0_BITS
  | CLOCKS_SLEEP_EN1_CLK_PERI_UART0_BITS
  | CLOCKS_SLEEP_EN1_CLK_SYS_UART1_BITS
  | CLOCKS_SLEEP_EN1_CLK_PERI_UART1_BITS;
#elif defined(PICO_RP2350)
  clocks_hw->sleep_en1 =
  CLOCKS_SLEEP_EN1_CLK_SYS_TIMER0_BITS
  | CLOCKS_SLEEP_EN1_CLK_SYS_TIMER1_BITS
  | CLOCKS_SLEEP_EN1_CLK_SYS_USBCTRL_BITS
  | CLOCKS_SLEEP_EN1_CLK_USB_BITS
  | CLOCKS_SLEEP_EN1_CLK_SYS_UART0_BITS
  | CLOCKS_SLEEP_EN1_CLK_PERI_UART0_BITS
  | CLOCKS_SLEEP_EN1_CLK_SYS_UART1_BITS
  | CLOCKS_SLEEP_EN1_CLK_PERI_UART1_BITS;
#if defined(PICO_MYPC)
  init_lcd();
  init_keyboard();
#endif
#else
  #error "Unsupported Board"
#endif

  tud_init(TUD_OPT_RHPORT);
  irq_add_shared_handler(USBCTRL_IRQ, usb_irq_handler,
      PICO_SHARED_IRQ_HANDLER_LOWEST_ORDER_PRIORITY);
}

void
hal_enable_irq()
{

  if (interrupt_nesting == 0) {
//    return; // wrong state???
  }
  interrupt_nesting--;
  if (interrupt_nesting > 0) {
    return;
  }
  __dmb();
  asm volatile ("cpsie i" : : : "memory");
}

void
hal_disable_irq()
{
  asm volatile ("cpsid i" : : : "memory");
  __dmb();
  interrupt_nesting++;
}

void
hal_idle_cpu()
{
#if defined(PICO_RP2040)
  __wfi();
#elif defined(PICO_RP2350)
  /*
   * TODO: Fix this for RP2350
   *       Why does `__wfi` not wake up?
   */
  asm volatile (
    "wfe\n"           // Wait for Event
    "nop\n"           // No Operation
    "sev\n"           // Set Event
    : : : "memory"    // clobber
  );
#endif
}

int
hal_write(int fd, const void *buf, int nbytes)
{
  int len = nbytes;
#if defined(PICO_MYPC)
  term_input(buf, nbytes);
#else
  tud_cdc_write(buf, nbytes);
  len = tud_cdc_write_flush();
#endif
  return len;
}

int hal_flush(int fd) {
  int len = tud_cdc_write_flush();
  return len;
}

int
hal_read_available(void)
{
  int len = tud_cdc_available();
  if (0 < len) {
    return 1;
  } else {
    return 0;
  }
}

int
hal_getchar(void)
{
  int c = -1;

#if defined(PICO_MYPC)  
  // First check if we have a response to send
  if (response_pos < response_len) {
    c = (unsigned char)response_buffer[response_pos++];
    printf("[ESC] Sending response char: 0x%02X\n", c);
    if (response_pos >= response_len) {
      response_pos = 0;
      response_len = 0;
    }
    return c;
  }
  
  uint8_t data;
  int result = i2c_read_blocking(i2c_default, CARDKB_ADDR, &data, 1, false);
  if (result == 1 && data != 0) {
      printf("0x%02X\n", data);
      c = (int)data;
  }
#else
  int len = tud_cdc_available();
  if (0 < len) {
    c = tud_cdc_read_char();
  }
#endif

  return c;
}

void
hal_abort(const char *s)
{
  if( s ) {
    hal_write(1, s, strlen(s));
  }

  abort();
}


/*-------------------------------------
 *
 * USB
 *
 *------------------------------------*/

void
Machine_tud_task(void)
{
  tud_task();
}

bool
Machine_tud_mounted_q(void)
{
  return tud_mounted();
}


/*-------------------------------------
 *
 * RTC
 *
 *------------------------------------*/

/*
 * References:
 *   https://github.com/ghubcoder/PicoSleepDemo
 *   https://ghubcoder.github.io/posts/awaking-the-pico/
 */

/*
 * Current implementation can not restore USB-CDC.
 * This article might help:
 *   https://elehobica.blogspot.com/2021/08/raspberry-pi-pico-2.html
 */

#define YEAR  2020
#define MONTH    6
#define DAY      5
#define DOTW     5

//#if !defined(PICO_RP2350)
#if 0
  static const uint32_t PIN_DCDC_PSM_CTRL = 23;
  static uint32_t _scr;
  static uint32_t _sleep_en0;
  static uint32_t _sleep_en1;
#endif

/*
 * deep_sleep doesn't work yet
 */
void
Machine_deep_sleep(uint8_t gpio_pin, bool edge, bool high)
{
//#if !defined(PICO_RP2350)
#if 0
  bool psm = gpio_get(PIN_DCDC_PSM_CTRL);
  gpio_put(PIN_DCDC_PSM_CTRL, 0); // PFM mode for better efficiency
  uint32_t ints = save_and_disable_interrupts();
  {
    // preserve_clock_before_sleep
    _scr = scb_hw->scr;
    _sleep_en0 = clocks_hw->sleep_en0;
    _sleep_en1 = clocks_hw->sleep_en1;
  }
  sleep_run_from_xosc();
  sleep_goto_dormant_until_pin(gpio_pin, edge, high);
  {
    // recover_clock_after_sleep
    rosc_write(&rosc_hw->ctrl, ROSC_CTRL_ENABLE_BITS); //Re-enable ring Oscillator control
    scb_hw->scr = _scr;
    clocks_hw->sleep_en0 = _sleep_en0;
    clocks_hw->sleep_en1 = _sleep_en1;
    //clocks_init();
    set_sys_clock_khz(125000, true);
  }
  restore_interrupts(ints);
  gpio_put(PIN_DCDC_PSM_CTRL, psm); // recover PWM mode
#endif
}

static void
sleep_callback(void)
{
  return;
}

static void
rtc_sleep(uint32_t seconds)
{
//#if !defined(PICO_RP2350)
#if 0
  datetime_t t;
  rtc_get_datetime(&t);
  t.sec += seconds;
  if (t.sec >= 60) {
    t.min += t.sec / 60;
    t.sec %= 60;
  }
  if (t.min >= 60) {
    t.hour += t.min / 60;
    t.min %= 60;
  }
  sleep_goto_sleep_until(&t, &sleep_callback);
#endif
}


static void
recover_from_sleep(uint scb_orig, uint clock0_orig, uint clock1_orig)
{
//#if !defined(PICO_RP2350)
#if 0
  //Re-enable ring Oscillator control
  rosc_write(&rosc_hw->ctrl, ROSC_CTRL_ENABLE_BITS);
  //reset procs back to default
  scb_hw->scr = scb_orig;
  clocks_hw->sleep_en0 = clock0_orig;
  clocks_hw->sleep_en1 = clock1_orig;
  //reset clocks
  set_sys_clock_khz(125000, true);
  // Re-initialize peripherals
  stdio_init_all();
#endif
}

void
Machine_sleep(uint32_t seconds)
{
//#if !defined(PICO_RP2350)
#if 0
  // save values for later
  uint scb_orig = scb_hw->scr;
  uint clock0_orig = clocks_hw->sleep_en0;
  uint clock1_orig = clocks_hw->sleep_en1;

  // Start the Real time clock
  rtc_init();
  sleep_run_from_xosc();

  // Get current time and set alarm
  datetime_t t;
  rtc_get_datetime(&t);

  rtc_sleep(seconds);

  // reset processor and clocks back to defaults
  recover_from_sleep(scb_orig, clock0_orig, clock1_orig);

  // システムクロックをリセットし、デフォルト設定に戻す
  clock_configure(clk_sys,
                  CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX,
                  CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_XOSC_CLKSRC,
                  12 * MHZ,  // XOSC frequency
                  12 * MHZ);

  // PLLを使用して通常の動作周波数に戻す
  set_sys_clock_khz(133000, true);  // 133 MHz、デフォルトの周波数

  // Re-enable interrupts
  irq_set_enabled(RTC_IRQ, true);
#endif
}

void
Machine_delay_ms(uint32_t ms)
{
  sleep_ms(ms);
}

void
Machine_busy_wait_ms(uint32_t ms)
{
  busy_wait_us_32(1000 * ms);
}

int
Machine_get_unique_id(char *id_str)
{
  pico_get_unique_board_id_string(id_str, PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2 + 1);
  return 1;
}

uint32_t
Machine_stack_usage(void)
{
  // TODO
  return 0;
}

bool
Machine_set_hwclock(const struct timespec *ts)
{
  if (aon_timer_is_running()) {
    return aon_timer_set_time(ts);
  }
  return aon_timer_start(ts);
}

bool
Machine_get_hwclock(struct timespec *ts)
{
  return aon_timer_get_time(ts);
}
