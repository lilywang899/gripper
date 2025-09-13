#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/pulse_cnt.h"
#include "bdc_motor.h"
#include "pid_ctrl.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "event_source.h"
#include "esp_event_base.h"
#include "driver/gptimer.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "can_message.h"


static const char *TAG = "controller";

#define BDC_MCPWM_TIMER_RESOLUTION_HZ 10000000 // 10MHz, 1 tick = 0.1us
#define BDC_MCPWM_FREQ_HZ             25000    // 25KHz PWM
#define BDC_MCPWM_DUTY_TICK_MAX       (BDC_MCPWM_TIMER_RESOLUTION_HZ / BDC_MCPWM_FREQ_HZ) // maximum value we can set for the duty cycle, in ticks

//Motor A
#define BDC_MCPWM_GPIO_A              26  // Motor Forward pin
#define BDC_MCPWM_GPIO_B              27  // Motor Reverse pin

//Motor B
//#define BDC_MCPWM_GPIO_A              33  // Motor Forward pin
//#define BDC_MCPWM_GPIO_B              25  // Motor Reverse pin

#define BDC_PID_LOOP_PERIOD_MS        10   // calculate the motor speed every 10ms
#define BDC_PID_EXPECT_SPEED          400  // expected motor speed, in the pulses counted by the rotary encoder

int g_cur_pulse_count = 0;

typedef struct {
    bdc_motor_handle_t motor;
    pid_ctrl_block_handle_t pid_ctrl;
    esp_event_loop_handle_t handle;
    int report_pulses;
    pcnt_unit_handle_t pcnt_encoder;
} motor_control_context_t;

static motor_control_context_t motor_ctrl_ctx = {
        .pcnt_encoder = NULL,
};

// Event loops
esp_event_loop_handle_t controller_task;
esp_timer_handle_t pid_loop_timer = NULL;


/* Event source periodic timer related definitions */
ESP_EVENT_DEFINE_BASE(PID_TIMER_EVENTS);

// Handler which executes when the timer started event gets executed by the loop.
static void pid_timer_callback(void *args)
{
    motor_control_context_t *ctx = (motor_control_context_t *)args;
    ESP_ERROR_CHECK(esp_event_post_to(ctx->handle, PID_TIMER_EVENTS, PID_TIMER_EVENT_EXPIRY, NULL, 0, portMAX_DELAY));
}

static void timer_started_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    ESP_LOGI(TAG, "Create a timer to do PID calculation periodically");
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = pid_timer_callback,
        .arg = &motor_ctrl_ctx,
        .name = "pid_loop"
    };

    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &pid_loop_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(pid_loop_timer, BDC_PID_LOOP_PERIOD_MS * 1000));

    ESP_LOGI(TAG, "motor speed loop Started.");
}

// Handler which executes when the timer expiry event gets executed by the loop. This handler keeps track of
// how many times the timer expired. When a set number of expiry is reached, the handler stops the timer
// and sends a timer stopped event.
static void timer_expiry_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    ESP_LOGI(TAG, "Enter timer_expiry_handler.");
    static int last_pulse_count = 0;
    motor_control_context_t *ctx = (motor_control_context_t *)handler_args;
    pcnt_unit_handle_t pcnt_unit = ctx->pcnt_encoder;
    pid_ctrl_block_handle_t pid_ctrl = ctx->pid_ctrl;
    bdc_motor_handle_t motor = ctx->motor;

    // get the result from rotary encoder
//    int cur_pulse_count = 0;
//    pcnt_unit_get_count(pcnt_unit, &cur_pulse_count);
//    int real_pulses = cur_pulse_count - last_pulse_count;
//    last_pulse_count = cur_pulse_count;
    g_cur_pulse_count ++;
    int real_pulses =  g_cur_pulse_count - last_pulse_count;
    last_pulse_count = g_cur_pulse_count;
    ctx->report_pulses = real_pulses;

    // calculate the speed error
    float error = BDC_PID_EXPECT_SPEED - real_pulses;
    float new_speed = 0;

    // set the new speed
    pid_compute(pid_ctrl, error, &new_speed);
    bdc_motor_set_speed(motor, (uint32_t)new_speed);

    SetPositionSetPoint(true);
}

// Handler which executes when the timer stopped event gets executed by the loop. Since the timer has been
// stopped, it is safe to delete it.
static void timer_stopped_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    //ESP_LOGI(TAG, "%s:%s: timer_stopped_handler", base, get_id_string(base, id));
    // Delete the timer
    //ESP_ERROR_CHECK(gptimer_del_timer(s_gptimer));
    //ESP_LOGI(TAG, "%s:%s: deleted timer event source", base, get_id_string(base, id));
}

/* Event source task related definitions */
ESP_EVENT_DEFINE_BASE(MOTOR_EVENTS);

static void motor_init_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    bdc_motor_config_t motor_config = {
        .pwm_freq_hz = BDC_MCPWM_FREQ_HZ,
        .pwma_gpio_num = BDC_MCPWM_GPIO_A,
        .pwmb_gpio_num = BDC_MCPWM_GPIO_B,
    };
    bdc_motor_mcpwm_config_t mcpwm_config = {
        .group_id = 0,
        .resolution_hz = BDC_MCPWM_TIMER_RESOLUTION_HZ,
    };
    bdc_motor_handle_t motor = NULL;
    ESP_ERROR_CHECK(bdc_motor_new_mcpwm_device(&motor_config, &mcpwm_config, &motor));
    motor_control_context_t *ctx = (motor_control_context_t *)handler_args;
    ctx->motor = motor;
    ESP_LOGI(TAG, "DC motor Created.");
}

static void motor_start_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    motor_control_context_t *ctx = (motor_control_context_t *)handler_args;

    ESP_ERROR_CHECK(bdc_motor_enable(ctx->motor));
    ESP_LOGI(TAG, "motor Enabled.");

    ESP_ERROR_CHECK(bdc_motor_forward(ctx->motor));
    ESP_LOGI(TAG, "Forward motor");
}

/* Event source task related definitions */
ESP_EVENT_DEFINE_BASE(CONTROLLER_EVENTS);
static void controller_init_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    pid_ctrl_parameter_t pid_runtime_param = {
        .kp = 0.6,
        .ki = 0.4,
        .kd = 0.2,
        .cal_type = PID_CAL_TYPE_INCREMENTAL,
        .max_output   = BDC_MCPWM_DUTY_TICK_MAX - 1,
        .min_output   = 0,
        .max_integral = 1000,
        .min_integral = -1000,
    };
    pid_ctrl_block_handle_t pid_ctrl = NULL;
    pid_ctrl_config_t pid_config = {
        .init_param = pid_runtime_param,
    };
    ESP_ERROR_CHECK(pid_new_control_block(&pid_config, &pid_ctrl));

    motor_control_context_t *ctx = (motor_control_context_t *)handler_args;
    ctx->pid_ctrl = pid_ctrl;
    ESP_LOGI(TAG, "PID control block Created");
}

static void controller_start_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    esp_event_loop_handle_t handle = (esp_event_loop_handle_t)handler_args;

    ESP_ERROR_CHECK(esp_event_post_to(handle, MOTOR_EVENTS, TASK_MOTOR_STARTED, NULL, 0, portMAX_DELAY));
    ESP_ERROR_CHECK(esp_event_post_to(handle, PID_TIMER_EVENTS, PID_TIMER_EVENT_STARTED, NULL, 0, portMAX_DELAY));
}

static void controller_motor_enable_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    ESP_LOGI(TAG, "Handle enable disable motor");
}

void controller_task_init()
{
    esp_event_loop_args_t args = {
        .queue_size = 50,
        .task_name = "controller_task", // task will be created
        .task_priority = uxTaskPriorityGet(NULL),
        .task_stack_size = 3072,
        .task_core_id = tskNO_AFFINITY
    };

    // Create the event loops
    ESP_ERROR_CHECK(esp_event_loop_create(&args, &controller_task));

    // Register the handler for task iteration event. Notice that the same handler is used for handling event on different loops.
    // The loop handle is provided as an argument in order for this example to display the loop the handler is being run on.
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(controller_task, MOTOR_EVENTS, TASK_MOTOR_INIT,    motor_init_handler,  &motor_ctrl_ctx, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(controller_task, MOTOR_EVENTS, TASK_MOTOR_STARTED, motor_start_handler, &motor_ctrl_ctx, NULL));
    //ESP_ERROR_CHECK(esp_event_handler_instance_register_with(controller_task, MOTOR_EVENTS, TASK_MOTOR_DEINIT, motor_deinit_handler, controller_task, NULL));

    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(controller_task, CONTROLLER_EVENTS, TASK_CONTROLLER_INIT,     controller_init_handler,  &motor_ctrl_ctx, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(controller_task, CONTROLLER_EVENTS, TASK_CONTROLLER_STARTED,  controller_start_handler, controller_task, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(controller_task, CONTROLLER_EVENTS, TASK_CONTROLLER_ENABLE_MOTOR,  controller_motor_enable_handler, controller_task, NULL));

    //ESP_ERROR_CHECK(esp_event_handler_instance_register_with(controller_task, CONTROLLER_EVENTS, TASK_CONTROLLER_UPDATE,   controller_update_handler, controller_task, NULL));
    //ESP_ERROR_CHECK(esp_event_handler_instance_register_with(controller_task, CONTROLLER_EVENTS, TASK_CONTROLLER_STOPPED,  controller_stop_handler, controller_task, NULL));

   // Register the specific timer event handlers. Timer start handler is registered twice.
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(controller_task, PID_TIMER_EVENTS, PID_TIMER_EVENT_STARTED, timer_started_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(controller_task, PID_TIMER_EVENTS, PID_TIMER_EVENT_EXPIRY,  timer_expiry_handler,  &motor_ctrl_ctx, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(controller_task, PID_TIMER_EVENTS, PID_TIMER_EVENT_STOPPED, timer_stopped_handler, NULL, NULL));
    vTaskDelay(10);
    ESP_ERROR_CHECK(esp_event_post_to(controller_task, MOTOR_EVENTS, TASK_MOTOR_INIT, NULL, 0, portMAX_DELAY));
    ESP_ERROR_CHECK(esp_event_post_to(controller_task, CONTROLLER_EVENTS, TASK_CONTROLLER_INIT, NULL, 0, portMAX_DELAY));

    motor_ctrl_ctx.handle = controller_task;
 }