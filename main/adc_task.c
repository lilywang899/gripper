#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "event_source.h"
#include "esp_event_base.h"
#include "driver/gptimer.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

/*---------------------------------------------------------------
        ADC General Macros
---------------------------------------------------------------*/
typedef struct ADC_Channel {
    adc_channel_t name;
    adc_cali_handle_t handle;
    int adc_raw;
    int voltage;
}ADC_Channel;

struct ADC_Channel channel[4]= {
  {ADC_CHANNEL_3,NULL,0,0},
  {ADC_CHANNEL_6,NULL,0,0}
//  ADC_Channel(ADC_CHANNEL_5,NULL,0,0),
//  ADC_Channel(ADC_CHANNEL_6,NULL,0,0),
};
#define EXAMPLE_ADC2_CHAN0          ADC_CHANNEL_3
#define EXAMPLE_ADC2_CHAN1          ADC_CHANNEL_4
//#define EXAMPLE_ADC2_CHAN0          ADC_CHANNEL_5
//#define EXAMPLE_ADC2_CHAN1          ADC_CHANNEL_6

#define EXAMPLE_ADC_ATTEN           ADC_ATTEN_DB_12
long unsigned int  count = 0;

typedef struct {
     esp_event_loop_handle_t handle;
} adc_timer_context_t;

adc_timer_context_t adc_timer_ctx;



static const char* TAG = "adc_task";

static gptimer_handle_t s_gptimer;
extern gptimer_handle_t eventfd_timer_init( const uint64_t alarm_count, bool reload_on_alarm, void *user_ctx);
extern char* get_id_string(esp_event_base_t base, int32_t id);

#define TIMER_RESOLUTION      1000000 // 1MHz, 1 tick = 1us
#define TIMER_INTERVAL_US     2.500000  // 0.5s

adc_oneshot_unit_handle_t adc2_handle;
bool adc_init_done = false;

// Event loops
esp_event_loop_handle_t adc_task;

/* Event source periodic timer related definitions */
ESP_EVENT_DEFINE_BASE(TIMER_EVENTS);

// Handler which executes when the timer started event gets executed by the loop.
static void timer_started_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    int start_handler_num = *((int*) handler_args);
    ESP_LOGI(TAG, "%s:%s: timer_started_handler, instance %d", base, get_id_string(base, id), start_handler_num);
}

// Handler which executes when the timer expiry event gets executed by the loop. This handler keeps track of
// how many times the timer expired. When a set number of expiry is reached, the handler stops the timer
// and sends a timer stopped event.
static void timer_expiry_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    count ++;

    for(int i = 0; i < 2 ; i++) {
        ESP_ERROR_CHECK(adc_oneshot_read(adc2_handle, channel[i].name, &(channel[i].adc_raw)));
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(channel[i].handle, channel[i].adc_raw, &(channel[i].voltage)));
    }
   if( count % 20 == 0) {
         ESP_LOGI(TAG, "[%010lu] Ch[%d][%d]:[%d], Ch[%d][%d]:[%d] ",count,
                        channel[0].name, channel[0].adc_raw, channel[0].voltage,
                        channel[1].name, channel[1].adc_raw, channel[1].voltage);
   }
}

// Handler which executes when the timer stopped event gets executed by the loop. Since the timer has been
// stopped, it is safe to delete it.
static void timer_stopped_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    ESP_LOGI(TAG, "%s:%s: timer_stopped_handler", base, get_id_string(base, id));
    // Delete the timer
    ESP_ERROR_CHECK(gptimer_del_timer(s_gptimer));
    ESP_LOGI(TAG, "%s:%s: deleted timer event source", base, get_id_string(base, id));
}

/* Event source task related definitions */
ESP_EVENT_DEFINE_BASE(TASK_EVENTS);
static void adc_init_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    //-------------ADC2 Init---------------//
    adc_oneshot_unit_init_cfg_t init_config2 = {
        .unit_id = ADC_UNIT_2,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config2, &adc2_handle));

    //-------------ADC2 Config---------------//
    adc_oneshot_chan_cfg_t config = {
        .atten = EXAMPLE_ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT_2,
        .atten = EXAMPLE_ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    for(int i = 0; i < 2 ; i++) {
       adc_cali_create_scheme_line_fitting(&cali_config, &(channel[i].handle));
       ESP_ERROR_CHECK(adc_oneshot_config_channel(adc2_handle, channel[i].name, &config));
    }
    ESP_LOGI(TAG, "adc initialization is done");
    adc_init_done = true;
}

static void task_iteration_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    // Two types of data can be passed in to the event handler: the handler specific data and the event-specific data.
    //
    // The handler specific data (handler_args) is a pointer to the original data, therefore, the user should ensure that
    // the memory location it points to is still valid when the handler executes.
    //
    // The event-specific data (event_data) is a pointer to a deep copy of the original data, and is managed automatically.
    int iteration = *((int*) event_data);
    char* loop;

    if (handler_args == adc_task) {
        loop = "adc_task";
    }

    ESP_LOGI(TAG, "handling %s:%s from %s, iteration %d", base, "TASK_ITERATION_EVENT", loop, iteration);
}

static void adc_deinit_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    //Tear Down ADC channel.
    ESP_ERROR_CHECK(adc_oneshot_del_unit(adc2_handle));
    for(int i = 0; i < 2 ; i++) {
         ESP_ERROR_CHECK(adc_cali_delete_scheme_line_fitting(channel[i].handle));
    }
    ESP_LOGI(TAG, "adc deinit  is done");
}

static void adc_timer_callback(void *args)
{
    adc_timer_context_t *ctx = (adc_timer_context_t *)args;
    ESP_ERROR_CHECK(esp_event_post_to(ctx->handle, TIMER_EVENTS, TIMER_EVENT_EXPIRY, NULL, 0, portMAX_DELAY));
}

void adc_task_init()
{
    esp_event_loop_args_t loop_with_task_args = {
        .queue_size = 50,
        .task_name = "adc_task", // task will be created
        .task_priority = uxTaskPriorityGet(NULL),
        .task_stack_size = 3072,
        .task_core_id = tskNO_AFFINITY
    };

    // Create the event loops
    ESP_ERROR_CHECK(esp_event_loop_create(&loop_with_task_args, &adc_task));

    // Register the handler for task iteration event. Notice that the same handler is used for handling event on different loops.
    // The loop handle is provided as an argument in order for this example to display the loop the handler is being run on.
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(adc_task, TASK_EVENTS, TASK_ADC_INIT, adc_init_handler, adc_task, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(adc_task, TASK_EVENTS, TASK_ITERATION_EVENT, task_iteration_handler, adc_task, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(adc_task, TASK_EVENTS, TASK_ADC_DEINIT, adc_deinit_handler, adc_task, NULL));

   // Register the specific timer event handlers. Timer start handler is registered twice.
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(adc_task, TIMER_EVENTS, TIMER_EVENT_STARTED, timer_started_handler, adc_task, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(adc_task, TIMER_EVENTS, TIMER_EVENT_EXPIRY,  timer_expiry_handler,  adc_task, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(adc_task, TIMER_EVENTS, TIMER_EVENT_STOPPED, timer_stopped_handler, adc_task, NULL));

    vTaskDelay(10);
    ESP_ERROR_CHECK(esp_event_post_to(adc_task, TASK_EVENTS, TASK_ADC_INIT, NULL, 0, portMAX_DELAY));

    adc_timer_ctx.handle = adc_task;
    const esp_timer_create_args_t adc_timer_args = {
        .callback = adc_timer_callback,
        .arg = &adc_timer_ctx,
        .name = "adc_timer"
    };
    esp_timer_handle_t adc_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&adc_timer_args, &adc_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(adc_timer, 5 * 1000));
}


