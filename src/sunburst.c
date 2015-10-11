#include <pebble.h>

#define DEGREES_IN_HOUR 30
#define DEGREES_IN_MINUTE 6

static Window *main_window;

static Layer *sun_layer;

static GPoint s_center;

int second;
int minute;
int hour;

static void update_time() {
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);
  
  second = tick_time->tm_sec;
  minute = tick_time->tm_min;
  hour = tick_time->tm_hour;

  // Create a long-lived buffer
  static char buffer[] = "00:00";

  // Write the current hours and minutes into the buffer
  if(clock_is_24h_style() == true) {
    // Use 24 hour format
    strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
  } else {
    // Use 12 hour format
    strftime(buffer, sizeof("00:00"), "%I:%M", tick_time);
  }

}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void draw_circle(GContext *ctx, GRect rect, GColor color, int r, int deg) {
  graphics_context_set_fill_color(ctx, color);
  graphics_fill_radial(
    ctx, rect, 
    GOvalScaleModeFillCircle,
    r,
    DEG_TO_TRIGANGLE(0),
    deg % 360 == 0 ? TRIG_MAX_ANGLE : DEG_TO_TRIGANGLE(deg)
  );  
}

static void sun_layer_update(Layer *layer, GContext *ctx) {
  const GRect entire_screen = GRect(0, 0, 180, 180);
  const GRect entire_screen_shorter = GRect(15, 15, 150, 150);
  const GRect inner_circle = GRect(30, 30, 120, 120);
  const GRect inner_circle_shorter = GRect(40, 40, 100, 100);
  const GRect hour_rect = GRect(70,70,40,40);
  const GRect minute_rect = GRect(50,50,80,80);

  draw_circle(ctx, entire_screen, GColorWhite, 90, 360);

  int i;

  graphics_context_set_stroke_color(ctx, GColorDarkGray);
  graphics_context_set_stroke_width(ctx, 6);

  // no bools in C, so int with values 0 or 1 instead
  int shorter = 0;
  
  for (i = -3; i < 360; i += 15) {
    GRect outer_rect;
    if (shorter == 0) {
      outer_rect = entire_screen;
      shorter = 1;
    } else {
      outer_rect = entire_screen_shorter;
      shorter = 0;
    }
    const GPoint out = gpoint_from_polar(
      outer_rect,
      GOvalScaleModeFitCircle,
      DEG_TO_TRIGANGLE(i)
    );
    graphics_draw_line(ctx, out, s_center);
  }
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, 4);

  for (i = 0; i < 360; i += 10) {
    GRect outer_rect;
    if (shorter == 0) {
      outer_rect = inner_circle;
      shorter = 1;
    } else {
      outer_rect = inner_circle_shorter;
      shorter = 0;
    }
    const GPoint out = gpoint_from_polar(
      outer_rect,
      GOvalScaleModeFitCircle,
      DEG_TO_TRIGANGLE(i)
    );
    graphics_draw_line(ctx, out, s_center);
  }

  
  // Mod hour by 12 since 14:00 is 2:00 in analog, then multiply by the degrees in an hour
  // Then add the fraction of the hour that has passed
  int hour_degree =  (DEGREES_IN_HOUR * (hour % 12)) + (DEGREES_IN_HOUR * minute / 60);  
  int minute_degree = DEGREES_IN_MINUTE * minute;

  graphics_context_set_stroke_color(ctx, GColorTiffanyBlue);
  graphics_context_set_stroke_width(ctx, 4);

  const GPoint hour_out = gpoint_from_polar(
    hour_rect,
    GOvalScaleModeFitCircle,
    DEG_TO_TRIGANGLE(hour_degree)
  );
  graphics_draw_line(ctx, hour_out, s_center);
  const GPoint minute_out = gpoint_from_polar(
    minute_rect,
    GOvalScaleModeFitCircle,
    DEG_TO_TRIGANGLE(minute_degree)
  );
  graphics_draw_line(ctx, minute_out, s_center);
}

static void main_window_load(Window *window) {

  Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);

  s_center = grect_center_point(&window_bounds);

  window_set_background_color(window, GColorBlack);

  sun_layer = layer_create(GRect(0, 0, 180, 180));
  layer_set_update_proc(sun_layer, sun_layer_update);

  layer_add_child(window_layer, sun_layer);
}
static void main_window_unload(Window *window) {
  layer_destroy(sun_layer);
}

static void init() {

  // Create main Window element and assign to pointer
  main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(main_window, true);
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  // Make sure the time is displayed from the start
  update_time();

}

static void deinit() {
  // Destroy Window
  window_destroy(main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}