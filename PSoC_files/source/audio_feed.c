#include "audio_feed.h"
#include "audio_app.h"
#include "usb_comm.h"
#include "audio.h"

#include "cycfg.h"
#include "cy_sysint.h"

/*******************************************************************************
* Local Functions
*******************************************************************************/
void audio_feed_endpoint_callback(USBFS_Type *base,
                                  cy_stc_usbfs_dev_drv_context_t *context);

/*******************************************************************************
* Audio Feedback Variables
*******************************************************************************/
uint8_t audio_feed_data[AUDIO_FEEDBACK_ENDPOINT_SIZE] = {0x00, 0x00, 0x0C};

/*******************************************************************************
* Function Name: audio_feed_init
********************************************************************************
* Summary:
*   Initialize the audio feedback endpoint.
*
*******************************************************************************/
void audio_feed_init(void)
{
    /* Register SOF Callback */
    Cy_USBFS_Dev_Drv_RegisterSofCallback(CYBSP_USBDEV_HW,
                                         audio_feed_endpoint_callback,
                                         &usb_drvContext);
}

/*******************************************************************************
* Function Name: audio_feed_update_sample_rate
********************************************************************************
* Summary:
*   Set the audio streaming sample rate.
*
*******************************************************************************/
void audio_feed_update_sample_rate(uint32_t sample_rate)
{
    switch (sample_rate)
    {
        /* The sample rate in the feedback endpoint is represented with 3 bytes
         * as an fraction number - X.Y, where:
         * X = (Byte[2] << 2) + (Byte[1] >> 6)
         * Y = (Byte[1] & 0x3F)) + (Byte[0] & 0xF0)
         */ 
        case AUDIO_SAMPLING_RATE_48KHZ:
            audio_feed_data[0] = 0x00u;
            audio_feed_data[1] = 0x00u;
            audio_feed_data[2] = 0x0Cu;
            break;

          case AUDIO_SAMPLING_RATE_44KHZ:
            audio_feed_data[0] = 0x40u;
            audio_feed_data[1] = 0x06u;
            audio_feed_data[2] = 0x0Bu;
            break;
          default:
            break;
    }
}

/*******************************************************************************
* Function Name: audio_feed_endpoint_callback
********************************************************************************
* Summary:
*   Audio feedback endpoint callback implementation. It updates the sample rate
*   based on the status of the internal I2S buffer.
*
*******************************************************************************/
void audio_feed_endpoint_callback(USBFS_Type *base, cy_stc_usbfs_dev_drv_context_t *context)
{
    uint32_t i2s_count;
    uint32_t feedback_sample_rate;
    uint8_t  feedback_data[AUDIO_FEEDBACK_ENDPOINT_SIZE];
    cy_stc_usb_dev_context_t *devContext = Cy_USBFS_Dev_Drv_GetDevContext(base, context);

    /* Only process if the enable feedback flag is set */
    if (usb_comm_enable_feedback == true)
    {
        /* Get the number of bytes in the I2S TX FIFO */
        i2s_count = Cy_I2S_GetNumInTxFifo(i2s.base);

        /* Extract the current sample rate */
        feedback_sample_rate = (((uint32_t) audio_feed_data[2]) << 16) |
                               (((uint32_t) audio_feed_data[1]) << 8)  |
                               (((uint32_t) audio_feed_data[0]) << 0);

        /* Check the current I2S TX FIFO count to slightly change the sample
           rate if necessary */
        if (i2s_count < (AUDIO_FRAME_DATA_SIZE - 1))
        {
            feedback_sample_rate += AUDIO_FEED_SINGLE_SAMPLE;
        }
        else if (i2s_count > (AUDIO_FRAME_DATA_SIZE + 1))
        {
            feedback_sample_rate -= AUDIO_FEED_SINGLE_SAMPLE;
        }

        /* Update the feedback data */
        feedback_data[2] = (uint8_t) (feedback_sample_rate >> 16);
        feedback_data[1] = (uint8_t) (feedback_sample_rate >> 8);
        feedback_data[0] = (uint8_t) (feedback_sample_rate >> 0);

        /* Load feedback enpoint */
        Cy_USB_Dev_WriteEpNonBlocking(AUDIO_FEEDBACK_IN_ENDPOINT, feedback_data,
                                      AUDIO_FEEDBACK_ENDPOINT_SIZE, devContext);
    }
}

/* [] END OF FILE */

