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
 */

/**
 * \file
 *         Smart Thermostat Implementation
 * \author
 *         Alessandro Pozone <alessandro.pozone@hotmail.com>
 */

#include "contiki.h"
#include "contiki-net.h"
#include "thermostat.h"

#include "dev/leds.h"
#include <random.h>
#include <stdio.h>
#include <stdbool.h>
#include "er-coap-13.h"
/*---------------------------------------------------------------------------*/

static struct etimer timer;
static int temperature;
static thermostat_status status;

RESOURCE(discover, METHOD_GET, ".well-known/core", "");
void discover_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  char temp[100];
  int index = 0;
  index += sprintf(temp + index, "%s,", "</status>;n=\"Status\"");
  index += sprintf(temp + index, "%s,", "</temperature>;n=\"Temperature\"");
  index += sprintf(temp + index, "%s,", "</set>;n=\"Set Device Mode\"");

  REST.set_response_payload(response, (uint8_t*)temp, strlen(temp));
  REST.set_header_content_type(response, APPLICATION_LINK_FORMAT);
}

RESOURCE(get_status, METHOD_GET, "status", "title=\"Status\"");
void get_status_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{  
    char message[REST_MAX_CHUNK_SIZE];
    sprintf(message, "{\"ac\":%s,\"heater\":%s,\"vent\":%s}", status.air_conditioning?"true":"false", status.heating?"true":"false", status.ventilation?"true":"false");

    REST.set_header_content_type(response, REST.type.APPLICATION_JSON);
    REST.set_response_payload(response, (uint8_t*)message, strlen(message));
}

PERIODIC_RESOURCE(get_temperature, METHOD_GET, "temperature", "title=\"Temperature\";obs", 5*CLOCK_SECOND);
void get_temperature_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
    static char content[REST_MAX_CHUNK_SIZE];
    REST.set_header_content_type(response, REST.type.APPLICATION_JSON);
    REST.set_response_payload(response, snprintf(content, sizeof(content), "{\"temperature\":%d}", temperature), strlen(content));
}

void get_temperature_periodic_handler(resource_t *r)
{
  static uint16_t obs_counter = 0;
  static char content[REST_MAX_CHUNK_SIZE];

  ++obs_counter;

  coap_packet_t notification[1];
  coap_init_message(notification, COAP_TYPE_NON, REST.status.OK, 0 );
  coap_set_payload(notification, content, snprintf(content, sizeof(content), "{\"temperature\":%d}", temperature));

  REST.notify_subscribers(r, obs_counter, notification);
}

RESOURCE(set_device, METHOD_POST | METHOD_PUT, "set", "title=\"Device: ?device=ac|heater|ventilation, POST/PUT mode=on|off\";rt=\"Control\";ct=50");
void set_device_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
    size_t len = 0;
    thermostat_status new_status = status;
    bool new_mode;
    const char *device = NULL;
    const char *mode = NULL;
    const char dev[11];
    uint8_t led = 0;
    bool success;
    static char content[40];

    if ((len = REST.get_post_variable(request, "mode", &mode)))
    {
        if (strncmp(mode, "on", len) == 0)
        {
            new_mode = true;
            success = true;
        }
        else if (strncmp(mode, "off", len) == 0)
        {
            new_mode = false;
            success = true;
        }
    }

    if (success && (len = REST.get_query_variable(request, "device", &device)))
    {
        strncpy(dev, device, len);
        if (strncmp(device, "ac", len) == 0)
        {
            led = LEDS_BLUE;
            new_status.air_conditioning = new_mode;
        }
        else if (strncmp(device, "heater", len) == 0)
        {
            led = LEDS_RED;
            new_status.heating = new_mode;
        }
        else if (strncmp(device, "ventilation", len) == 0)
        {
            led = LEDS_GREEN;
            new_status.ventilation = new_mode;
        }
        else
        {
            success = false;
        }
    }
    else
    {
        success = false;
    }
        
    if (!success || (new_status.air_conditioning && new_status.heating))
    {
        char message[REST_MAX_CHUNK_SIZE];
        sprintf(message, "{\"status\":\"error\"}");
        printf("Error while changing %s to mode: %s\n", dev, mode);
        REST.set_header_content_type(response, REST.type.APPLICATION_JSON);
        REST.set_response_payload(response, (uint8_t*)message, strlen(message));
        REST.set_response_status(response, REST.status.BAD_REQUEST);
    }
    else
    {
        status = new_status;
        if (new_mode)
            leds_on(led);
        else
            leds_off(led);

        char message[REST_MAX_CHUNK_SIZE];

        printf("Changing %s to mode: %s\n", dev, mode);
        REST.set_header_content_type(response, REST.type.APPLICATION_JSON);
        sprintf(message, "{\"status\":\"ok\"}");
        REST.set_response_payload(response, (uint8_t*)message, strlen(message));
    }
}

static void update_temperature(int *temperature, thermostat_status *status)
{
    // printf("Current Temperature: %d.\n", *temperature);
    // printf("Current Status:\n");
    // printf("AC: %d\n", status->air_conditioning);
    // printf("Heating: %d\n", status->heating);
    // printf("Ventilation: %d\n", status->ventilation);
    int oldTemp = *temperature;
    int multiplier = status->ventilation ? ventilation_multiplier : 1;

    if (status->air_conditioning)
        *temperature += (air_conditioning_factor * multiplier);
    else if (status->heating)
        *temperature += (heating_factor * multiplier);

    if(oldTemp != *temperature)
        printf("Updating Temperature to: %d\n", *temperature);    
}

PROCESS(sensor_smtthmst_process, "Smart Thermostat");
AUTOSTART_PROCESSES(&sensor_smtthmst_process);

PROCESS_THREAD(sensor_smtthmst_process, ev, data)
{
    PROCESS_BEGIN();
    random_init(rimeaddr_node_addr.u8[3]);
    temperature = (random_rand() % 20) + 11;
    printf("Setting initial random temperature to: %d degrees.\n", temperature);

    /* Initialize the REST engine. */
    rest_init_engine();
    rest_activate_resource(&resource_get_status);
    rest_activate_resource(&resource_set_device);
    rest_activate_periodic_resource(&periodic_resource_get_temperature);

    while (1)
    {
        etimer_set(&timer, CLOCK_SECOND * 20);

        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

        if (etimer_expired(&timer))
        {
            update_temperature(&temperature, &status);
        }
    } //end of while

    PROCESS_END();
}
