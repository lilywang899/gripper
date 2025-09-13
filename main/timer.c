#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "event_source.h"
#include "esp_event_base.h"
#include "esp_timer.h"
#include "driver/gptimer.h"


static const char* TAG = "timer";
extern bool adc_init_done;

#define TIMER_RESOLUTION      1000000 // 1MHz, 1 tick = 1us
#define TIMER_INTERVAL_US     2500000 // 2.5s


// Callback that will be executed when the timer period lapses. Posts the timer expiry event
// to the default event loop.
#if 0
static void timer_callback(void* arg)
{
    ESP_LOGI(TAG, "%s:%s: posting to default loop", TIMER_EVENTS, get_id_string(TIMER_EVENTS, TIMER_EVENT_EXPIRY));
    esp_event_loop_handle_t handle = (esp_event_loop_handle_t)arg;
    ESP_ERROR_CHECK(esp_event_post_to(handle, TIMER_EVENTS, TIMER_EVENT_EXPIRY, NULL, 0, portMAX_DELAY));
}
#endif
char* get_id_string(esp_event_base_t base, int32_t id);

IRAM_ATTR static bool  eventfd_timer_isr_callback(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{


    if ( adc_init_done ) {
        int sample = 0;
        //esp_event_loop_handle_t handle = (esp_event_loop_handle_t)user_ctx;
        //ESP_ERROR_CHECK(esp_event_post_to(handle, TIMER_EVENTS, TIMER_EVENT_EXPIRY, NULL, 0, portMAX_DELAY));
        //ESP_ERROR_CHECK(esp_event_isr_post_to(handle, TIMER_EVENTS, TIMER_EVENT_EXPIRY, NULL, 0, NULL));
        //ESP_ERROR_CHECK(esp_event_isr_post_to(handle, TIMER_EVENTS, TIMER_EVENT_EXPIRY, &sample, sizeof(sample), NULL));
        struct interrupt_args * args = (struct interrupt_args *)user_ctx;
        //ESP_ERROR_CHECK(esp_event_isr_post_to(args->_custom_event_loop_handle, TIMER_EVENTS, TIMER_EVENT_EXPIRY, &sample, sizeof(sample), NULL));
        ESP_ERROR_CHECK(esp_event_post_to(args->_custom_event_loop_handle, TIMER_EVENTS, TIMER_EVENT_EXPIRY, NULL, 0, portMAX_DELAY));
    }
    return true;
}

gptimer_handle_t eventfd_timer_init( const uint64_t alarm_count, bool reload_on_alarm, void *user_ctx)
{
    gptimer_handle_t timer_handle;
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = TIMER_RESOLUTION,
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &timer_handle));

    gptimer_alarm_config_t alarm_config = {
        .reload_count = 0,
        .alarm_count = alarm_count,
        .flags.auto_reload_on_alarm = reload_on_alarm,
    };
    gptimer_event_callbacks_t cbs = {
        .on_alarm = eventfd_timer_isr_callback,
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(timer_handle, &cbs, user_ctx));
    ESP_ERROR_CHECK(gptimer_set_alarm_action(timer_handle, &alarm_config));

    /* Clear the timer raw count value, make sure it'll count from 0 */
    ESP_ERROR_CHECK(gptimer_set_raw_count(timer_handle, 0));

    ESP_ERROR_CHECK(gptimer_enable(timer_handle));
    ESP_ERROR_CHECK(gptimer_start(timer_handle));
    ESP_LOGI(TAG, "eventfd_timer_init timer_handle is started.");
    return timer_handle;
 }

char* get_id_string(esp_event_base_t base, int32_t id) {
    char* event = "";
    if (base == TIMER_EVENTS) {
        switch(id) {
            case TIMER_EVENT_STARTED:
            event = "TIMER_EVENT_STARTED";
            break;
            case TIMER_EVENT_EXPIRY:
            event = "TIMER_EVENT_EXPIRY";
            break;
            case TIMER_EVENT_STOPPED:
            event = "TIMER_EVENT_STOPPED";
            break;
        }
    } else {
        event = "TASK_ITERATION_EVENT";
    }
    return event;
}