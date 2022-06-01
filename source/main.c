#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"
#include "cycfg.h"

#include "audio_app.h"
#include "audio_out.h"
#include "audio_in.h"
#include "touch.h"
//#include "ai.h"

#include "rtos.h"

#define COMPONENT_ML_FLOAT32

/*******************************************************************************
* Global Variables
********************************************************************************/
/* RTOS tasks */
TaskHandle_t rtos_audio_app_task;
TaskHandle_t rtos_audio_in_task;
TaskHandle_t rtos_audio_out_task;
TaskHandle_t rtos_touch_task;
TaskHandle_t rtos_ai_task;

/* RTOS Event Group */
EventGroupHandle_t rtos_events;

/*******************************************************************************
* Function Name: main
********************************************************************************
* Summary:
*  Main function of Cortex-M4. Creates all RTOS related elements and runs the
*  RTOS scheduler.
*
* Parameters:
*  void
*
* Return:
*  int
*
*******************************************************************************/
int main(void)
{
    cy_rslt_t result;

    /* Initialize the device and board peripherals */
    result = cybsp_init() ;
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Enable global interrupts */
    __enable_irq();

    /* Create the RTOS tasks */
    xTaskCreate(audio_app_process, "Audio App Task",
                RTOS_STACK_DEPTH, NULL, RTOS_TASK_PRIORITY,
                &rtos_audio_app_task);

    xTaskCreate(audio_in_process, "Audio In Task",
                RTOS_STACK_DEPTH, NULL, RTOS_TASK_PRIORITY,
                &rtos_audio_in_task);

    xTaskCreate(audio_out_process, "Audio Out Task",
                RTOS_STACK_DEPTH, NULL, RTOS_TASK_PRIORITY,
                &rtos_audio_out_task);

    xTaskCreate(touch_process, "Touch Task",
                RTOS_STACK_DEPTH, NULL, RTOS_TASK_PRIORITY,
                &rtos_touch_task);

  ///  xTaskCreate( ai_process, "Ai Task",
   //             RTOS_STACK_DEPTH, NULL, RTOS_TASK_PRIORITY,
   //             &rtos_ai_task);


    /* Create RTOS Event Group */
    rtos_events = xEventGroupCreate();

    /* Start the RTOS Scheduler */
    vTaskStartScheduler();

    /* Should never get there */

    return 0;
}

/*******************************************************************************
* Function Name: vApplicationIdleHook()
********************************************************************************
* Summary:
*   RTOS Idle task implementation.
*******************************************************************************/
void vApplicationIdleHook( void )
{
    /* Go to sleep */
    cyhal_system_sleep();
}





/* [] END OF FILE */
