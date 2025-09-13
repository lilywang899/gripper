/* esp_event (event loop library) basic example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#ifndef EVENT_SOURCE_H_
#define EVENT_SOURCE_H_

#include "esp_event.h"
#include "esp_timer.h"

#ifdef __cplusplus
extern "C" {
#endif


// Declarations for event source 1: periodic timer
#define TIMER_EXPIRIES_COUNT        3        // number of times the periodic timer expires before being stopped
#define TIMER_PERIOD                1000000  // period of the timer event source in microseconds

extern esp_timer_handle_t g_timer;           // the periodic timer object

// Declare an event base
ESP_EVENT_DECLARE_BASE(TIMER_EVENTS);        // declaration of the timer events family

enum {                                       // declaration of the specific events under the timer event family
    TIMER_EVENT_STARTED,                     // raised when the timer is first started
    TIMER_EVENT_EXPIRY,                      // raised when a period of the timer has elapsed
    TIMER_EVENT_STOPPED                      // raised when the timer has been stopped
};


// Declarations for the event source
#define TASK_ITERATIONS_COUNT        20      // number of times the task iterates
#define TASK_PERIOD                  500     // period of the task loop in milliseconds

ESP_EVENT_DECLARE_BASE(TASK_EVENTS);         // declaration of the task events family

enum {
    TASK_ADC_INIT,                           // Initialize ADC interface
    TASK_ITERATION_EVENT,                    // raised during an iteration of the loop within the task
    TASK_ADC_DEINIT                          // De-Initialized ADC interface
};

typedef struct interrupt_args
{
            bool _event_handler_set;
            bool _custom_event_handler_set ;
            bool _queue_enabled;
//            gpio_num_t _pin;
            esp_event_loop_handle_t _custom_event_loop_handle;
//            xQueueHandle _queue_handle;
} interrupt_args;

ESP_EVENT_DECLARE_BASE(MOTOR_EVENTS);         // declaration of the task events family

enum {
    TASK_MOTOR_INIT,                           // Initialize ADC interface
    TASK_MOTOR_STARTED,                         // raised during an iteration of the loop within the task
    TASK_MOTOR_DEINIT                          // De-Initialized ADC interface
};


ESP_EVENT_DECLARE_BASE(PID_TIMER_EVENTS);         // declaration of the task events family

enum {
    PID_TIMER_EVENT_STARTED,                     // raised when the timer is first started
    PID_TIMER_EVENT_EXPIRY,                      // raised when a period of the timer has elapsed
    PID_TIMER_EVENT_STOPPED                      // raised when the timer has been stopped
};

ESP_EVENT_DECLARE_BASE(CONTROLLER_EVENTS);         // declaration of the task events family

enum {
    TASK_CONTROLLER_INIT,
    TASK_CONTROLLER_ENABLE_MOTOR,                // raised when the timer is first started
    TASK_CONTROLLER_STARTED,
    TASK_CONTROLLER_UPDATE,                      // raised when a period of the timer has elapsed
    TASK_CONTROLLER_STOPPED                      // raised when the timer has been stopped
};


ESP_EVENT_DECLARE_BASE(CAN_EVENTS);         // declaration of the task events family

enum {
    TASK_CAN_INIT,
    TASK_CAN_STARTED,                     // raised when the timer is first started
    TASK_CAN_STOPPED,
    TASK_CAN_DEINIT                       // raised when a period of the timer has elapsed
};


ESP_EVENT_DECLARE_BASE(CONSOLE_EVENTS);         // declaration of the task events family
enum {
    TASK_CONSOLE_STARTED,                     // raised when the timer is first started
    TASK_CONSOLE_STOPPED
};

#ifdef __cplusplus
}
#endif

#endif // #ifndef EVENT_SOURCE_H_
