#ifndef AUDIO_H
#define AUDIO_H

/*******************************************************************************
* Constants from USB Audio Descriptor
*******************************************************************************/
#define AUDIO_OUT_ENDPOINT_SIZE         (294U)
#define AUDIO_IN_ENDPOINT_SIZE          (294U)
#define AUDIO_FEEDBACK_ENDPOINT_SIZE    (3U)

#define AUDIO_FRAME_DATA_SIZE           (96u)
#define AUDIO_DELTA_VALUE               (2u)
#define AUDIO_MAX_DATA_SIZE             (AUDIO_FRAME_DATA_SIZE + AUDIO_DELTA_VALUE)

#define AUDIO_CONTROL_INTERFACE         (0x00U)
#define AUDIO_CONTROL_IN_ENDPOINT       (6U)
#define AUDIO_CONTROL_FEATURE_UNIT_IDX  (0x02U)
#define AUDIO_CONTROL_FEATURE_UNIT      ((AUDIO_CONTROL_FEATURE_UNIT_IDX << 8U) | (AUDIO_CONTROL_INTERFACE))

#define AUDIO_STREAMING_OUT_INTERFACE   (1U)
#define AUDIO_STREAMING_OUT_ALTERNATE   (1U)
#define AUDIO_STREAMING_IN_INTERFACE    (2U)
#define AUDIO_STREAMING_IN_ALTERNATE    (1U)

#define AUDIO_STREAMING_OUT_ENDPOINT    (1U)
#define AUDIO_STREAMING_IN_ENDPOINT     (2U)
#define AUDIO_FEEDBACK_IN_ENDPOINT      (3U)

#define AUDIO_STREAMING_OUT_ENDPOINT_ADDR   (0x01U)
#define AUDIO_STREAMING_IN_ENDPOINT_ADDR    (0x82U)
#define AUDIO_FEEDBACK_IN_ENDPOINT_ADDR     (0x83U)

#define AUDIO_FEED_SINGLE_SAMPLE            (0x000800U)

#define AUDIO_STREAMING_EPS_NUMBER          (0x2U)
#define AUDIO_SAMPLE_FREQ_SIZE              (3U)
#define AUDIO_SAMPLE_DATA_SIZE              (3U)

#define AUDIO_FEATURE_UNIT_MASTER_CHANNEL   (0U)

#define AUDIO_HID_ENDPOINT                  (0x4u)
#define AUDIO_HID_REPORT_SIZE               (1u)
#define AUDIO_HID_REPORT_VOLUME_UP          (0x20u)
#define AUDIO_HID_REPORT_VOLUME_DOWN        (0x40u)
#define AUDIO_HID_REPORT_PLAY_PAUSE         (0x01u)
#define AUDIO_HID_REPORT_STOP               (0x08u)

#define AUDIO_VOLUME_SIZE   (2U)

#define AUDIO_VOL_RES_MSB   (0x00u)
#define AUDIO_VOL_RES_LSB   (0x01u)

#define AUDIO_SAMPLING_RATE_48KHZ   (48000U)
#define AUDIO_SAMPLING_RATE_44KHZ   (44100U)
#define AUDIO_SAMPLING_RATE_32KHZ   (32000U)
#define AUDIO_SAMPLING_RATE_22KHZ   (22050U)
#define AUDIO_SAMPLING_RATE_16KHZ   (16000U)

#endif /* AUDIO_H */

/* [] END OF FILE */
