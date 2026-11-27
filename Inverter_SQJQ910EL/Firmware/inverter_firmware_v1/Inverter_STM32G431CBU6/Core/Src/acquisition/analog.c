#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "acquisition/analog.h"
#include "user_interface/shell.h"
#include "control/motor.h"
#include "control/courant.h"
#include "adc.h"
#include "main.h"
#include "tim.h"

h_analoginput_t h_analogInput1;

HAL_StatusTypeDef analogInputInit(h_analoginput_t *analogInput, ADC_HandleTypeDef* hadc, int nb_channels){
	analogInput->hadc = hadc;
	analogInput->nb_channels = nb_channels;
    HAL_ADC_Start_DMA(analogInput->hadc, (uint32_t*) analogInput->raw_values, (uint32_t) analogInput->nb_channels);
    return HAL_OK;
}

/* Raw ADC -> physical values (V or A), cheap enough to run at 40 kHz */
HAL_StatusTypeDef analogInputUpdate(h_analoginput_t *analogInput)
{
	for(int idx = 0; idx < analogInput->nb_channels; idx++){
		analogInput->values[idx] = (((float)analogInput->raw_values[idx]*3.3f/4096.0f)-analogInput->offset[idx])*analogInput->gain[idx];
	}
    return HAL_OK;
}

HAL_StatusTypeDef analogInputPrint(h_analoginput_t *analogInput)
{
	return analogInputUpdate(analogInput);
}

/* Called at the end of each 6-channel scan, i.e. at 40 kHz (TIM1 TRGO).
 * No printf / blocking call allowed here. */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc){
	if(hadc->Instance != ADC2) return;

	analogInputUpdate(&h_analogInput1);

	float i_u = h_analogInput1.values[CURRENT_ADC_INDEX];

	/* Absolute protection, active whatever the control mode */
	if(fabsf(i_u) > CURRENT_HARD_LIMIT_A){
		currentTrip(&h_current1);
		return;
	}

	currentUpdate(&h_current1, i_u);
}
