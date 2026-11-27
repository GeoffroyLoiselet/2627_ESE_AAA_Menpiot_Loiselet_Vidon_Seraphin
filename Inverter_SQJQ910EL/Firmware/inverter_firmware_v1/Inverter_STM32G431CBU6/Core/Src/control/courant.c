/*
 * courant.c
 *
 *  Created on: 27 nov. 2026
 *      Author: geoff
 *
 *  Current control loop (PI) with anti-windup.
 *  Output u in [-1, 1] is the fraction of Vbus applied to the motor,
 *  duty = 0.5 + 0.5*u (same convention as the rest of the project).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "main.h"
#include "control/courant.h"
#include "control/control.h"
#include "control/motor.h"
#include "user_interface/shell.h"

h_current_t h_current1;

void currentInit(h_current_t *h, h_motor_t *hmotor, float sign, float kp, float ki, float u_max, float limit){
    h->hmotor = hmotor;
    h->sign = sign;
    h->kp = kp;
    h->ki = ki;
    h->ts = CURRENT_LOOP_TS;
    h->u_max = u_max;
    h->limit = limit;

    h->target = 0;
    h->measured = 0;
    h->integral = 0;
    h->u = 0;
    h->enabled = 0;
    h->fault = 0;

    shellAdd(&hshell1, "setCurrentTarget",  currentShellSetTarget,  "set current target in A");
    shellAdd(&hshell1, "setCurrentCoeff",   currentShellSetCoeff,   "setCurrentCoeff kp|ki value");
    shellAdd(&hshell1, "setCurrentLimit",   currentShellSetLimit,   "set current trip limit in A");
    shellAdd(&hshell1, "setCurrentSign",    currentShellSetSign,    "set sensor sign 1 or -1");
    shellAdd(&hshell1, "setCurrentEnable",  currentShellEnable,     "start PWM + current loop");
    shellAdd(&hshell1, "setCurrentDisable", currentShellDisable,    "stop current loop + PWM");
    shellAdd(&hshell1, "getCurrent",        currentShellGet,        "print current loop state");
}

/* Called at 40 kHz from HAL_ADC_ConvCpltCallback */
void currentUpdate(h_current_t *h, float measured_raw){
    h->measured = h->sign * measured_raw;

    if(!h->enabled) return;

    if(fabsf(h->measured) > h->limit){
        currentTrip(h);
        return;
    }

    float e = h->target - h->measured;
    float i_new = h->integral + h->ki * h->ts * e;
    float u = h->kp * e + i_new;

    /* Anti-windup: saturate and stop integrating further into the saturation */
    if(u > h->u_max){
        u = h->u_max;
        if(e > 0) i_new = h->integral;
    }
    else if(u < -h->u_max){
        u = -h->u_max;
        if(e < 0) i_new = h->integral;
    }

    h->integral = i_new;
    h->u = u;

    motorSetDutyImmediate(h->hmotor, 0.5f + 0.5f*u);
}

void currentEnable(h_current_t *h){
    h_control1.pwm_status = PWM_DISABLED;   /* the speed loop must not drive the PWM */

    h->integral = 0;
    h->u = 0;
    h->fault = 0;

    motorSetDutyImmediate(h->hmotor, 0.5f); /* 0 V across the motor */
    motorStart(h->hmotor);
    h->enabled = 1;
}

void currentDisable(h_current_t *h){
    h->enabled = 0;        /* first, so the ISR stops writing the PWM */
    h->target = 0;
    h->integral = 0;
    h->u = 0;
    motorStop(h->hmotor);
}

void currentTrip(h_current_t *h){
    h->enabled = 0;
    h->fault = 1;
    h->target = 0;
    h->integral = 0;
    h->u = 0;
    motorStop(h->hmotor);
}

/* ---------------------------- Shell commands ---------------------------- */

int currentShellSetTarget(h_shell_t* h_shell, int argc, char** argv){
    if(argc != 2) return 1;
    float t = atof(argv[1]);
    if(fabsf(t) > h_current1.limit){
        printf("Target above limit (%.2f A)\r\n", h_current1.limit);
        return 1;
    }
    h_current1.target = t;
    return 0;
}

int currentShellSetCoeff(h_shell_t* h_shell, int argc, char** argv){
    if(argc != 3) return 1;
    float v = atof(argv[2]);
    if(strcmp(argv[1], "kp") == 0){ h_current1.kp = v; return 0; }
    if(strcmp(argv[1], "ki") == 0){ h_current1.ki = v; return 0; }
    return 1;
}

int currentShellSetLimit(h_shell_t* h_shell, int argc, char** argv){
    if(argc != 2) return 1;
    float l = atof(argv[1]);
    if(l <= 0 || l > CURRENT_HARD_LIMIT_A) return 1;
    h_current1.limit = l;
    return 0;
}

int currentShellSetSign(h_shell_t* h_shell, int argc, char** argv){
    if(argc != 2) return 1;
    int s = atoi(argv[1]);
    if(s != 1 && s != -1) return 1;
    h_current1.sign = (float)s;
    return 0;
}

int currentShellEnable(h_shell_t* h_shell, int argc, char** argv){
    if(argc != 1) return 1;
    currentEnable(&h_current1);
    return 0;
}

int currentShellDisable(h_shell_t* h_shell, int argc, char** argv){
    if(argc != 1) return 1;
    currentDisable(&h_current1);
    return 0;
}

int currentShellGet(h_shell_t* h_shell, int argc, char** argv){
    if(argc != 1) return 1;
    printf("I = %.3f A, target = %.3f A, u = %.3f, Kp = %.4f, Ki = %.2f, limit = %.1f A, loop %s%s\r\n",
           h_current1.measured, h_current1.target, h_current1.u,
           h_current1.kp, h_current1.ki, h_current1.limit,
           h_current1.enabled ? "ON" : "OFF",
           h_current1.fault ? " (FAULT)" : "");
    return 0;
}
