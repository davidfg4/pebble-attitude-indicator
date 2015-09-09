#include <pebble.h>
#include <math.h>

static Window* window;
static Layer* attitude_layer;
static GRect bounds;
static int accel_x, accel_y, accel_z;
static bool light;

// http://forums.getpebble.com/discussion/5792/sqrt-function
static int32_t my_sqrt(int32_t num) {
  int32_t estimate, p, error;
  error = 0x4;
  estimate = num / 100;
  p = estimate * estimate;
  while (p - num >= error) {
    estimate = (estimate + (num / estimate)) / 2;
    p = estimate * estimate;
  }
  return estimate;
}

// Draw a line, rotated around the center of the screen by "bank" degrees
static void draw_absolute_rotated_line(GContext *ctx, int bank, int x1, int y1, int x2, int y2) {
  int px1, py1, px2, py2;
  x1 = x1 - 72;
  y1 = y1 - 84;
  x2 = x2 - 72;
  y2 = y2 - 84;
  px1 = (cos_lookup(-bank) * x1 - sin_lookup(-bank) * y1) / 0x10000;
  py1 = (sin_lookup(-bank) * x1 + cos_lookup(-bank) * y1) / 0x10000;
  px2 = (cos_lookup(-bank) * x2 - sin_lookup(-bank) * y2) / 0x10000;
  py2 = (sin_lookup(-bank) * x2 + cos_lookup(-bank) * y2) / 0x10000;
  px1 = px1 + 72;
  py1 = py1 + 84;
  px2 = px2 + 72;
  py2 = py2 + 84;
  graphics_draw_line(ctx, GPoint(px1, py1), GPoint(px2, py2));
}

static void draw_attitude(Layer *layer, GContext *ctx) {
  int pitch_scaling = 1000;

  // Calculate pitch and bank angles
  int pitch = my_sqrt((int32_t)accel_x * (int32_t)accel_x + (int32_t)accel_y * (int32_t)accel_y);
  pitch = atan2_lookup(accel_z, pitch);
  if (pitch > 0x8000) {
    pitch = pitch - 0x10000;
  }
  int bank = atan2_lookup(accel_x, -accel_y);

  // Draw background (ground/sky)
  graphics_context_set_fill_color(ctx, GColorWindsorTan);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  graphics_context_set_stroke_color(ctx, GColorBlue);
  graphics_context_set_fill_color(ctx, GColorBlue);
  graphics_context_set_stroke_width(ctx, 1);
  int i;
  int horizon;
  for (i = 0; i < 168; i++) {
    if (sin_lookup(bank) == 0) {
      if ((i - 84) * -cos_lookup(bank) + pitch*pitch_scaling > 0x10000/2) {
        horizon = 145;
      } else {
        horizon = -1;
      }
    } else {
      horizon = 72 + ((i - 84) * -cos_lookup(bank) + pitch*pitch_scaling)/sin_lookup(bank);
    }
    // 72 = half screen width (144/2)
    // 84 = half screen height (168/2)
    // i-84 * -cos / sin -> calculates bank
    // pitch_scaling = magic ???
    // pitch / sin -> calculates pitch
    if (bank >= 0 && bank <= 0x8000) {
      if (horizon < 0) {
        // do nothing
      } else if (horizon > 144) {
        graphics_draw_line(ctx, GPoint(0, i), GPoint(144, i));
      } else {
        graphics_draw_line(ctx, GPoint(0, i), GPoint(horizon, i));
      }
    } else {
      if (horizon < 0) {
        graphics_draw_line(ctx, GPoint(0, i), GPoint(144, i));
      } else if (horizon > 144) {
        // do nothing
      } else {
        graphics_draw_line(ctx, GPoint(horizon, i), GPoint(144, i));
      }
    }
  }

  // Draw pitch angle lines
  graphics_context_set_stroke_color(ctx, GColorWhite);
  int x, x2, y, px, px2, py, py2;
  for (i = -26; i < 26; i++) {
    if (i % 2 == 1 || -i % 2 == 1) {
      x = 10; // 10 pixels
    } else {
      x = 20; // 20 pixels
    }
    x2 = -x;
    y = 0x10000 * i / 72;
    y = (y + pitch)*pitch_scaling / 0x10000;
    px = (cos_lookup(-bank) * x - sin_lookup(-bank) * y) / 0x10000;
    px2 = (cos_lookup(-bank) * x2 - sin_lookup(-bank) * y) / 0x10000;
    py = (sin_lookup(-bank) * x + cos_lookup(-bank) * y) / 0x10000;
    py2 = (sin_lookup(-bank) * x2 + cos_lookup(-bank) * y) / 0x10000;
    px = px + 72;
    px2 = px2 + 72;
    py = py + 84;
    py2 = py2 + 84;
    graphics_draw_line(ctx, GPoint(px, py), GPoint(px2, py2));
  }
  // Draw numbers for degrees of pitch
  graphics_context_set_text_color(ctx, GColorWhite);
  static char text[] = "1234";
  for (i = -9; i < 10; i++) {
    if (i == 0)
        continue;
    x = 23; // 20 pixels
    y = 0x10000 * i / 36; // ten degrees
    y = (y + pitch)*pitch_scaling / 0x10000;
    x = x + 10;
    px = (cos_lookup(-bank) * x - sin_lookup(-bank) * y) / 0x10000;
    py = (sin_lookup(-bank) * x + cos_lookup(-bank) * y) / 0x10000;
    px = px + 72;
    py = py + 84;
    px = px -10;
    py = py -10;
    snprintf(text, sizeof(text), "%i0", -i);
    graphics_draw_text(ctx, text, fonts_get_system_font(FONT_KEY_GOTHIC_14), GRect(px, py, 20, 10), GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
  }

  // Draw the bank indicator
  // draw the arrow at 0 deg
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_context_set_stroke_width(ctx, 5);
  draw_absolute_rotated_line(ctx, bank, 72, 10, 76, 2);
  draw_absolute_rotated_line(ctx, bank, 72, 10, 68, 2);
  draw_absolute_rotated_line(ctx, bank, 76, 2, 68, 2);
  // draw the bank indicator tick marks going left
  graphics_context_set_stroke_width(ctx, 3);
  draw_absolute_rotated_line(ctx, bank, 59, 12, 58, 9);
  draw_absolute_rotated_line(ctx, bank, 47, 15, 46, 12);
  draw_absolute_rotated_line(ctx, bank, 35, 21, 31, 14);
  draw_absolute_rotated_line(ctx, bank, 20, 32, 16, 24);
  draw_absolute_rotated_line(ctx, bank, 20, 32, 12, 28);
  draw_absolute_rotated_line(ctx, bank, 16, 24, 12, 28);
  draw_absolute_rotated_line(ctx, bank, 9, 47, 2, 43);
  // draw the bank indicator tick marks going right
  draw_absolute_rotated_line(ctx, bank, 84, 12, 85, 9);
  draw_absolute_rotated_line(ctx, bank, 96, 15, 97, 12);
  draw_absolute_rotated_line(ctx, bank, 108, 21, 112, 14);
  draw_absolute_rotated_line(ctx, bank, 123, 32, 127, 24);
  draw_absolute_rotated_line(ctx, bank, 123, 32, 131, 28);
  draw_absolute_rotated_line(ctx, bank, 127, 24, 131, 28);
  draw_absolute_rotated_line(ctx, bank, 134, 47, 141, 43);
  // Draw the bank indicator
  graphics_context_set_stroke_color(ctx, GColorYellow);
  graphics_context_set_stroke_width(ctx, 5);
  graphics_draw_line(ctx, GPoint(72, 16), GPoint(76, 24));
  graphics_draw_line(ctx, GPoint(72, 16), GPoint(68, 24));
  graphics_draw_line(ctx, GPoint(76, 24), GPoint(68, 24));

  // Draw crosshair
  graphics_context_set_stroke_color(ctx, GColorYellow);
  graphics_context_set_stroke_width(ctx, 5);
  graphics_draw_line(ctx, GPoint(72, 84), GPoint(72, 84));
  graphics_context_set_stroke_width(ctx, 3);
  graphics_draw_line(ctx, GPoint(37, 84), GPoint(57, 84));
  graphics_draw_line(ctx, GPoint(57, 84), GPoint(57, 88));
  graphics_draw_line(ctx, GPoint(87, 84), GPoint(109, 84));
  graphics_draw_line(ctx, GPoint(87, 84), GPoint(87, 88));
}

static void data_handler(AccelData *data, uint32_t num_samples) {
  int smoothing = 3;
  accel_x = (data[0].x + accel_x * smoothing) / (smoothing + 1);
  accel_y = (data[0].y + accel_y * smoothing) / (smoothing + 1);
  accel_z = (data[0].z + accel_z * smoothing) / (smoothing + 1);
  layer_mark_dirty(attitude_layer);
}

static void toggle_light() {
  light = !light;
  light_enable(light);
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) { toggle_light(); }
static void up_click_handler(ClickRecognizerRef recognizer, void *context) { toggle_light(); }
static void down_click_handler(ClickRecognizerRef recognizer, void *context) { toggle_light(); }

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  bounds = layer_get_bounds(window_layer);

  attitude_layer = layer_create(bounds);
  layer_set_update_proc(attitude_layer, draw_attitude);
  layer_add_child(window_layer, attitude_layer);
}

static void window_unload(Window *window) {
  layer_destroy(attitude_layer);
}

static void init(void) {
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);
  window_set_click_config_provider(window, click_config_provider);

  accel_data_service_subscribe(1, data_handler);
  light = true;
  light_enable(light);
}

static void deinit(void) {
  accel_data_service_unsubscribe();
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
