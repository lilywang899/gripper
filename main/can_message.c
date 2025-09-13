#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/twai.h"
#include "event_source.h"
#include "esp_event_base.h"

enum State
{
        RUNNING,
        FINISH,
        STOP
};
extern uint8_t nodeID;
extern void CanSendMessage(uint8_t* txData, uint8_t length);
extern twai_message_t  txHeader ;
extern esp_event_loop_handle_t controller_task;


typedef void (*CanCommandHandleFunc)(uint8_t data[8],  int length);
typedef struct cmdTable {
   uint8_t cmdId;
   void (*CanCommandHandleFunc)(uint8_t data[8],  int length);
}cmdTable;

void HandleEnableController(uint8_t data[8],  int length);
void HandleGetTroque(uint8_t data[8],  int length);

cmdTable commands[] = {
   {0x23, HandleEnableController},
   {0x23, HandleGetTroque}
};

uint8_t canBuf[8] = {0};
void SetEnable(bool _enable)
{
    enum State state = _enable ? FINISH : STOP;

    uint8_t mode = 0x01;
    txHeader.identifier = nodeID << 7 | mode;

    // Int to Bytes
    uint32_t val = _enable ? 1 : 0;
    unsigned char* b = (unsigned char*) &val;
    for (int i = 0; i < 4; i++)
        canBuf[i] = *(b + i);
    CanSendMessage(canBuf, 4);
}

void SetPositionSetPoint(float _val)
{
    uint8_t mode = 0x05;
    txHeader.identifier = nodeID << 7 | mode;

    // Float to Bytes
    unsigned char* b = (unsigned char*) &_val;
    for (int i = 0; i < 4; i++)
        canBuf[i] = *(b + i);
    canBuf[4] = 1; // Need ACK

    CanSendMessage(canBuf, 5);
}

CanCommandHandleFunc getCommandHandlerById(int command)
{
    CanCommandHandleFunc func_ptr = NULL;
    for(int i = 0;  i < (sizeof(commands) / sizeof(commands[0])); i++)
     {
          if( commands [i].cmdId == command)
          {
              func_ptr = commands[i].CanCommandHandleFunc;
          }
     }
     return func_ptr;
}
void HandleEnableController(uint8_t data[8],  int length)
{
    bool enable  = data[0];
    ESP_ERROR_CHECK(esp_event_post_to(controller_task, CONTROLLER_EVENTS, TASK_CONTROLLER_ENABLE_MOTOR, &enable, sizeof(enable), portMAX_DELAY));
}

void HandleGetTroque(uint8_t data[8],  int length)
{
    bool enable  = data[0];
    ESP_ERROR_CHECK(esp_event_post_to(controller_task, CONTROLLER_EVENTS, TASK_CONTROLLER_ENABLE_MOTOR, &enable, sizeof(enable), portMAX_DELAY));
}

