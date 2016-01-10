#include <pebble.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#define ACCEL_STEP_MS 50
#define X_VEC_LENGTH 85
#define HIDDEN_LAYER_SIZE 150
#define CLASSIFYER_NUM 3
#define THETA_SIZE 85 * 90


Window *window;
TextLayer *text_layer;
static double xVec[X_VEC_LENGTH];
static double aVec[HIDDEN_LAYER_SIZE];
static double cVec[CLASSIFYER_NUM];
static double* theta1;
static double* theta2;
static char *s_buffer;

double strtod2(const char *nptr, char **endptr)
{
    double x = 0.0;
    double xs= 1.0;
    double es = 1.0;
    double xf = 0.0;
    double xd = 1.0;
    while( isspace( (unsigned char)*nptr ) ) ++nptr;
    if(*nptr == '-')
    {
        xs = -1;
        nptr++;
    }
    else if(*nptr == '+')
    {
        nptr++;
    }
 
    while (1)
    {
        if (isdigit((unsigned char)*nptr))
        {
            x = x * 10 + (*nptr - '0');
            nptr++;
        }
        else
        {
            x = x * xs;
            break;
        }
    }
    if (*nptr == '.')
    {
        nptr++;
        while (1)
        {
            if (isdigit((unsigned char)*nptr))
            {
                xf = xf * 10 + (*nptr - '0');
                xd = xd * 10;
            }
            else
            {
                x = x + xs * (xf / xd);
                break;
            }
            nptr++;
        }
    }
    if ((*nptr == 'e') || (*nptr == 'E'))
    {
        nptr++;
        if (*nptr == '-')
        {
            es = -1;
            nptr++;
        }
        xd = 1;
        xf = 0;
        while (1)
        {
            if (isdigit((unsigned char)*nptr))
            {
                xf = xf * 10 + (*nptr - '0');
                nptr++;
            }
            else
            {
                while (xf > 0)
                {
                    xd *= 10;
                    xf--;
                }
                if (es < 0.0)
                {
                    x = x / xd;
                }
                else
                {
                    x = x * xd;
                }
                break;
            }
        }
    }
    if (endptr != NULL)
    {
        *endptr = (char *)nptr;
    }
    return (x);
}

static void load_resource() 
{
  // Get resource and size
  ResHandle handle = resource_get_handle(RESOURCE_ID_THETA1_DATA);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "handle %p", handle);
  size_t res_size = resource_size(handle);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "size %d", res_size);
  // Copy to buffer
  s_buffer = (char*)malloc(res_size + 1);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "after malloc %d", res_size);
  resource_load(handle, (uint8_t*)s_buffer, res_size);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "after load %d", res_size);
  s_buffer[res_size] = '\0';
  APP_LOG(APP_LOG_LEVEL_DEBUG, "after null %d", res_size);  
  theta1 = (double*)malloc(sizeof(double) * THETA_SIZE);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "aftermalloc2  %p", theta1);  
  char* p = s_buffer;
  for (int i = 0; i < THETA_SIZE; i++) {
    theta1[i] = strtod2(p, &p);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "%g", theta1[i]);
    p++;
  }
}


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
    xVec[vecIndex] = x * x + y * y + z * z;
    vecIndex++;
    if (vecIndex == X_VEC_LENGTH) {
#if 0
      for (int i = 1; i < HIDDEN_LAYER_SIZE; i++) {
        aVec[i] = 0;
        for (int j = 1; j < X_VEC_LENGTH; j++) {
          aVec[i] += theta1[j] * xVec[j];
        }
      }

      // todo i = 0
      for (int i = 1; i < CLASSIFYER_NUM; i++) {
        cVec[i] = 0;
        for (int j = 1; j < HIDDEN_LAYER_SIZE; j++) {
          cVec[i] += theta2[j] * aVec[j];
        }
      }
#endif
      vecIndex = 0;
    }
    app_timer_register(ACCEL_STEP_MS, timer_callback, NULL);    
    return;
  }
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "%d %d %d", x - accel.x, y - accel.y, z - accel.z);  
  x = accel.x;
  y = accel.y;
  y = accel.z;

  app_timer_register(ACCEL_STEP_MS, timer_callback, NULL);
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Message received!");
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

void init() {
  //  load_resource();
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  app_message_open(64, 64);

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
