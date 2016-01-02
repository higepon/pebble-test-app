#include <pebble.h>

Window *window;
TextLayer *text_layer;

void init() {
  window = window_create();
  text_layer = text_layer_create(GRect(50, 50, 144, 40));
  text_layer_set_text(text_layer, "Hello, Pebble!");
  layer_add_child(window_get_root_layer(window), 
                    text_layer_get_layer(text_layer));
  window_stack_push(window, true);
}

void deinit() {
  text_layer_destroy(text_layer);
  window_destroy(window);
}

void try_log() {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Data logging Start");

  DataLoggingSessionRef logging_session = data_logging_create(0x1234, DATA_LOGGING_UINT, 4,
                                                              false);
  // Fake creating some data and logging it to the session.
  uint32_t data[] = { 1, 2, 3};
  data_logging_log(logging_session, &data, 3);

  // Fake creating more data and logging that as well.
  uint32_t data2[] = { 1, 2 };
  data_logging_log(logging_session, &data2, 2);

  // When we don't need to log anything else, we can close off the session.
  data_logging_finish(logging_session);
}

int main() {
  try_log();
  init();
  try_log();
  app_event_loop();
  deinit();
  return 0;
}
