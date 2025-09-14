#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "event_source.h"
#include "esp_event_base.h"
#include "driver/gptimer.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"


extern void adc_task_init();
extern void controller_task_init();
extern void can_task_init();
extern void console_task_init();

// Event loops
extern esp_event_loop_handle_t adc_task;
extern esp_event_loop_handle_t can_task;
extern esp_event_loop_handle_t controller_task;
extern esp_event_loop_handle_t console_task;


static const char* TAG = "main";

/* Example main */
void app_main(void)
{
    ESP_LOGI(TAG, "setting up");
    adc_task_init();
    //controller_task_init();
    can_task_init();
    console_task_init();
    vTaskDelay(100);

    // Post the timer started event
    ESP_ERROR_CHECK(esp_event_post_to(adc_task, TIMER_EVENTS, TIMER_EVENT_STARTED, NULL, 0, portMAX_DELAY));
    // Post the timer started event
    //ESP_ERROR_CHECK(esp_event_post_to(controller_task, CONTROLLER_EVENTS, TASK_CONTROLLER_STARTED, NULL, 0, portMAX_DELAY));
    //ESP_ERROR_CHECK(esp_event_post_to(can_task, CAN_EVENTS, TASK_CAN_INIT, NULL, 0, portMAX_DELAY));
    ESP_ERROR_CHECK(esp_event_post_to(console_task, CONSOLE_EVENTS, TASK_CONSOLE_STOPPED, NULL, 0, portMAX_DELAY));
    vTaskDelay(100);
    while(1) {
        vTaskDelay(100);
    }
    ESP_ERROR_CHECK(esp_event_post_to(adc_task, TASK_EVENTS, TASK_ADC_DEINIT, NULL, 0, portMAX_DELAY));
//
//    gptimer_stop(s_gptimer);
//    gptimer_disable(s_gptimer);
//    gptimer_del_timer(s_gptimer);
}
