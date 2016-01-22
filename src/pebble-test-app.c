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

// Simple ring buffer
typedef struct ring_buffer
{
  uint32_t *buffer;
  uint32_t *_consective_buffer;
  uint32_t length;
  uint32_t index;
} ring_buffer;

static ring_buffer* ring_buffer_init(uint32_t length)
{
  ring_buffer *ret = (ring_buffer*)malloc(sizeof(ring_buffer));
  ret->buffer = (uint32_t *)malloc(sizeof(uint32_t) * length);
  ret->_consective_buffer = (uint32_t *)malloc(sizeof(uint32_t) * length);
  ret->length = length;
  ret->index = 0;
  return ret;
}

static void ring_buffer_write(ring_buffer* r, uint32_t val)
{
  r->buffer[r->index] = val;
  r->index++;
  if (r->index == r->length) {
    r->index = 0;
  }
}

static uint32_t* ring_buffer_get_buffer(ring_buffer* r)
{
  memset(r->_consective_buffer, 0, r->length * sizeof(uint32_t));
  memcpy(r->_consective_buffer, r->buffer + r->index, r->length - r->index);
  memcpy(r->_consective_buffer + r->length - r->index, r->buffer, r->index);
  return r->_consective_buffer;
}

static ring_buffer* get_ring_buffer()
{
  static ring_buffer* r;
  if (NULL == r) {
    r = ring_buffer_init(X_VEC_LENGTH);
  }
  return r;
}

static void timer_callback(void *data) {
  static int x;
  static int y;
  static int z;
  static uint32_t counter;
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
  ring_buffer* r = get_ring_buffer();
  uint32_t val =  x * x + y * y + z * z;
  ring_buffer_write(r, val);

  counter++;
  // every 500 msec, we send it to the companion app
  if ((counter % 10) == 0) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "FULL");
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    if (iter == NULL) {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "null iter");
      app_timer_register(ACCEL_STEP_MS, timer_callback, NULL);
      return;
    }
    uint32_t* data = ring_buffer_get_buffer(r);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "data <%x %x %x>", (unsigned int)(data[0]), (unsigned int)(data[1]), (unsigned int)(data[2]));
    Tuplet tuple = TupletBytes(MESSAGE_KEY_ACCEL_DATA, (uint8_t*)data, r->length * sizeof(uint32_t));
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

static char* texts[] = { "Noise", "Down", "Warrior" };
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  Tuple *data = dict_find(iterator, 1);
  if (data) {
    APP_LOG(APP_LOG_LEVEL_INFO, "KEY_DATA received with value %d", (int)data->value->int32);

    //    snprintf(buf, 32, "%d", (int)data->value->int32);
    text_layer_set_text(text_layer, texts[data->value->int32]);
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
  text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
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
