/****************************************************************************************
* \file         MqttInterface.h
* \brief        MQTT interface and buffer functions
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

#ifndef GOCONTROLL_INC_MQTTINTERFACE_H_
#define GOCONTROLL_INC_MQTTINTERFACE_H_

/* General includes for data types and string functions */
#include "stdio.h"

/* Define the connection interface that is used by MQTT */
#define MQTTNETCONNINTERFACE		0
/* GOcontroll AT stack for use with SIM800/7000 */
#define MQTTGOCONTROLLATINTERFACE	1
/* Define other interfaces that need to be used */


uint8_t MqttInterface_ConnectToServer(char* address, uint16_t port);
uint8_t MqttInterface_SendToServer(uint8_t* data, uint8_t dataLocation, uint8_t length);
#if MQTTNETCONNINTERFACE == 1
uint8_t MqttInterface_ReceiveFromServer(void);
#endif
#if MQTTGOCONTROLLATINTERFACE == 1
void MqttInterface_ReceiveFromServer(uint8_t* bufferLocation, uint8_t bufferPointer, uint8_t length);
#endif

int32_t MqttInterface_ExtractValueFromString(uint8_t numberOfCharacters, char *dataReceived);



#endif /* GOCONTROLL_INC_MQTTINTERFACE_H_ */
