#include <pebble.h>
#include <math.h>

static Window* window;
static Layer* attitude_layer;
static TextLayer* text_layer;
static GRect bounds;
static int accel_x, accel_y, accel_z;

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

static void draw_attitude(Layer *layer, GContext *ctx) {
  static char s_buffer[128];
  int pitch_scaling = 1000;

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
  // Draw lines
  graphics_context_set_stroke_color(ctx, GColorWhite);
  int x, x2, x3, y, y2, y3, px, px2, px3, py, py2, py3;
  for (i = -26; i < 26; i++) {
    if (i % 2 == 1 || -i % 2 == 1) {
      x = 10; // 20 pixels
    } else {
      x = 20; // 20 pixels
    }
    x2 = -x;
    y = 0x10000 * i / 72; // ten degrees
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
  // Draw bank indicator
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_context_set_stroke_width(ctx, 3);
  // the arrow at 0 deg
  graphics_draw_line(ctx, GPoint(72, 11), GPoint(76, 3));
  graphics_draw_line(ctx, GPoint(72, 11), GPoint(68, 3));
  graphics_draw_line(ctx, GPoint(76, 3), GPoint(68, 3));
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_line(ctx, GPoint(59, 12), GPoint(58, 4));
  graphics_draw_line(ctx, GPoint(47, 15), GPoint(44, 7));
  graphics_draw_line(ctx, GPoint(35, 21), GPoint(31, 14));
  graphics_draw_line(ctx, GPoint(20, 32), GPoint(16, 24));
  graphics_draw_line(ctx, GPoint(20, 32), GPoint(12, 28));
  graphics_draw_line(ctx, GPoint(16, 24), GPoint(12, 28));
  graphics_draw_line(ctx, GPoint(9, 47), GPoint(2, 43));
  graphics_draw_line(ctx, GPoint(84, 12), GPoint(85, 4));
  graphics_draw_line(ctx, GPoint(96, 15), GPoint(99, 7));
  graphics_draw_line(ctx, GPoint(108, 21), GPoint(112, 14));
  graphics_draw_line(ctx, GPoint(123, 32), GPoint(127, 24));
  graphics_draw_line(ctx, GPoint(123, 32), GPoint(131, 28));
  graphics_draw_line(ctx, GPoint(127, 24), GPoint(131, 28));
  graphics_draw_line(ctx, GPoint(134, 47), GPoint(141, 43));
  graphics_context_set_stroke_color(ctx, GColorYellow);
  graphics_context_set_stroke_width(ctx, 3);
  x = 0;
  y = -72;
  x2 = 4;
  y2 = -64;
  x3 = -x2;
  y3 = y2;
  px = (cos_lookup(-bank) * x - sin_lookup(-bank) * y) / 0x10000;
  py = (sin_lookup(-bank) * x + cos_lookup(-bank) * y) / 0x10000;
  px2 = (cos_lookup(-bank) * x2 - sin_lookup(-bank) * y2) / 0x10000;
  py2 = (sin_lookup(-bank) * x2 + cos_lookup(-bank) * y2) / 0x10000;
  px3 = (cos_lookup(-bank) * x3 - sin_lookup(-bank) * y3) / 0x10000;
  py3 = (sin_lookup(-bank) * x3 + cos_lookup(-bank) * y3) / 0x10000;
  px = px + 72;
  py = py + 84;
  px2 = px2 + 72;
  py2 = py2 + 84;
  px3 = px3 + 72;
  py3 = py3 + 84;
  graphics_draw_line(ctx, GPoint(px, py), GPoint(px2, py2));
  graphics_draw_line(ctx, GPoint(px2, py2), GPoint(px3, py3));
  graphics_draw_line(ctx, GPoint(px3, py3), GPoint(px, py));
  // Draw crosshair
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, 5);
  graphics_draw_line(ctx, GPoint(72, 84), GPoint(72, 84));
  graphics_context_set_stroke_width(ctx, 3);
  graphics_draw_line(ctx, GPoint(37, 84), GPoint(57, 84));
  graphics_draw_line(ctx, GPoint(87, 84), GPoint(109, 84));
}

static void data_handler(AccelData *data, uint32_t num_samples) {
  int smoothing = 3;
  accel_x = (data[0].x + accel_x * smoothing) / (smoothing + 1);
  accel_y = (data[0].y + accel_y * smoothing) / (smoothing + 1);
  accel_z = (data[0].z + accel_z * smoothing) / (smoothing + 1);
  layer_mark_dirty(attitude_layer);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  bounds = layer_get_bounds(window_layer);

  attitude_layer = layer_create(bounds);
  layer_set_update_proc(attitude_layer, draw_attitude);
  layer_add_child(window_layer, attitude_layer);

  /*
  text_layer = text_layer_create((GRect) { .origin = { 0, 0 }, .size = { bounds.size.w, 100 } });
  text_layer_set_text(text_layer, "data");
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_layer));
  */
}

static void window_unload(Window *window) {
  text_layer_destroy(text_layer);
}

static void init(void) {
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);

  accel_data_service_subscribe(1, data_handler);
  light_enable(true);
}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
