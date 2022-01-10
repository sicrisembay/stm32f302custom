#include "stdio.h"
#include "FreeRTOS.h"
#include "FreeRTOS_CLI.h"
#include "cmd_power.h"
#include "main.h"

static BaseType_t FuncPower(char * write_buffer, size_t bufferLen, const char *commandStr)
{
	char * ptrStrParam;
	BaseType_t strLenParam;

	/* Get string parameter */
	ptrStrParam = (char *)FreeRTOS_CLIGetParameter(commandStr, 1, &strLenParam);
	ptrStrParam[strLenParam] = '\0';

	if(0 == strcmp(ptrStrParam, "on")) {
		/* Turn on Relay */
		LL_GPIO_SetOutputPin(RELAY_GPIO_Port, RELAY_Pin);
		snprintf(write_buffer, bufferLen,
				"\r\n    Relay ON.\r\n\r\n");
	} else if (0 == strcmp(ptrStrParam, "off")) {
		/* Turn off Relay */
		LL_GPIO_ResetOutputPin(RELAY_GPIO_Port, RELAY_Pin);
		snprintf(write_buffer, bufferLen,
				"\r\n    Relay OFF.\r\n\r\n");
	} else {
		snprintf(write_buffer, bufferLen,
				"    Unknown argument!\r\n\r\n");
		return 0;
	}

    return 0;
}

static const CLI_Command_Definition_t boardResetCommand = {
    "power",
    "power:\r\n"
	"    Power on/off\r\n\r\n",
    FuncPower,
    1
};


void CMD_power_init(void)
{
	FreeRTOS_CLIRegisterCommand(&boardResetCommand);
}
