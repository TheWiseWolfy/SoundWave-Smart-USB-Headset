/*****************************************************************************
* File Name: audio_app.h
*
* Description: This file contains the function prototypes and constants used in
*  audio_app.c.
*
******************************************************************************
* (c) 2019-2020, Cypress Semiconductor Corporation. All rights reserved.
*******************************************************************************
* This software, including source code, documentation and related materials
* ("Software"), is owned by Cypress Semiconductor Corporation or one of its
* subsidiaries ("Cypress") and is protected by and subject to worldwide patent
* protection (United States and foreign), United States copyright laws and
* international treaty provisions. Therefore, you may use this Software only
* as provided in the license agreement accompanying the software package from
* which you obtained this Software ("EULA").
*
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software source
* code solely for use in connection with Cypress's integrated circuit products.
* Any reproduction, modification, translation, compilation, or representation
* of this Software except as specified above is prohibited without the express
* written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer of such
* system or application assumes all risk of such use and in doing so agrees to
* indemnify Cypress against all liability.
*****************************************************************************/
#ifndef AUDIO_APP_H
#define AUDIO_APP_H

#include "cyhal.h"
#include "audio.h"

/*******************************************************************************
* Constants
*******************************************************************************/
#define AUDIO_APP_MCLK_PIN          P5_0

#define PC_VOLUME_MSB_CODEC_OFFSET  64
#define PC_VOLUME_CODEC_COEFF       4096

/*******************************************************************************
* Externs
*******************************************************************************/
extern cyhal_i2s_t i2s;

extern uint volume_value;

/*******************************************************************************
* Functions
*******************************************************************************/
void audio_app_init(void);
void audio_app_process(void *arg);

#endif /* AUDIO_APP_H */
