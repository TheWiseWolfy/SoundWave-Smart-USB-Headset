/*
 * ai.c
 *
 *  Created on: 2 Jun 2022
 *      Author: Lucian
 */
#include "ai.h"

#include "mtb_ml_utils.h"
#include "mtb_ml_common.h"
#include "cybsp.h"
#include "cy_retarget_io.h"

#include "audio_app.h"

#include MTB_ML_INCLUDE_MODEL_FILE(SMART_WAVE)

//Constant variables

const int psoc_start = 48;
const int psoc_stop = 127;

uint volume_denormalization(float vol){
	float finalVolume = (psoc_stop - psoc_start) * vol + psoc_start;
    return (uint)finalVolume;
}

/*******************************************************************************
* Global Variables
*******************************************************************************/
float data_feed[AI_BUFFER_SIZE];
volatile size_t recorded_size = 0;

/* Model information */
mtb_ml_model_t *smart_wave_obj;

//Init ai
cy_rslt_t ai_init(void){
    cy_rslt_t result;

    mtb_ml_model_bin_t smart_wave_bin = {MTB_ML_MODEL_BIN_DATA(SMART_WAVE)};

    result = mtb_ml_model_init(&smart_wave_bin, NULL, &smart_wave_obj);

    if(CY_RSLT_SUCCESS != result)
    {
        return result;
    }

	#if !COMPONENT_ML_FLOAT32
		/* Set the q-factor */
		mtb_ml_model_set_input_q_fraction_bits(magic_wand_obj, QFORMAT_VALUE);
	#endif

	cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX, CY_RETARGET_IO_BAUDRATE);

    return result;
}

void ai_process(void *arg){

    (void)arg;

    /* Regression pointers */
     MTB_ML_DATA_T *input_reference;
     MTB_ML_DATA_T *result_buffer;

     /* Data processed in floating point */

    int model_output_size = mtb_ml_model_get_output_size(smart_wave_obj);

   // printf("%d", model_output_size);

    for(int i = 0; i < AI_BUFFER_SIZE; i++){
    	data_feed[i]=0;
    }

	for(;;){
        /* Feed the Beast */
        input_reference = (MTB_ML_DATA_T *) data_feed;

        MTB_ML_DATA_T result_buffer;
        mtb_ml_model_run(smart_wave_obj, input_reference, &result_buffer);

        if( recorded_size >= AI_BUFFER_SIZE){
        	recorded_size = 0;
        }

        volume_value = volume_denormalization((float)result_buffer);
	}
}


