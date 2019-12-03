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


/***************************************************************************************
** \brief		Function that connects to the Broker.
** \param		Address of the broker on which to connect to
** \param		Port from the broker to connect to
** \param		Pointer to the string that holds the client ID
** \param		Keep alive ping period in seconds
**  \param		Connection flags  (see MqttApplication.h for flags)
** \return		1 if information was ok, 0 if one of parameters was not correct
**
****************************************************************************************/
uint8_t MqttApplication_ConnectToBroker (char* address, uint16_t port, char* clientID, uint16_t keepAlive, uint8_t connectionsFlag)
{
	/* Check if address is not to long */
	if(strlen(address)>= MAXADDRESSCHARACTERS){return 0;}
	strcpy((char*)&mqttBroker.address,address);

	mqttBroker.port = port;

	/* Check if client ID is not to long */
	if(strlen(clientID)>= MAXCLIENTIDCHARACTERS){return 0;}
	strcpy((char*)&mqttBroker.clientId,clientID);

	mqttBroker.keepAlive = keepAlive;

	mqttBroker.connectionFlags = connectionsFlag;

	mqttStackConnection.infoComplete = MQTTTRUE;
	return 1;
}


/***************************************************************************************
** \brief		Function that subscribes to a topic.
** \param		Pointer to a string that contains the topic
** \param		The qos (0,1,2) that is required for the message delivery (NOTE: QOS2 not tested)
** \param		Pointer to data string that holds the data comming from the broker if other
** 				client publish to the subscribed topic.
** \return		1 if information was ok, 0 if one of parameters was not correct
**
****************************************************************************************/
uint8_t MqttApplication_SubscribeToTopic(char* topic, uint8_t qos, char* data)
{
	/* Check if not already the maximum number of subscribtions are reached */
	if(mqttStackSubscribe.topicCounter == MAXSUBSCIPTIONS){return 0;}

	/* Check if topic is not to long */
	if(strlen(topic) >= MAXTOPICCHARACTERS){return 0;}
	strcpy((char*)&mqttSubscribe[mqttStackSubscribe.topicCounter].topic, topic);

	/* Check if QOS has a value between 0 and 2 */
	if(qos > 2){return 0;}
	mqttSubscribe[mqttStackSubscribe.topicCounter].qos = qos;

	/* Check if data pointer is not empty */
	if(data == NULL){return 0;}
	mqttSubscribe[mqttStackSubscribe.topicCounter++].data = data;

	return 1;
}


/***************************************************************************************
** \brief		Function that can be used to publish to a topic.
** \param		Pointer to a string that contains the topic
** \param		The qos (0,1,2) that is required for the message delivery (NOTE: QOS2 not tested)
** \param		Retain flag (0-1) 1 keep on broker, 0 forget message
** \param		Pointer to data string that holds the data to publish
** \return		1 if information was ok, 0 if one of parameters was not correct
**
****************************************************************************************/
uint8_t MqttApplication_PublishToTopic(char* topic, uint8_t qos, uint8_t retain, char* data)
{
	if(strlen(topic) >= MAXTOPICCHARACTERS){return 0;}
	strcpy((char*)&mqttPublish.topic, topic);

	/* Check if data pointer is not empty */
	if(data != NULL){return 0;}

	/* Check if not already a message is pending*/
	if(mqttStackPublish.messagePending == MQTTTRUE){return 0;}

	/* Check if QOS has a value between 0 and 2 */
	if(qos > 2){return 0;}
	mqttPublish.qos = qos;

	/* Check if retain flag is 1 or 0 */
	if(retain > 1){return 0;}
	mqttPublish.retain = retain;

	/* Check if data pointer is not empty */
	if(data == NULL){return 0;}
	mqttPublish.data = data;

	mqttStackPublish.messagePending = MQTTTRUE;

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
			for(uint8_t charPointer = 0; charPointer < strlen(key); charPointer++)
			{
				if(data[pointer++] != key[charPointer])
				{
				/* If character is not the same as key, continue to check the data */
				continue;
				}
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
