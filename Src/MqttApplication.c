/****************************************************************************************
* \file         MqttApplication.c
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

/* General includes for data types and string functions */
#include "stdio.h"
#include "string.h"

/* MQTT stack includes */
#include "MqttStack.h"
#include "MqttInterface.h"
#include "MqttBuffer.h"
#include "MqttApplication.h"


/***************************************************************************************
** \brief	Function that connects to the Broker.
** \param	Address of the broker on which to connect to
** \param	Port from the broker to connect to
** \param	Pointer to the string that holds the client ID
** \param	Keep alive ping period in seconds
**  \param	Connection flags  (see MqttApplication.h for flags)
** \return	1 if information was ok, 0 if one of parameters was not correct
**
****************************************************************************************/
uint8_t MqttApplication_ConnectToBroker (char* address, uint16_t port, char* clientID, uint16_t keepAlive, uint8_t connectionsFlag)
{
	_mqttStackConnection * mqttStackConnection = MqttStack_ConnectionInformation();

	/* Check if address is not to long */
	if(strlen(address)>= MAXADDRESSCHARACTERS){return 0;}
	strcpy((char*)mqttStackConnection->mqttBroker.address,address);

	mqttStackConnection->mqttBroker.port = port;

	/* Check if client ID is not to long */
	if(strlen(clientID)>= MAXCLIENTIDCHARACTERS){return 0;}
	strcpy((char*)&mqttStackConnection->mqttBroker.clientId,clientID);

	mqttStackConnection->mqttBroker.keepAlive = keepAlive;

	mqttStackConnection->mqttBroker.connectionFlags = connectionsFlag;

	mqttStackConnection->infoComplete = MQTTTRUE;
	return 1;
}


/***************************************************************************************
** \brief	Function that subscribes to a topic.
** \param	Pointer to a string that contains the topic
** \param	The qos (0,1,2) that is required for the message delivery (NOTE: QOS2 not tested)
** \param	Pointer to data string that holds the data comming from the broker if other
** 			client publish to the subscribed topic.
** \return	1 if information was ok, 0 if one of parameters was not correct
**
****************************************************************************************/
uint8_t MqttApplication_SubscribeToTopic(char* topic, uint8_t qos, char* data)
{
	_mqttStackSubscribe* mqttStackSubscribe = MqttStack_SubscribeInformation();

	/* Check if not already the maximum number of subscribtions are reached */
	if(mqttStackSubscribe->topicCounter == MAXSUBSCIPTIONS){return 0;}

	/* Check if topic is not to long */
	if(strlen(topic) >= MAXTOPICCHARACTERS){return 0;}
	/* Check if QOS has a value between 0 and 2 */
	if(qos > 2){return 0;}
	/* Check if data pointer is not empty */
	if(data == NULL){return 0;}

	strcpy((char*)&mqttStackSubscribe->mqttSubscribe[mqttStackSubscribe->topicCounter].topic, topic);
	mqttStackSubscribe->mqttSubscribe[mqttStackSubscribe->topicCounter].qos = qos;
	mqttStackSubscribe->mqttSubscribe[mqttStackSubscribe->topicCounter++].data = data;

	return 1;
}


/***************************************************************************************
** \brief	Function that can be used to publish to a topic.
** \param	Pointer to a string that contains the topic
** \param	The qos (0,1,2) that is required for the message delivery (NOTE: QOS2 not tested)
** \param	Retain flag (0-1) 1 keep on broker, 0 forget message
** \param	Pointer to data string that holds the data to publish
** \return	1 if information was ok, 0 if one of parameters was not correct
**
****************************************************************************************/
uint8_t MqttApplication_PublishToTopic(char* topic, uint8_t qos, uint8_t retain, char* data)
{
	_mqttStackPublish* mqttStackPublish = MqttStack_PublishInformation();

	/* Check if string from topic is not to long */
	if(strlen(topic) >= MAXTOPICCHARACTERS){return 0;}
	/* Check if QOS has a value between 0 and 2 */
	if(qos > 2){return 0;}
	/* Check if retain flag is 1 or 0 */
	if(retain > 1){return 0;}
	/* Check if data pointer is not empty */
	if(data == NULL){return 0;}

	strcpy((char*)&mqttStackPublish->mqttPublish[mqttStackPublish->writePointer].topic, topic);
	mqttStackPublish->mqttPublish[mqttStackPublish->writePointer].qos = qos;
	mqttStackPublish->mqttPublish[mqttStackPublish->writePointer].retain = retain;
	mqttStackPublish->mqttPublish[mqttStackPublish->writePointer].data = data;

	/* Increase the write pointer to tell the stack a new message is ready to send*/
	if(++mqttStackPublish->writePointer >= MAXPUBLISHQUEUELENGTH)
	{
	mqttStackPublish->writePointer = 0;
	}

	return 1;
}


/***************************************************************************************
** \brief		Example function to extract a specific value from a key:value pair in
** 				A JSON string if present
** \param		Pointer to a data string that contains the JSON key:value pair
** \param		Pointer to a string thet holds the key.
** \param		Pointer to int32 variable to store new value on.
** \return		1 if key and value were found 0 otherwise
**
****************************************************************************************/
uint8_t MqttApplication_ExtractJsonString(char* data, char* key, int32_t* value)
{
uint8_t pointer = 0;
uint8_t valuePointer = 0;
char tempValue[15] = {0};

	while(pointer < strlen(data))
	{
		/* Possible start of Key */
		if(data[pointer++] == '\"')
		{
			uint8_t charPointer = 0;
			while(data[pointer] != '\"')
			{
				/* Check if key that is read is different than wished */
				if(data[pointer] != key[charPointer])
				{
				/* If character is not the same as key, continue to check the data */
				/* charPointer = 0 meands the folowing if (lengthcheck will exit the loop */
				charPointer = 0;
				break;
				}
				else
				{
				charPointer++;
				}
			pointer++;
			}

			/* If key match check if key lengths are equal */
			if(charPointer != strlen(key))
			{
			continue;
			}
			/* If all characters from the key match, check if next character is quote */
			if(data[pointer++] == '\"')
			{
			/* At this point we have a key match */
			/* Check if there are white spaces */
			while(data[pointer] == ' '){pointer++;}
			/* Check for the colon */
			while(data[pointer] == ':'){pointer++;}
			/* Check again if there are white spaces */
			while(data[pointer] == ' '){pointer++;}
			/* At this point the value will start*/
			while(data[pointer] != ' ' && data[pointer] != ',' && data[pointer] != '}')
			{
				tempValue[valuePointer++]=data[pointer++];
			}
			*value = MqttInterface_ExtractValueFromString(strlen(tempValue), tempValue);
			return 1;
			}
		}
	}
return 0;
}


/***************************************************************************************
** \brief	This function needs to be called when the MQTT stack is initialized for
** \		the first time or if the stack needs to be reinitialized after connection
** \		loss.
** \param	none
** \return	None
**
****************************************************************************************/
void MqttApplication_Initialize(void)
{
	/* Set all stack parameters to initialization state */
	MqttStack_Initialize();

	/* Set all buffer parameters to initialization state */
	MqttBuffer_Initialize();
}

/***************************************************************************************
** \brief	Function that is called when the stack stops the MQTT connection
** \param	The MQTT error that causes the stop of the connection
** \return	None
**
****************************************************************************************/
void MqttApplication_ConnectionFail(_mqttError mqttError)
{
	switch(mqttError)
	{
		case MQTT_CONNECT_TIMEOUT:
		break;
		case MQTT_PING_TIMEOUT:
		break;
		case MQTT_SUBSCRIBE_TIMEOUT:
		break;
	}
}
