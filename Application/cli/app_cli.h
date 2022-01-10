#ifndef APPLICATION_CLI_APP_CLI_H
#define APPLICATION_CLI_APP_CLI_H

#include "FreeRTOS.h"
#include "task.h"

#define APP_CLI_TASK_STACK_SIZE         128
#define APP_CLI_QUEUE_LEN               128
#define APP_CLI_QUEUE_ITEM_SZ           sizeof(uint8_t)
#define APP_CLI_COMMAND_MAX_INPUT_SZ    (256)

extern TaskHandle_t appCliTskHdl;

extern void APP_cli_init(void);


#endif /* APPLICATION_CLI_APP_CLI_H */
