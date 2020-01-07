/****************************************************************************************
* \file         MqttBuffer.h
* \brief        MQTT client buffer mechanism functions
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

#ifndef GOCONTROLL_MQTT_INC_MQTTBUFFER_H_
#define GOCONTROLL_MQTT_INC_MQTTBUFFER_H_


void 	MqttBuffer_SendQueue(void);
void 	MqttBuffer_SendQueueSendSucces(void);
void 	MqttBuffer_LoadSendQueue(uint8_t bufferPointer, uint8_t length, uint8_t priority);
void 	MqttBuffer_ExtractReceiveBuffer(void);

void 	MqttBuffer_AddByteToSendBuffer(uint8_t byte);
uint8_t MqttBuffer_ReadByteFromSendBuffer(uint8_t bufferPointer);
uint8_t MqttBuffer_GetWritePointerSendBuffer(void);

void 	MqttBuffer_AddByteToReceiveBuffer(uint8_t byte);
uint8_t MqttBuffer_ReadByteFromReceiveBuffer(uint8_t bufferPointer);
uint8_t MqttBuffer_GetWritePointerReceiveBuffer(void);

uint8_t MqttBuffer_AddStringToSendBuffer(char* string, uint8_t length);
uint8_t MqttBuffer_ExtractStringfromReceiveBuffer(char* string, uint8_t startLocation, uint8_t length);
void 	MqttBuffer_Initialize(void);

#endif /* GOCONTROLL_MQTT_INC_MQTTBUFFER_H_ */
