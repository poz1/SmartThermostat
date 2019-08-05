/*
 * Copyright (c) 2019, Alessandro Pozone.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *
 *
 */

#include <stdlib.h>
#include "contiki.h"
#include "lib/sensors.h"
#include "thermostat.h"

static int temperature;
static thermostat_status status;

/*---------------------------------------------------------------------------*/
static int value(int type)
{
    return temperature;
}

void update_temperature()
{
    printf("Current Temperature: %d.\n", temperature);
    printf("Current Status:\n");
    printf("AC: %d\n", status->air_conditioning);
    printf("Heating: %d\n", status->heating);
    printf("Ventilation: %d\n", status->ventilation);

    int multiplier = status->ventilation ? ventilation_multiplier : 1;

    if (status->air_conditioning)
        temperature += (air_conditioning_factor * multiplier);
    else if (status->heating)
        temperature += (heating_factor * multiplier);
}

static int configure(thermostat_status *new_status)
{
  status = &new_status;
}

/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(faketemp_sensor, "faketemp", value, configure, status);
