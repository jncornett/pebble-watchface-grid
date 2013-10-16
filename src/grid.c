#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

/* Defines */

#define SCREEN_X    144
#define SCREEN_Y    168

#define NUM_ROWS    5
#define NUM_COLS    9
#define OFFSET      0
#define FONT_MAIN   fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_MATRIX_24))

#define ANIMATION_TIME_MS   100

#define NUM_PERMUTATIONS    4

/* Typedefs */

typedef unsigned short ushort;
typedef unsigned long ulong;



/* Constants */

const ushort CELL_SIZE_X = SCREEN_X / NUM_COLS;
const ushort CELL_SIZE_Y = SCREEN_Y / NUM_ROWS;

const char ENCRYPTED_CHARS[12][NUM_PERMUTATIONS] = {
    {'o', '9', '@', 'C'},
    {'[', 'i', '*', '/'},
    {'S', '5', '?', '&'},
    {'E', '8', 'B', '#'},
    {'7', '^', '9', '+'},
    {'S', '2', '6', '?'},
    {'9', 'b', 'c', '<'},
    {'/', '>', '1', '9'},
    {'B', '&', '%', '$'},
    {'6', 'q', '2', '-'},
    {'^', '@', 'V', 'G'},
    {'B', '9', 'T', '='}
};

const char ACTUAL_CHARS[12] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'P'};

const ushort CHAR_A = 10;
const ushort CHAR_P = 11;

const ushort HR_1 = OFFSET + 0, HR_2 = OFFSET + 1;
const ushort M_1 = OFFSET + 2, M_2 = OFFSET + 3, AP = OFFSET + 4;

/* Globals */

Layer layer;
static ushort tail_sizes[5];
static ushort digit_values[5];
static ushort step;

/* Macros */

#define CELL_RECT(X, Y) \
    GRect(X * CELL_SIZE_X, Y * CELL_SIZE_Y, CELL_SIZE_X, CELL_SIZE_Y)

#define GET_RAND(array, size) \
    ( array[rand() % size] )

#define RAND_NUM(minN, maxN) \
    ( (rand() % (maxN - minN)) + minN )

/* Static Headers */

static void setup_layers(AppContextRef ctx);
static void grid_set_cell_char(GContext* ctx, GColor color, char c, ushort x, ushort y);
static void grid_set_cell_background(GContext* ctx, GColor color, ushort x, ushort y);


/* Inlines */

inline ushort min(ushort a, ushort b) {
    return (a < b)? a: b;
}

/* Pebble API Code */

#define MY_UUID { 0xE1, 0x18, 0xE0, 0x76, 0x8B, 0x41, 0x46, 0xD7, 0x9A, 0x8F, 0x16, 0xD1, 0x35, 0x00, 0x23, 0x6E }
PBL_APP_INFO(MY_UUID,
             "Grid", "Joel",
             1, 0, /* App version */
             DEFAULT_MENU_ICON,
             APP_INFO_WATCH_FACE);

Window window;

void layer_update_callback(Layer *me, GContext *ctx) {
    volatile ushort i, j;
    ushort tail_size;
    ushort digit_value;
    ushort col_max;
    
    for (i = HR_1; i <= AP; i++) {
        tail_size = tail_sizes[i - OFFSET];
        digit_value = digit_values[i - OFFSET];
        col_max = min(tail_size, step + 1);
        for (j = 0; j < col_max; j++) {
            if (j < tail_size - 1) {
                //grid_set_cell_char(ctx, GColorBlack, GET_RAND(ENCRYPTED_CHARS[digit_value], NUM_PERMUTATIONS), j, i);
                grid_set_cell_char(ctx, GColorBlack, (char) RAND_NUM(33, 127), j, i);
            } else if (j == tail_size - 1) {
                grid_set_cell_background(ctx, GColorBlack, j, i);
                grid_set_cell_char(ctx, GColorWhite, ACTUAL_CHARS[digit_value], j, i);
            }
        }
    }
}

void handle_init(AppContextRef ctx) {

  window_init(&window, "Window Name");
  window_stack_push(&window, true /* Animated */);
    window_set_background_color(&window, GColorWhite);
    
    resource_init_current_app(&APP_RESOURCES);
    step = 0;
    
    setup_layers(ctx);
    layer.update_proc = &layer_update_callback;
    
}

void handle_minute_tick(AppContextRef ctx, PebbleTickEvent *t) {
    ushort i;
    PblTm ptm;
    get_time(&ptm);
    
    for (i = HR_1; i <= AP; i++) {
        tail_sizes[i - HR_1] = RAND_NUM(2, NUM_COLS);
    }
    
    if ((ptm.tm_hour == 0) || (ptm.tm_hour == 12)) {
        digit_values[HR_1 - OFFSET] = 1;
        digit_values[HR_2 - OFFSET] = 2;
    } else {
        digit_values[HR_1 - OFFSET] = (ptm.tm_hour % 12) / 10;
        digit_values[HR_2 - OFFSET] = (ptm.tm_hour % 12) % 10;
    }
    
    digit_values[M_1 - OFFSET] = ptm.tm_min / 10;
    digit_values[M_2 - OFFSET] = ptm.tm_min % 10;
    digit_values[AP - OFFSET] = (ptm.tm_hour > 11)? CHAR_P: CHAR_A;
    
    step = 0;
    layer_mark_dirty(&layer);
    app_timer_send_event(ctx, ANIMATION_TIME_MS, 0);
}

void handle_timer(AppContextRef ctx, AppTimerHandle handle, uint32_t cookie) {
    if (step < NUM_COLS - 1) {
        step++;
        layer_mark_dirty(&layer);
    }
    
    if (step < NUM_COLS - 1) {
        app_timer_send_event(ctx, ANIMATION_TIME_MS, 0);
    }
}


void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
      .timer_handler = &handle_timer,
      .tick_info = {
          .tick_handler = &handle_minute_tick,
          .tick_units = MINUTE_UNIT
      }
  };
  app_event_loop(params, &handlers);
}



/* Static Definitions */

static void setup_layers(AppContextRef ctx) {
    layer_init(&layer, window.layer.frame);
    layer_set_bounds(&layer, GRect(0, 0, SCREEN_X, SCREEN_Y));
    layer_add_child(&window.layer, &layer);
}

static void grid_set_cell_char(GContext* ctx, GColor color, char c, ushort x, ushort y) {
    char buffer[2] = {c, '\0'};
    graphics_context_set_text_color(ctx, color);
    graphics_text_draw(ctx, buffer, FONT_MAIN, CELL_RECT(x, y), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}

static void grid_set_cell_background(GContext* ctx, GColor color, ushort x, ushort y) {
    graphics_context_set_fill_color(ctx, color);
    graphics_fill_rect(ctx, CELL_RECT(x, y), 5, GCornerTopLeft | GCornerBottomRight);
}
