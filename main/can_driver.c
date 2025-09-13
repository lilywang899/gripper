#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "event_source.h"
#include "esp_event_base.h"
//#include "sdkconfig.h"
#include "driver/twai.h"

/* --------------------- Definitions and static variables ------------------ */
//Example Configuration
#define TX_GPIO_NUM             CONFIG_EXAMPLE_TX_GPIO_NUM
#define RX_GPIO_NUM             CONFIG_EXAMPLE_RX_GPIO_NUM

#define ID_MASTER_STOP_CMD      0x0A0
#define ID_MASTER_PING          0x0A2

#define ID_SLAVE_STOP_RESP      0x0B0
#define ID_SLAVE_DATA           0x0B1

uint8_t nodeID = 1;


static const char *TAG = "Can";
static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_25KBITS();
static const twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
static const twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(TX_GPIO_NUM, RX_GPIO_NUM, TWAI_MODE_NORMAL);
static bool running = true;

static const twai_message_t stop_message = {
    // Message type and format settings
    .extd = 0,              // Standard Format message (11-bit ID)
    .rtr = 0,               // Send a data frame
    .ss = 0,                // Not single shot
    .self = 0,              // Not a self reception request
    .dlc_non_comp = 0,      // DLC is less than 8
    // Message ID and payload
    .identifier = ID_MASTER_STOP_CMD,
    .data_length_code = 0,
    .data = {0},
};

twai_message_t  txHeader =  {
    .extd = 0,          // Standard Format message (11-bit ID)
    .rtr = 0,           // Send a data frame
    .ss = 0,            // Not single shot
    .self = 0,          // Not a self reception request
    .dlc_non_comp = 1,  // DLC is less than 8
    // Message ID and payload
    .data_length_code = 8,
    .data = {0},
};

// Event loops
esp_event_loop_handle_t can_task;

/* --------------------------- Tasks and Functions -------------------------- */
/* Event source can bus related definitions */
ESP_EVENT_DEFINE_BASE(CAN_EVENTS);

static void can_init_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    //Install TWAI driver
    ESP_ERROR_CHECK(twai_driver_install(&g_config, &t_config, &f_config));
    ESP_LOGI(TAG, "Can bus Driver installed");
}

static void can_start_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
   uint32_t data_msgs_rec = 0;
   while (running)
   {
       twai_message_t rx_msg;
       twai_receive(&rx_msg, portMAX_DELAY);
       uint8_t id = rx_msg.identifier >> 7;     // 4Bits ID & 7Bits Msg
       if(id == nodeID) {
          uint8_t cmd = rx_msg.identifier & 0x7F;  // 4Bits ID & 7Bits Msg

         /*----------------------- ↓ Add Your CAN1 Packet Protocol Here ↓ ------------------------*/
         switch (cmd)
         {
            // enable gripper
            case 0x23:
                //dummy.motorJ[id]->UpdateAngleCallback(*(float*) (data), data[4]);
                break;
            // disable gripper
            case 0x25:
                 //memcpy(&dummy.motorJ[id]->temperature, data, sizeof(uint32_t));//(uint32_t) (data);
                break;

            // update PID (set value of position)

            // update Torque (set value of position)

            // get_status of Torque (periodical interval =10ms)

            // get_status of PID (periodical interval =10ms)
            default:
                break;
        }
      }
   }
}
static void can_stop_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
     running = false;
}

static void can_deinit_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    ESP_ERROR_CHECK(twai_stop());
    ESP_LOGI(TAG, "Driver stopped");
}

void can_task_init()
{
    esp_event_loop_args_t args = {
        .queue_size = 5,
        .task_name = "can_task", // task will be created
        .task_priority = uxTaskPriorityGet(NULL),
        .task_stack_size = 3072,
        .task_core_id = tskNO_AFFINITY
    };

    // Create the event loops
    ESP_ERROR_CHECK(esp_event_loop_create(&args, &can_task));

    // Register the handler for task iteration event. Notice that the same handler is used for handling event on different loops.
    // The loop handle is provided as an argument in order for this example to display the loop the handler is being run on.
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(can_task, CAN_EVENTS, TASK_CAN_INIT,    can_init_handler,  can_task, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(can_task, CAN_EVENTS, TASK_CAN_STARTED, can_start_handler, can_task, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(can_task, CAN_EVENTS, TASK_CAN_STOPPED, can_stop_handler,  can_task, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(can_task, CAN_EVENTS, TASK_CAN_DEINIT,  can_deinit_handler,can_task, NULL));
}

static void send_stop_data()
{
   //Transmit start command to slave
   twai_transmit(&stop_message, portMAX_DELAY);
   ESP_LOGI(TAG, "Transmitted start command");
}

void CanSendMessage(uint8_t* txData, uint8_t length)
{
   //Transmit start command to slave
   for( int i = 0; i < length; i++)
   {
       txHeader.data[i] = txData[i];
   }
   txHeader.data_length_code = length;

   twai_transmit(&txHeader, portMAX_DELAY);
}
