/* SPDX-License-Identifier: BSD-3-Clause-Clear */
/*
 * Copyright (C) 2022, by Kevin Routley
 *
 * The servo processor. This is executed in a separate thread from the GUI.
 */

#include <pthread.h>
extern "C"
{
#include "PCA9685_servo_driver.h"
}

#define SLEEP 100

extern double servoTDegree;
extern double servoBDegree;
double ServoUpDegree = 90;
double ServoDownDegree = 90;

void* servo_func(void *p)
{
    while(true)
    {
        usleep(SLEEP);
        
        // Check to see if GUI values have changed vs local
        if (ServoUpDegree != servoTDegree)
        {
            setServoDegree(SERVO_UP_CH, servoTDegree);
            ServoUpDegree = servoTDegree; // TODO get range-checked value
        }

        if (ServoDownDegree != servoBDegree)
        {
            setServoDegree(SERVO_DOWN_CH, servoBDegree);
            ServoDownDegree = servoBDegree; // TODO get range-checked value
        }            
    }
    
    return nullptr;
}

pthread_t servo_thread;

pthread_t *fire_servo_thread(int argc, char ** argv)
{        
    // Initialize the servos
    PCA9685_init(I2C_ADDR);
    PCA9685_setPWMFreq(60);  // Servos run at ~60 Hz updates
    setServoDegree(SERVO_UP_CH, ServoUpDegree);
    setServoDegree(SERVO_DOWN_CH, ServoDownDegree);
    delay_ms(1000);
    
    
    pthread_create(&servo_thread, 0, servo_func, nullptr);
    pthread_detach( servo_thread );
    return &servo_thread;
}

