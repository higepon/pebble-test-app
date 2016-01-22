#include <pebble.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>

#define ACCEL_STEP_MS 50
#define X_VEC_LENGTH 85
#define HIDDEN_LAYER_SIZE 150
#define CLASSIFYER_NUM 3
#define THETA_SIZE 85 * 90

#define MESSAGE_KEY_ACCEL_DATA 0

Window *window;
TextLayer *text_layer;
static uint32_t xVec[X_VEC_LENGTH];

static void timer_callback(void *data) {
  static int x;
  static int y;
  static int z;
  static int vecIndex;
  AccelData accel = (AccelData) { .x = 0, .y = 0, .z = 0 };
  accel_service_peek(&accel);
  if (x == 0 && y == 0 && z == 0) {
    x = accel.x;
    y = accel.y;
    y = accel.z;
    app_timer_register(ACCEL_STEP_MS, timer_callback, NULL);
    return;
  }

  x = x - accel.x;
  x = y - accel.y;
  x = z - accel.z;
  xVec[vecIndex] = x * x + y * y + z * z;
  vecIndex++;
  //  APP_LOG(APP_LOG_LEVEL_DEBUG, "%d/%d", vecIndex, X_VEC_LENGTH);
  if (vecIndex == X_VEC_LENGTH) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "FULL");
      vecIndex = 0;
      DictionaryIterator *iter;
      app_message_outbox_begin(&iter);
      if (iter == NULL) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "null iter");
        app_timer_register(ACCEL_STEP_MS, timer_callback, NULL);
        return;
      }
      APP_LOG(APP_LOG_LEVEL_DEBUG, "data <%x %x %x>", (unsigned int)(xVec[0]), (unsigned int)(xVec[1]), (unsigned int)(xVec[2]));
      Tuplet tuple = TupletBytes(MESSAGE_KEY_ACCEL_DATA, (uint8_t*)xVec, X_VEC_LENGTH * sizeof(uint32_t));
      dict_write_tuplet(iter, &tuple);
      dict_write_end(iter);
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Sending...");
      app_message_outbox_send();
  }

  app_timer_register(ACCEL_STEP_MS, timer_callback, NULL);    
  x = accel.x;
  y = accel.y;
  y = accel.z;
  return;
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  Tuple *data = dict_find(iterator, 1);
  if (data) {
    APP_LOG(APP_LOG_LEVEL_INFO, "KEY_DATA received with value %d", (int)data->value->int32);
  } else {
    APP_LOG(APP_LOG_LEVEL_INFO, "KEY_DATA not received.");
  }
  //  APP_LOG(APP_LOG_LEVEL_INFO, "Message received!");
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed! %d", reason);
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

void init() {
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

  window = window_create();
  text_layer = text_layer_create(GRect(50, 50, 144, 40));
  text_layer_set_text(text_layer, "Hello, Pebble!");
  layer_add_child(window_get_root_layer(window), 
                    text_layer_get_layer(text_layer));
  window_stack_push(window, true);

  accel_data_service_subscribe(0, NULL);
  app_timer_register(ACCEL_STEP_MS, timer_callback, NULL);
}

void deinit() {
  text_layer_destroy(text_layer);
  window_destroy(window);
}

int main() {
  init();
  app_event_loop();
  deinit();
  return 0;
}
