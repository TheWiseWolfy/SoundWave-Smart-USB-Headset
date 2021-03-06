#include "audio_app.h"
#include "audio_feed.h"
#include "audio_in.h"
#include "audio_out.h"
#include "usb_comm.h"

#include "mtb_wm8960.h"

#include "touch.h"

#include "cyhal.h"
#include "cycfg.h"
#include "cybsp.h"

#include "rtos.h"

/*******************************************************************************
* Macros
********************************************************************************/
#define MI2C_TIMEOUT_MS     10u         /* in ms */
#define MCLK_CODEC_DELAY_MS 10u         /* in ms */
#define MCLK_FREQ_HZ        18432000u   /* in Hz */
#define MCLK_DUTY_CYCLE     50.0f       /* in %  */
#define USB_CLK_RESET_HZ    100000      /* in Hz */
#define PLL_TIMEOUT_US      12000u      /* in us */
#define PLL_FREQ_FOR_48KHZ  55296000    /* in Hz */
#define PLL_FREQ_FOR_44KHZ  50803200    /* in Hz */


/*******************************************************************************
* Global Variables
********************************************************************************/
uint volume_value = 0;


uint8_t  audio_app_control_report;
uint32_t audio_app_current_sample_rate;
int8_t   audio_app_volume;
int8_t   audio_app_prev_volume;
bool     audio_app_mute;

const cyhal_i2s_pins_t i2s_tx_pins = {
    .sck  = P5_1,
    .ws   = P5_2,
    .data = P5_3,
};

const cyhal_i2s_pins_t i2s_rx_pins = {
    .sck  = P5_4,
    .ws   = P5_5,
    .data = P5_6,
};

const cyhal_i2s_config_t i2s_config = {
    .is_tx_slave    = true,     /* TX is Slave */
    .is_rx_slave    = false,    /* RX is Master */
    .mclk_hz        = 0,        /* External MCLK not used */
    .channel_length = 24,       /* In bits */
    .word_length    = 24,       /* In bits */
    .sample_rate_hz = 48000,    /* In Hz */
};

/* Master I2C variables */
cyhal_i2c_t mi2c;
const cyhal_i2c_cfg_t mi2c_cfg = {
	.is_slave        = false,
	.address         = 0,
	.frequencyhal_hz = 400000
};

/* HAL Objects */
cyhal_i2s_t i2s;
cyhal_clock_t pll_clock;
cyhal_clock_t usb_rst_clock;
cyhal_pwm_t mclk_pwm;

/* Tolerance Values */
const cyhal_clock_tolerance_t tolerance_0_p = {CYHAL_TOLERANCE_PERCENT, 0};
const cyhal_clock_tolerance_t tolerance_1_p = {CYHAL_TOLERANCE_PERCENT, 1};

/*******************************************************************************
* Function Prototypes
********************************************************************************/
void audio_app_clock_init(void);
void audio_app_set_clock(uint32_t sample_rate);
void audio_app_update_codec_volume(void);
void audio_app_update_sample_rate(void);
void audio_app_touch_events(uint32_t widget, touch_event_t event, uint32_t value);

cy_rslt_t mi2c_transmit(uint8_t reg_adrr, uint8_t data);

/*******************************************************************************
* Function Name: audio_app_usb_delay
********************************************************************************
* Summary:
*   Delay task used by the USB driver. Delays 1 millisecond.
*
* Parameters:
*   milliseconds: number of milliseconds left to delay.
*
* Return:
*   Updated number of milliseconds left to delay.
*
*******************************************************************************/
__STATIC_INLINE int32_t audio_app_usb_delay(int32_t milliseconds)
{
    vTaskDelay(1/portTICK_PERIOD_MS);

    return (milliseconds - 1);
}

/*******************************************************************************
* Function Name: audio_app_init
********************************************************************************
* Summary:
*   Initialize all the audio related hardware, including the Audio Codec and
*   I2S interface.
*
*******************************************************************************/
void audio_app_init(void)
{
    usb_comm_interface_t interface =
    {
        .disable_in = audio_in_disable,
        .enable_in = audio_in_enable,
        .disable_out = audio_out_disable,
        .enable_out = audio_out_enable
    };

    /* Init the clocks */
    audio_app_clock_init();

    /* Initialize the Master Clock with a PWM */
    cyhal_pwm_init(&mclk_pwm, (cyhal_gpio_t) AUDIO_APP_MCLK_PIN, NULL);
    cyhal_pwm_set_duty_cycle(&mclk_pwm, MCLK_DUTY_CYCLE, MCLK_FREQ_HZ);
    cyhal_pwm_start(&mclk_pwm);

    /* Wait for the MCLK to clock the audio codec */
    cyhal_system_delay_ms(MCLK_CODEC_DELAY_MS);
    
    /* Initialize the I2C Master */
    cyhal_i2c_init(&mi2c, CYBSP_I2C_SDA, CYBSP_I2C_SCL, NULL);
    cyhal_i2c_configure(&mi2c, &mi2c_cfg);

    /* Configure the wm8960 codec and enable it */ ////////////////

    cy_rslt_t result;

	 result = mtb_wm8960_init(&mi2c, WM8960_FEATURE_HEADPHONE | WM8960_FEATURE_MICROPHONE );

	 if (result != 0)
	{
		NVIC_SystemReset();
	}

	mtb_wm8960_activate();

    usb_comm_init();
    usb_comm_register_interface(&interface);
    usb_comm_register_usb_callbacks();

    /* Initialize the I2S block */
    cyhal_i2s_init(&i2s, &i2s_tx_pins, &i2s_rx_pins, NC, &i2s_config, NULL);

    /* Init the audio endpoints */
    audio_in_init();
    audio_out_init();
    audio_feed_init();

    /* Register and enable touch events */
    touch_register_callback(audio_app_touch_events);
    touch_enable_event(TOUCH_ALL, true);

    /* Overwrite the function used internally by the USBFS to handle timeouts */
    Cy_USB_Dev_OverwriteHandleTimeout(audio_app_usb_delay, &usb_devContext);
}

/*******************************************************************************
* Function Name: audio_app_process
********************************************************************************
* Summary:
*   Main audio task. Initialize the USB communication and the audio application.
*   In the main loop, process requests to update the sample rate and change
*   the volume.
*
*******************************************************************************/
void audio_app_process(void *arg)
{
    (void) arg;

    /* Initialize the Audio application */
    audio_app_init();

    /* Enumerate the USB device */
    usb_comm_connect();

    while (1)
    {
        xEventGroupWaitBits(rtos_events, RTOS_EVENT_USB,
            pdTRUE, pdFALSE, portMAX_DELAY );

        if (0u != usb_comm_is_ready())
        {
            /* Update the sample rate */
            audio_app_update_sample_rate();

            /* Convert to Codec Volume */
            audio_app_update_codec_volume();
            //mtb_wm8960_adjust_heaphone_output_volume(0x20);

            /* Set sync bit */
            xEventGroupSetBits(rtos_events, RTOS_EVENT_SYNC);
        }
    }
}

/*******************************************************************************
* Function Name: audio_app_clock_init
********************************************************************************
* Summary:
*   Initialize the clocks in the system.
*******************************************************************************/
void audio_app_clock_init(void)
{
    /* Initialize the PLL */
    cyhal_clock_get(&pll_clock, &CYHAL_CLOCK_PLL[0]);
    cyhal_clock_init(&pll_clock);

    /* Get the reset USB clock */
    cyhal_clock_get(&usb_rst_clock, &CYBSP_USB_CLK_DIV_obj);
}

/*******************************************************************************
* Function Name: audio_app_update_codec_volume
********************************************************************************
* Summary:
*   Update the audio codec volume by sending an I2C message.
*******************************************************************************/
void audio_app_update_codec_volume(void)
{
    uint8_t touch_audio_control_status = 0;
    cy_en_usb_dev_ep_state_t epState;

    /*
     Steps of 1dB, where:
 *        Minimum volume: -73dB (0x30)
 *        Maximum volume: +6dB  (0x7F)
 *        Mute: (0x00~0x2F)
 	 */

    //Windows conversion calculation
	 ulong usbVolume = abs( (int8_t)usb_comm_cur_volume[1]  +128  ) ;
	 uint windows_output = 48u + ( usbVolume / (float)255u * 79u);


	 //Change codec volume
	 	 uint output = 0;
	 	if( audio_app_prev_volume != volume_value ){
	 	     audio_app_prev_volume = volume_value;

	 		 output = volume_value;
	 		 if(volume_value < 80){
	 			 output = 80;
	 		 }
	     	mtb_wm8960_adjust_heaphone_output_volume(output);
	    }


	 //Nebunie
	 if(windows_output < output ){
		    touch_audio_control_status = AUDIO_HID_REPORT_VOLUME_UP;
	 }
	 else if(windows_output > output ){
		    touch_audio_control_status = AUDIO_HID_REPORT_VOLUME_DOWN;
	 }
	//Change windows volume


	 if(abs(windows_output - output ) > 5 && touch_audio_control_status != 0){
		epState = Cy_USBFS_Dev_Drv_GetEndpointState(CYBSP_USBDEV_HW, AUDIO_HID_ENDPOINT, &usb_drvContext);

		/* Check that endpoint is ready for operation */
		if ((CY_USB_DEV_EP_IDLE == epState) || (CY_USB_DEV_EP_COMPLETED == epState))
		{
			audio_app_control_report = touch_audio_control_status;

			/* Send out report data */
			Cy_USB_Dev_WriteEpNonBlocking(AUDIO_HID_ENDPOINT,
				  &audio_app_control_report,
				  AUDIO_HID_REPORT_SIZE,
				  &usb_devContext);
		}
	 }

}

/*******************************************************************************
* Function Name: audio_app_update_sample_rate
********************************************************************************
* Summary:
*   Update the sample rate of the audio streaming.
*******************************************************************************/
void audio_app_update_sample_rate(void)
{
    /* Check if need to change sample rate. */
    if ((usb_comm_new_sample_rate != 0) &&
        (usb_comm_new_sample_rate != audio_app_current_sample_rate))
    {
        /* Capture the new sample rate */
        audio_app_current_sample_rate = usb_comm_new_sample_rate;

        /* Update feedback sample rate */
        audio_feed_update_sample_rate(audio_app_current_sample_rate);

        /* Update Audio In sample rate */
        audio_in_update_sample_rate(audio_app_current_sample_rate);

        /* Disable the I2S block */
        cyhal_i2s_stop_tx(&i2s);
        cyhal_i2s_stop_rx(&i2s);
        
        /* Disable the codec */
        mtb_wm8960_deactivate();

        /* Set the new clock rate */
        audio_app_set_clock(audio_app_current_sample_rate);

        /* Re-enable the codec */
        mtb_wm8960_activate();

        /* Re-enable the I2S FIFOs */
        if (usb_comm_enable_out_streaming)
        {
            cyhal_i2s_start_tx(&i2s);
            cyhal_i2s_start_rx(&i2s);
        }
        if (usb_comm_enable_in_streaming)
        {
            cyhal_i2s_start_rx(&i2s);
        }
    }

    usb_comm_enable_feedback = true;
}

/*******************************************************************************
* Function Name: audio_app_set_clock
********************************************************************************
* Summary:
*   Update the PLL clock to achieve the desired sample rate.
*
* Parameters:
*   sample_rate: new sample rate to be enforced.
*
*******************************************************************************/
void audio_app_set_clock(uint32_t sample_rate)
{
    /* Wait till the CapSense is ready */
    while (!touch_is_ready())
    {
        vTaskDelay(1);
    };

    switch (sample_rate)
    {
        case AUDIO_SAMPLING_RATE_48KHZ:
        {
            cyhal_clock_set_frequency(&pll_clock, PLL_FREQ_FOR_48KHZ, &tolerance_0_p);
            break;
        }
        case AUDIO_SAMPLING_RATE_44KHZ:
        {
            cyhal_clock_set_frequency(&pll_clock, PLL_FREQ_FOR_44KHZ, &tolerance_0_p);
            break;
        }
        default:
            break;
    }

    /* Update the USB Reset clock based on the new frequency */
    cyhal_clock_set_frequency(&usb_rst_clock, USB_CLK_RESET_HZ, &tolerance_1_p);

    /* Set flag to indicate that the clock was configured */
    usb_comm_clock_configured = true;

    /* Update baseline to compensate change in the clock */
    touch_update_baseline();
}

/*******************************************************************************
* Function Name: audio_app_touch_events
********************************************************************************
* Summary:
*  Handle touch events from CapSense.
*
* Parameters:
*  widget: CapSense element ID
*  event: type of event
*  value: slider position (if a slider event)
*
*
*******************************************************************************/
void audio_app_touch_events(uint32_t widget, touch_event_t event, uint32_t value)
{
    uint8_t touch_audio_control_status = 0;
    cy_en_usb_dev_ep_state_t epState;

    switch (widget)
    {
        case CY_CAPSENSE_LINEARSLIDER0_WDGT_ID:
            /* Check the direction to change the volume */
            if (event == TOUCH_SLIDE_RIGHT)
            {
                touch_audio_control_status = AUDIO_HID_REPORT_VOLUME_UP;
            }
            if (event == TOUCH_SLIDE_LEFT)
            {
                touch_audio_control_status = AUDIO_HID_REPORT_VOLUME_DOWN;
            }
            break;
        case CY_CAPSENSE_BUTTON0_WDGT_ID:
            /* Play/Pause on Button 0 lift */
            if ( event == TOUCH_LIFT)
            {
                touch_audio_control_status = AUDIO_HID_REPORT_PLAY_PAUSE;
            }
            break;
        case CY_CAPSENSE_BUTTON1_WDGT_ID:
            /* Stop on Button 1 lift */
            if ( event == TOUCH_LIFT)
            {
                touch_audio_control_status = AUDIO_HID_REPORT_STOP;
            }
            break;
        default:
            break;
    }

    epState = Cy_USBFS_Dev_Drv_GetEndpointState(CYBSP_USBDEV_HW, AUDIO_HID_ENDPOINT, &usb_drvContext);

    /* Check that endpoint is ready for operation */
    if ((CY_USB_DEV_EP_IDLE == epState) || (CY_USB_DEV_EP_COMPLETED == epState))
    {
        audio_app_control_report = touch_audio_control_status;

        /* Send out report data */
        Cy_USB_Dev_WriteEpNonBlocking(AUDIO_HID_ENDPOINT,
              &audio_app_control_report,
              AUDIO_HID_REPORT_SIZE,
              &usb_devContext);
    }
}

/*******************************************************************************
* Function Name: _transmit
********************************************************************************
* Summary:
*  I2C Master function to transmit data to the given address.
*
* Parameters:
*  reg_addr: address to be updated
*  data: 8-bit data to be written in the register
*
* Return:
*  cy_rslt_t - I2C master transaction error status.
*              Returns CY_RSLT_SUCCESS if succeeded.
*
*******************************************************************************/
cy_rslt_t _transmit(uint8_t reg_addr, uint8_t data)
{
    cy_rslt_t result;
    uint8_t buffer[2];

    buffer[0] = reg_addr;
    buffer[1] = data;

    /* Send the data over the I2C */
    result = cyhal_i2c_master_write(&mi2c, 12, buffer,4 , MI2C_TIMEOUT_MS, true);

    return result;
}

/* [] END OF FILE */
