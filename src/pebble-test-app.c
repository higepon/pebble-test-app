#include <pebble.h>

Window *window;

void init() {
  // Create the Window
  window = window_create();

  // Push to the stack, animated
  window_stack_push(window, true);
}

void deinit() {
  // Destroy the Window
  window_destroy(window);
}

int main() {
  // Initialize the app
  init();

  // Wait for app events
  app_event_loop();

  // Deinitialize the app
  deinit();

  // App finished without error
  return 0;
}
