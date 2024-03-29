#include "cmsis_os2.h"
#include "usbd_cdc_if.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "FreeRTOS_CLI.h"
#include "app_cli.h"

static StaticTask_t xTaskBuffer;
static StackType_t xStack[APP_CLI_TASK_STACK_SIZE];
TaskHandle_t appCliTskHdl = NULL;

static StaticQueue_t xQueue;
static uint8_t xQueueStorage[APP_CLI_QUEUE_LEN * APP_CLI_QUEUE_ITEM_SZ];
static QueueHandle_t appCliQueueHdl;

static char input_strBuf[APP_CLI_COMMAND_MAX_INPUT_SZ];
static const char * const welcomeString = "\r\n\n"
        "******************************************************\r\n"
        "* Type 'help' to view a list of registered commands.\r\n"
        "******************************************************\r\n";
static const char * const strLineSep = "\r\n";
static const char * const strPrompt = "$ ";

static void vTaskCommandConsole(void * pvParam);
static void ReceiveData(uint8_t* Buf, uint32_t *Len);
static void helper_send_blocking(char * pBuf, BaseType_t count);

void APP_cli_init(void)
{
	appCliTskHdl = xTaskCreateStatic(
			vTaskCommandConsole,
			"cli",
			APP_CLI_TASK_STACK_SIZE,
			(void *) 0,
			osPriorityNormal1,
			xStack,
			&xTaskBuffer);
}

static void vTaskCommandConsole(void * pvParam)
{
	BaseType_t xMoreDataToFollow = 0;
	BaseType_t inputIndex = 0;
	char * output_str_buf = NULL;
	char rxChar = 0;

	appCliQueueHdl = xQueueCreateStatic(
					APP_CLI_QUEUE_LEN,
					APP_CLI_QUEUE_ITEM_SZ,
					xQueueStorage,
					&xQueue);
	output_str_buf = FreeRTOS_CLIGetOutputBuffer();
	CDC_Register_ReceiveCB(ReceiveData);

	vTaskDelay(1000);
    /* print welcome string */
    helper_send_blocking((char *)welcomeString, strlen(welcomeString));

    /* print prompt symbol */
    helper_send_blocking((char *)strPrompt, 2);

	while(1) {
		if(pdTRUE == xQueueReceive(appCliQueueHdl, &rxChar, portMAX_DELAY)) {
            /* Accept only ASCII (0x00 - 0x7F) */
            if((rxChar < 0x00) || (rxChar > 0x7F)) {
                continue;
            }

            if(rxChar == '\n') {
                /*
                 * A newline character was received, so the input command string is
                 * complete and can be processed.  Transmit a line separator, just to
                 * make the output easier to read.
                 */
                helper_send_blocking((char *)strLineSep, strlen(strLineSep));
                helper_send_blocking((char *)strLineSep, strlen(strLineSep));

                if(strlen(input_strBuf) == 0) {
                    /* No command to process */
                    /* Just print prompt */
                    helper_send_blocking((char *)strPrompt, 2);
                    continue;
                }

                /*
                 * The command interpreter is called repeatedly until it returns
                 * pdFALSE (0).
                 */
                do {
                    /*
                     * Send the command string to the command interpreter.  Any
                     * output generated by the command interpreter will be placed in the
                     * output_str_buf buffer.
                     */
                	xMoreDataToFollow = FreeRTOS_CLIProcessCommand
                                  (
                                      input_strBuf,                         /* The command string.*/
                                      output_str_buf,                       /* The output buffer. */
									  configCOMMAND_INT_MAX_OUTPUT_SIZE     /* The size of the output buffer. */
                                  );

                    /*
                     * Write the output generated by the command interpreter to the
                     * console.
                     */
                    helper_send_blocking(output_str_buf, strlen(output_str_buf));

                } while( xMoreDataToFollow != 0 );

                /*
                 * All the strings generated by the input command have been sent.
                 * Processing of the command is complete.  Clear the input string ready
                 * to receive the next command.
                 */
                inputIndex = 0;
                memset( input_strBuf, 0x00, sizeof(input_strBuf) );

                /* print prompt symbol */
                helper_send_blocking((char *)strPrompt, 2);

            } else {
                /*
                 * The if() clause performs the processing after a newline character
                 * is received.  This else clause performs the processing if any other
                 * character is received. */
                if( rxChar == '\r' ) {
                    /* Ignore carriage returns. */\

                } else if( rxChar == '\b' ) {
                    /*
                     * Backspace was pressed.  Erase the last character in the input
                     * buffer - if there are any
                     */
                    if( inputIndex > 0 )
                    {
                        inputIndex--;
                        input_strBuf[ inputIndex ] = '\0';
                    }
                } else {
                    /*
                     * A character was entered.  It was not a new line, backspace
                     * or carriage return, so it is accepted as part of the input and
                     * placed into the input buffer.  When a n is entered the complete
                     * string will be passed to the command interpreter.
                     */
                    if( inputIndex < APP_CLI_COMMAND_MAX_INPUT_SZ )
                    {
                        input_strBuf[ inputIndex ] = rxChar;
                        inputIndex++;
                    }
                }
            }
		}
	}
}

static void ReceiveData(uint8_t* Buf, uint32_t *Len)
{
	uint8_t rxByte;
	uint32_t idx;

	if((Buf == NULL) || (Len == NULL) || (*Len == 0)) {
		return;
	}

	if(xPortIsInsideInterrupt()) {
		BaseType_t xHigherPriorityTaskWoken;
		for(idx = 0; idx < *Len; idx++) {
			rxByte = Buf[idx];
			xQueueSendFromISR(appCliQueueHdl, &rxByte, &xHigherPriorityTaskWoken);
		}

		if(xHigherPriorityTaskWoken) {
			taskYIELD();
		}
	} else {
		for(idx = 0; idx < *Len; idx++) {
			if(pdTRUE != xQueueSend(appCliQueueHdl, &rxByte, 0)) {
				// FULL
				return;
			}
		}
	}
}

static void helper_send_blocking(char * pBuf, BaseType_t count)
{
	while(CDC_Trasmit_is_busy()) {
		vTaskDelay(1);
	}

	CDC_Transmit_FS((uint8_t *)pBuf, (uint16_t)count);
}
