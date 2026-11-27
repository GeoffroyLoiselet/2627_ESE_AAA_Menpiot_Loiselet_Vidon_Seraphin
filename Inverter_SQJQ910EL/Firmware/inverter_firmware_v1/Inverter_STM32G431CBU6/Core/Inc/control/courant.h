/*
 * courant.h
 *
 *  Created on: 27 nov. 2026
 *      Author: geoff
 * Current control loop (PI), runs at the ADC/PWM rate (40 kHz) from the
 * ADC DMA callback.
 */

#ifndef INC_CONTROL_COURANT_H_
#define INC_CONTROL_COURANT_H_


#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "main.h"
#include "stm32g4xx_hal.h"
#include "control/motor.h"
#include "user_interface/shell.h"

/* TIM1: 160 MHz / (PSC+1 = 4) / (ARR+1 = 1000) = 40 kHz, ADC triggered by TIM1 TRGO */
#define CURRENT_LOOP_TS          (1.0f/40000.0f)
#define CURRENT_ADC_INDEX        3        /* rank 4 in the ADC scan = U_I */
#define CURRENT_HARD_LIMIT_A     20.0f    /* absolute protection, always active */

typedef struct h_current_struct {
	h_motor_t* hmotor;

	float sign;        /* +1 or -1 : sensor sign vs. positive duty */
	float kp;          /* duty fraction per A  (duty fraction = Vapplied / Vbus) */
	float ki;          /* duty fraction per A per second */
	float ts;          /* s */
	float u_max;       /* saturation of the PI output, 0..1 */
	float limit;       /* A, trip threshold when the loop is enabled */

	float target;      /* A */
	float measured;    /* A, signed with 'sign' */
	float integral;    /* integrator state, duty fraction */
	float u;           /* PI output, -1..1 */

	volatile uint8_t enabled;
	volatile uint8_t fault;
} h_current_t;

extern h_current_t h_current1;

void currentInit(h_current_t *h, h_motor_t *hmotor, float sign, float kp, float ki, float u_max, float limit);
void currentUpdate(h_current_t *h, float measured_raw);   /* call from the ADC callback */
void currentEnable(h_current_t *h);
void currentDisable(h_current_t *h);
void currentTrip(h_current_t *h);                         /* safe to call from an ISR */
static inline uint8_t currentIsEnabled(const h_current_t *h){ return h->enabled; }

int currentShellSetTarget(h_shell_t* h_shell, int argc, char** argv);
int currentShellSetCoeff(h_shell_t* h_shell, int argc, char** argv);
int currentShellSetLimit(h_shell_t* h_shell, int argc, char** argv);
int currentShellSetSign(h_shell_t* h_shell, int argc, char** argv);
int currentShellEnable(h_shell_t* h_shell, int argc, char** argv);
int currentShellDisable(h_shell_t* h_shell, int argc, char** argv);
int currentShellGet(h_shell_t* h_shell, int argc, char** argv);


#endif /* INC_CONTROL_COURANT_H_ */
