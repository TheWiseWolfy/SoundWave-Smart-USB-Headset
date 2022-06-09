#include "audio_in.h"
#include "audio_app.h"
#include "audio.h"
#include "usb_comm.h"

#include "cyhal.h"
#include "cycfg.h"
#include "cy_sysint.h"

#include "rtos.h"

#include "cy_device_headers.h"

#include "ai.h"

/*******************************************************************************
* Local Functions
*******************************************************************************/
void audio_in_endpoint_callback(USBFS_Type *base, 
                                uint32_t endpoint,
                                uint32_t errorType, 
                                cy_stc_usbfs_dev_drv_context_t *context);

void convert_32_to_24_array(uint8_t *src, uint8_t *dst, uint32_t length);

/*******************************************************************************
* Audio In Variables
*******************************************************************************/
/* USB IN buffer data for Audio IN endpoint */
CY_USB_DEV_ALLOC_ENDPOINT_BUFFER(audio_in_usb_buffer, AUDIO_IN_ENDPOINT_SIZE + 1);

/* PCM buffer data (32-bits) */
uint8_t audio_in_pcm_buffer[4 * AUDIO_IN_ENDPOINT_SIZE/AUDIO_SAMPLE_DATA_SIZE];

/* Audio IN flags */
volatile bool audio_in_is_recording    = false;

/* Size of the frame */
volatile uint32_t audio_in_frame_size = AUDIO_FRAME_DATA_SIZE;

/*******************************************************************************
* Function Name: audio_in_init
********************************************************************************
* Summary:
*   Initialize the audio IN endpoint by setting up the DMAs involved and the
*   PDM/PCM block.
*
*******************************************************************************/
void audio_in_init(void)
{
    /* Register Data Endpoint Callbacks */
    Cy_USBFS_Dev_Drv_RegisterEndpointCallback(CYBSP_USBDEV_HW,
                                              AUDIO_STREAMING_IN_ENDPOINT,
                                              audio_in_endpoint_callback,
                                              &usb_drvContext);

    /* Run the I2S RX all the time */
    cyhal_i2s_start_rx(&i2s);
}

/*******************************************************************************
* Function Name: audio_in_enable
********************************************************************************
* Summary:
*   Start a recording session.
*
*******************************************************************************/
void audio_in_enable(void)
{
    BaseType_t xHigherPriorityTaskWoken, xResult;

    /* xHigherPriorityTaskWoken must be initialised to pdFALSE. */
    xHigherPriorityTaskWoken = pdFALSE;

    xResult  = xEventGroupSetBitsFromISR(rtos_events, RTOS_EVENT_IN,
                                         &xHigherPriorityTaskWoken);

    /* Was the message posted successfully? */
    if( xResult != pdFAIL )
    {
        /* If xHigherPriorityTaskWoken is now set to pdTRUE then a context
        switch should be requested.  The macro used is port specific and will
        be either portYIELD_FROM_ISR() or portEND_SWITCHING_ISR() - refer to
        the documentation page for the port being used. */
      portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
    }
}

/*******************************************************************************
* Function Name: audio_in_disable
********************************************************************************
* Summary:
*   Stop a recording session.
*
*******************************************************************************/
void audio_in_disable(void)
{
    /* Only disable I2S if Audio OUT is not enabled */
    if (false == usb_comm_enable_out_streaming)
    {
        cyhal_i2s_stop_rx(&i2s);
    }

    audio_in_is_recording = false;
}

/*******************************************************************************
* Function Name: audio_in_process
********************************************************************************
* Summary:
*   Main task for the audio in endpoint. Check if should start a recording
*   session.
*
*******************************************************************************/
void audio_in_process(void *arg)
{
    (void) arg;

    while (1)
    {
        /* Only process if a request to stream in data is set and the clock is
           in sync with the desired sample rate */
        xEventGroupWaitBits(rtos_events,
                            RTOS_EVENT_IN | RTOS_EVENT_SYNC,
                            pdFALSE, pdTRUE, portMAX_DELAY);

        if (usb_comm_clock_configured)
        {
            audio_in_is_recording = true;

            /* Clear Audio In buffer */
            memset(audio_in_usb_buffer, 0, AUDIO_IN_ENDPOINT_SIZE);

            /* Clear I2S RX FIFO */
            Cy_I2S_ClearRxFifo(i2s.base);

            /* Start I2S RX */
            cyhal_i2s_start_rx(&i2s);

            /* Start a transfer to the Audio IN endpoint */
            Cy_USB_Dev_WriteEpNonBlocking(AUDIO_STREAMING_IN_ENDPOINT,
                                          (uint8_t *) audio_in_usb_buffer,
                                          AUDIO_IN_ENDPOINT_SIZE,
                                          &usb_devContext);

            xEventGroupClearBits(rtos_events, RTOS_EVENT_IN);
        }
    }
}

/*******************************************************************************
* Function Name: audio_in_update_sample_rate
********************************************************************************
* Summary:
*   Updates the audio frame size based on the sample rate. The frame size is 
*   number of bytes transmitted in 1ms time frame.
*
* Parameters:
*   sample_rate: sample rate to be enforced in Hz
*
*******************************************************************************/
void audio_in_update_sample_rate(uint32_t sample_rate)
{
    /* Frame Size is equal to sample rate (Hz) / 1000 * 2 (stereo) */
    audio_in_frame_size = 2 * (sample_rate / 1000);
}

float stop = 4294967295;
float start = 0;

float sound_normalization(float sound){
	return (sound - start) / (stop - start);
}

/*******************************************************************************
* Function Name: audio_in_endpoint_callback
********************************************************************************
* Summary:
*   Audio in endpoint callback implementation. It enables the Audio in DMA to
*   stream an audio frame.
*
*******************************************************************************/
void audio_in_endpoint_callback(USBFS_Type *base, 
                                uint32_t endpoint,
                                uint32_t errorType, 
                                cy_stc_usbfs_dev_drv_context_t *context)
{
    /* Set the count equal to the frame size */
    size_t audio_in_count = audio_in_frame_size;

    (void) errorType;
    (void) endpoint,
    (void) context;
    (void) base;

    /* Check if should keep recording */
    if ((audio_in_is_recording == true) && (usb_comm_clock_configured == true))
    {
        /* Read all the data in the I2S RX buffer */
        cyhal_i2s_read(&i2s, (void *) audio_in_pcm_buffer, &audio_in_count);

        /* Limit the size to avoid overflow in the internal buffer */
        if (audio_in_count > AUDIO_MAX_DATA_SIZE)
        {
            audio_in_count = AUDIO_MAX_DATA_SIZE;
        }

        //Iuli magic
    	size_t j = 0;
    	size_t temp_recorded_size = recorded_size;

        for(size_t i = temp_recorded_size ; i < temp_recorded_size + audio_in_count && i < AI_BUFFER_SIZE; i++){

        	int32 sound = (audio_in_pcm_buffer[j] <<24) +
							(audio_in_pcm_buffer[j + 1]<<16) +
							(audio_in_pcm_buffer[j + 2]<<8) +
							audio_in_pcm_buffer[j + 3];

        	sound = sound - 2147483647;

        	float magic = sound / 2147483647.0f;

        	data_feed[i] = magic;
        	//data_feed[i]= (float)sound;
        	j+=4;
        }

      //  We remember how much we wrote
        recorded_size += audio_in_count;

        /* Convert the I2S data array (32-bit) to USB data array (24-bit) */
        convert_32_to_24_array(audio_in_pcm_buffer, audio_in_usb_buffer, audio_in_count);

        Cy_USB_Dev_WriteEpNonBlocking(AUDIO_STREAMING_IN_ENDPOINT,
                                      (uint8_t *) audio_in_usb_buffer,
                                      audio_in_count*AUDIO_SAMPLE_DATA_SIZE,
                                      &usb_devContext);
    }
    else
    {
        cyhal_i2s_stop_rx(&i2s);
    }
}

/*******************************************************************************
* Function Name: convert_32_to_24_array
********************************************************************************
* Summary:
*   Convert a 32-bit array to 24-bit array.
*
*******************************************************************************/
void convert_32_to_24_array(uint8_t *src, uint8_t *dst, uint32_t length)
{
    while (0u != length--)
    {
        *(dst++) = *src++;
        *(dst++) = *src++;
        *(dst++) = *src++;
        src++;
    }
}

/* [] END OF FILE */
