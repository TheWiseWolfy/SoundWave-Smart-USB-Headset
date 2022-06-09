/*
 * ai.h
 *
 *  Created on: 2 Jun 2022
 *      Author: Lucian
 */

#ifndef INCLUDE_AI_H_
#define INCLUDE_AI_H_

#include "cyabs_rtos.h"
#include "mtb_ml_model.h"

#define AI_BUFFER_SIZE 3675

extern mtb_ml_model_t *smart_wave_obj;
extern float data_feed[AI_BUFFER_SIZE];
extern volatile size_t recorded_size;

/*******************************************************************************
* Functions
*******************************************************************************/
cy_rslt_t ai_init(void);
void ai_process(void *arg);


#endif /* INCLUDE_AI_H_ */
