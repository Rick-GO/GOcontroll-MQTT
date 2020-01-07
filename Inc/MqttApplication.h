/****************************************************************************************
* \file         MqttApplication.h
* \brief        Application functions to be used in customer application.
* \internal
*----------------------------------------------------------------------------------------
*                          C O P Y R I G H T
*----------------------------------------------------------------------------------------
*  Copyright 2019 (c)  by GOcontroll   http://www.GOcontroll.com      All rights reserved
*
*----------------------------------------------------------------------------------------
*                            L I C E N S E
*----------------------------------------------------------------------------------------
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
* 3. The name of the author may not be used to endorse or promote products
*    derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
* SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
* OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
* IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
* OF SUCH DAMAGE.
*
*
* Author: Rick Gijsberts  <rickgijsberts@gocontroll.com>
****************************************************************************************/

#ifndef GOCONTROLL_INC_MQTTAPPLICATION_H_
#define GOCONTROLL_INC_MQTTAPPLICATION_H_

#include "stdio.h"

typedef enum
{
MQTT_CONNECT_TIMEOUT	=	0x01,
MQTT_PING_TIMEOUT		=	0x02,
MQTT_SUBSCRIBE_TIMEOUT	=	0x03
} _mqttError;

uint8_t MqttApplication_ConnectToBroker (char* address, uint16_t port, char* clientID, uint16_t keepAlive, uint8_t connectionsFlag);
uint8_t MqttApplication_SubscribeToTopic(char* topic, uint8_t qos, char* data);
uint8_t MqttApplication_PublishToTopic(char* topic, uint8_t qos, uint8_t retain, char* data);
uint8_t MqttApplication_ExtractJsonString(char* data, char* key, int32_t* value);
void MqttApplication_Initialize(void);
void MqttApplication_ConnectionFail(_mqttError mqttError);

#endif /* GOCONTROLL_INC_MQTTAPPLICATION_H_ */
