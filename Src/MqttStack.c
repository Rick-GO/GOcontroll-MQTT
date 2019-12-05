/****************************************************************************************
* \file         MqttStack.c
* \brief        Core MQTT client functions
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
#include "MqttInterface.h"
#include "MqttApplication.h"
#include "MqttStack.h"

/* MQTT stack functions */
static void MqttStack_ConnectToBroker(void);
static void MqttStack_PingToBroker(void);
static void MqttStack_SubscribeToTopic(void);
static void MqttStack_PublishToTopic(void);
static void MqttStack_MessageIdGen(void);
static void MqttStack_PublishAcknowledgedCheck(void);
static void MqttStack_ReinitializeConnection(void);


/***************************************************************************************
** \brief		Function to construct the message to connect to the Broker. This function
** 				Is called by MqttStackScheduler
** \param		None
** \return		None
**
****************************************************************************************/
static void MqttStack_ConnectToBroker(void)
{
	/* Exit connection routine if not all conditions are met */
	if( mqttStackConnection.acknowledgePending == MQTTTRUE)
	{
		/* Check if the connection Acknowlegement times out */
		mqttStackConnection.acknowledgePendingCounter ++;
		if(mqttStackConnection.acknowledgePendingCounter > CONNECTIONACKNOWLEDGEMENTTIMEOUT/10)
		{
			mqttStack.reinitializeConnection = MQTTTRUE ;
		}
		return;
	}

	if(mqttStackConnection.infoComplete == MQTTFALSE || mqttStackConnection.acknowledgePending == MQTTTRUE || mqttStackConnection.active == MQTTTRUE)
	{
	return;
	}


	/* Make connection with the server. This function checks for DHCP address so it can take a while */
	if (MqttInterface_ConnectToServer(mqttBroker.address,mqttBroker.port) == 0){return;}

	/* Be sure a client ID of 0 does not occur */
	MqttStack_MessageIdGen();

	uint8_t 	clientIdLength 		= strlen(mqttBroker.clientId);
	uint16_t 	protocolLength 		= strlen(PROTOCOL);
	uint8_t	messageStart		= sendBuffer.writePointer;

	sendBuffer.data[sendBuffer.writePointer++] =		CONNECT;

	sendBuffer.data[sendBuffer.writePointer++] =		clientIdLength + (uint8_t)protocolLength + 8;
	sendBuffer.data[sendBuffer.writePointer++] = 		(uint8_t)protocolLength<<8;
	sendBuffer.data[sendBuffer.writePointer++] =		(uint8_t)protocolLength;

	MqttInterface_AddStringToStringToSendBuffer(	PROTOCOL, protocolLength);

	sendBuffer.data[sendBuffer.writePointer++] =		PROTOCOLVERSION;
	sendBuffer.data[sendBuffer.writePointer++] = 		mqttBroker.connectionFlags;
	sendBuffer.data[sendBuffer.writePointer++] = 		(uint8_t)mqttBroker.keepAlive>>8;
	sendBuffer.data[sendBuffer.writePointer++] = 		(uint8_t)mqttBroker.keepAlive;
	sendBuffer.data[sendBuffer.writePointer++] =  		0;
	sendBuffer.data[sendBuffer.writePointer++] = 		clientIdLength;

	MqttInterface_AddStringToStringToSendBuffer(	mqttBroker.clientId, clientIdLength);

	MqttInterface_LoadSendQueue(messageStart, 10 + clientIdLength + protocolLength , MQTTHIGHPRIORITY);

	mqttStackConnection.acknowledgePending = MQTTTRUE;
}


/***************************************************************************************
** \brief		Function that is called by the interface when the connection is acknowledged
** 				by the Broker
** \param		None
** \return		None
**
****************************************************************************************/
void MqttStack_ConnectionAcknowledge(void)
{
mqttStackConnection.active = MQTTTRUE;
mqttStackConnection.acknowledgePending = MQTTFALSE;
}


/***************************************************************************************
** \brief		Function that constructs the message to ping the broker. Also timeout
** 				and retries are handeled in this function. This function is called by
** 				MqttStackScheduler
** \param		None
** \return		None
**
****************************************************************************************/
static void MqttStack_PingToBroker(void)
{
	/* Exit ping routine if there is no connection active */
	if(mqttStackConnection.active == MQTTFALSE){return;}

	/* Only send ping if keep alive time exeeds */
	if((mqttStackPing.timer++) < 100 * mqttBroker.keepAlive){return;}

	/* Set pingtimer PINGACHNOWLEDGETIMEOUT ms back to schedule an extra ping */
	mqttStackPing.timer = mqttStackPing.timer - (PINGACHNOWLEDGETIMEOUT/10);
	mqttStackPing.acknowledgePending ++;

	/* In case PINGRETRIES pings are not acknowledged by broker, restart connection */
	if(mqttStackPing.acknowledgePending >=PINGRETRIES)
	{
		mqttStackPing.acknowledgePending = 0;
		mqttStack.reinitializeConnection =  MQTTTRUE;
	return;
	}

	uint16_t	messageStart  = sendBuffer.writePointer;

	sendBuffer.data[sendBuffer.writePointer++] = PINGREQ;
	sendBuffer.data[sendBuffer.writePointer++] = 0;

	MqttInterface_LoadSendQueue(messageStart, 2, MQTTHIGHPRIORITY);

}


/***************************************************************************************
** \brief		Function that is called by the interface when the Broker responds to a
** 				ping
** \param		None
** \return		None
**
****************************************************************************************/
void MqttStack_PingResponseFromBroker(void)
{
	/* Exit ping routine if there is no connection active */
	if(mqttStackConnection.active == MQTTFALSE){return;}

	mqttStackPing.timer = 0;
	mqttStackPing.acknowledgePending = 0;
}


/***************************************************************************************
** \brief		Function to construct the message to subscribe on a topic. This function
** 				is called by MqttStackScheduler
** \param		None
** \return		None
**
****************************************************************************************/
static void MqttStack_SubscribeToTopic(void)
{
	static uint8_t subscribeFail = 0;
	uint8_t dupFlag = 0;
	/* Exit subscribe routine if there is no connection active */
	if(mqttStackConnection.active == MQTTFALSE){return;}
	/* Exit subscribe routine if there are no topics to subscribe to */
	if(mqttStackSubscribe.topicCounter == 0){return;}
	/* Exit subscribe routine if subscribed to al topics */
	if(mqttStackSubscribe.topicCounter == mqttStackSubscribe.topic){return;}

	if(mqttStackSubscribe.acknowledgePending == MQTTTRUE)
	{
		if(mqttStackSubscribe.acknowledgePendingTimer++ <= (SUBSCRIBTIONACHNOWLEDGETIMEOUT/10))
		{
		return;
		}

		mqttStackSubscribe.acknowledgePendingTimer = 0;

		subscribeFail ++;

		if(subscribeFail >= SUBSCRIBERETRIES)
		{
		subscribeFail = 0;
		mqttStack.reinitializeConnection =  MQTTTRUE;
		return;
		}

		dupFlag = DUP;
	}


	uint8_t	messageStart  	= sendBuffer.writePointer;
	uint8_t 	topicLength 	= strlen(mqttSubscribe[mqttStackSubscribe.topic].topic);

	sendBuffer.data[sendBuffer.writePointer++] = SUBSCRIBE | QOS1 | dupFlag;
	sendBuffer.data[sendBuffer.writePointer++] = 5+topicLength;
	sendBuffer.data[sendBuffer.writePointer++] = (uint8_t) mqttStack.messageId>>8;
	sendBuffer.data[sendBuffer.writePointer++] = (uint8_t) mqttStack.messageId;

	sendBuffer.data[sendBuffer.writePointer++] = topicLength>>8;
	sendBuffer.data[sendBuffer.writePointer++] = topicLength;

	MqttInterface_AddStringToStringToSendBuffer(mqttSubscribe[mqttStackSubscribe.topic].topic, topicLength);

	sendBuffer.data[sendBuffer.writePointer++] = mqttSubscribe[mqttStackSubscribe.topic].qos;

	MqttInterface_LoadSendQueue(messageStart, 7 +topicLength, MQTTHIGHPRIORITY);

	MqttStack_MessageIdGen();

	mqttStackSubscribe.acknowledgePending = MQTTTRUE;

}


/***************************************************************************************
** \brief		Function that is called by the interface when the Broker acknowledges a
** 				subscription to a topic
** \return
**
****************************************************************************************/
void MqttStack_SubscribeAcknowledge(void)
{
	mqttStackSubscribe.acknowledgePending = MQTTFALSE;
	mqttStackSubscribe.topic ++;
	mqttStackSubscribe.acknowledgePendingTimer = 0;
}


/***************************************************************************************
** \brief		Function that is called by the interface when an incoming message
** 				(subscribed to) is received
** \param		start of the contend in the receive buffer (index)
** \return		None
**
****************************************************************************************/
void MqttStack_ReceivePublishedMessage(uint8_t startOfContent)
{
	/* Retrieve the QOS from the received message */
	uint8_t qos;
	qos = (uint8_t)(receiveBuffer.data[startOfContent] & 0x06)>>1;

	/* Retrieve the remaining length from the message */
	uint8_t remainingLength;
	remainingLength = (uint8_t)receiveBuffer.data[startOfContent+1];

	/* Retrieve the topic of the received publish message */
	uint16_t topicLength;
	topicLength = (uint8_t)receiveBuffer.data[startOfContent+2]<<8;
	topicLength = (uint8_t)receiveBuffer.data[startOfContent+3];

	/* Retrieve the message identifier from the received message */
	uint8_t messageIdlength = 0;
	if(qos > 0)
	{
	messageIdlength = 2;
	}

	char topic[MAXTOPICCHARACTERS];

	MqttInterface_ExtractStringfromReceiveBuffer(topic,startOfContent+4,topicLength);

	/* Check which location owns this topic */
	uint8_t pointer;
	for(pointer = 0; pointer < MAXSUBSCIPTIONS; pointer ++)
	{
		/* Check if there is a topic on the specified location */
		if(mqttSubscribe[pointer].topic[0] == 0)
		{
		continue;
		}

		/* If stored topic meets the received topic */
		if(strcmp( topic, mqttSubscribe[pointer].topic) == 0)
		{
			mqttSubscribe[pointer].newDataReceived =  MQTTTRUE;
			MqttInterface_ExtractStringfromReceiveBuffer(mqttSubscribe[pointer].data,startOfContent+4+topicLength+messageIdlength,remainingLength-topicLength-messageIdlength);
			break;
		}
	}

	if(qos == 1)
	{
		uint8_t		messageStart  	= sendBuffer.writePointer;

		mqttSubscribe[pointer].messageId  = (uint16_t)receiveBuffer.data[startOfContent+4+topicLength]<<8;
		mqttSubscribe[pointer].messageId |= (uint16_t)receiveBuffer.data[startOfContent+5+topicLength];

		sendBuffer.data[sendBuffer.writePointer++] =  PUBACK;
		sendBuffer.data[sendBuffer.writePointer++] =  2;
		sendBuffer.data[sendBuffer.writePointer++] =  (uint8_t)mqttSubscribe[pointer].messageId>>8;
		sendBuffer.data[sendBuffer.writePointer++] =  (uint8_t)mqttSubscribe[pointer].messageId;

		MqttInterface_LoadSendQueue(messageStart, 4 , MQTTHIGHPRIORITY);
	}
}



/***************************************************************************************
** \brief		Function that constructs the publish message. This function is called by
** 				MqttStackScheduler
** \param		None
** \return		none
**
****************************************************************************************/
static void MqttStack_PublishToTopic(void)
{
	/* Exit publish routine if there is no connection active */
	if(mqttStackConnection.active == MQTTFALSE){return;}
	/* Exit publish routine if there are subscribtion active */
	if(mqttStackSubscribe.topicCounter != mqttStackSubscribe.topic){return;}
	/* Exit publish routine if there are no publish messages pending */
	if(mqttStackPublish.messagePending == MQTTFALSE){return;}

	/* Limit the maximal publish frequency */
	uint8_t		messageStart  	= sendBuffer.writePointer;
	uint8_t 	topicLength 	= strlen(mqttPublish.topic);
	uint8_t 	dataLength 		= strlen(mqttPublish.data);
	uint8_t		messageIdLength = 0;
	uint8_t		sendPriority	= MQTTLOWPRIORITY;

	if(mqttPublish.qos > 0)
	{
	messageIdLength = 2;
	sendPriority = MQTTHIGHPRIORITY;
	}

	sendBuffer.data[sendBuffer.writePointer++] =  PUBLISH |  mqttPublish.qos<<1 | mqttPublish.retain;

	sendBuffer.data[sendBuffer.writePointer++] =  2+topicLength+dataLength+messageIdLength;
	sendBuffer.data[sendBuffer.writePointer++] = topicLength>>8;
	sendBuffer.data[sendBuffer.writePointer++] = topicLength;

	MqttInterface_AddStringToStringToSendBuffer(	mqttPublish.topic, topicLength);

	if(mqttPublish.qos > 0)
	{
		sendBuffer.data[sendBuffer.writePointer++] = (uint8_t) mqttStack.messageId>>8;
		sendBuffer.data[sendBuffer.writePointer++] = (uint8_t) mqttStack.messageId;

		mqttStackPublish.messageIdAcknowledgePending = mqttStack.messageId;

		for(uint8_t pointer = 0; pointer < 10; pointer ++)
		{
			if(mqttStackPublishAcknowledge.messageId[pointer] == 0)
			{
				mqttStackPublishAcknowledge.messageId[pointer] = mqttStack.messageId;
				mqttStackPublishAcknowledge.messageLocation[pointer] = messageStart;
				mqttStackPublishAcknowledge.messageLengthRemaining[pointer] = 2+topicLength+dataLength+messageIdLength;
				mqttStackPublishAcknowledge.messageTimeout[pointer] = 0;
				mqttStackPublishAcknowledge.messageFailed[pointer] = 0;
				break;
			}
		}
	}

	MqttInterface_AddStringToStringToSendBuffer(mqttPublish.data, dataLength);

	MqttInterface_LoadSendQueue(messageStart, 4+topicLength+dataLength+messageIdLength, sendPriority);

	MqttStack_MessageIdGen();

	mqttStackPublish.messagePending = 0;
}


/***************************************************************************************
** \brief		Function that checks if different messages are acknowledged based on
** 				their message ID. If not, These messages are send again with the duplicate
** 				flag set. This function is called by MqttStackScheduler
** \param		None
** \return		None
**
****************************************************************************************/
static void MqttStack_PublishAcknowledgedCheck(void)
{
	for(uint8_t pointer = 0; pointer < MAXACKNOWLEDGEMENTPENDING; pointer ++)
	{
		if(mqttStackPublishAcknowledge.messageId[pointer] != 0)
		{
			mqttStackPublishAcknowledge.messageTimeout[pointer] ++;

			if(mqttStackPublishAcknowledge.messageTimeout[pointer] >= (PUBLISHACKNOWLEDGETIMEOUT/10))
			{
				/* Check how many times a message is resend */
				if(mqttStackPublishAcknowledge.messageFailed[pointer] >= PUBLISHRETRIES)
				{
					/* Cancel redelivery mechanism */
					mqttStackPublishAcknowledge.messageId[pointer]=0;
					break;
				}
				/* Check if message is not already overwritten in buffer */
				if((sendBuffer.data[mqttStackPublishAcknowledge.messageLocation[pointer]] && PUBLISH) != 1 ||
				   (sendBuffer.data[mqttStackPublishAcknowledge.messageLocation[pointer]+1] != mqttStackPublishAcknowledge.messageLengthRemaining[pointer]))
				{
					/* The data is possibly overwritten by new data */
					mqttStackPublishAcknowledge.messageId[pointer] = 0;
					continue;
				}

				/* Redeliver a QOS 1 message so add duplicate flag */
				sendBuffer.data[mqttStackPublishAcknowledge.messageLocation[pointer]] |= DUP;

				MqttInterface_LoadSendQueue(mqttStackPublishAcknowledge.messageLocation[pointer],mqttStackPublishAcknowledge.messageLengthRemaining[pointer], MQTTHIGHPRIORITY);
				mqttStackPublishAcknowledge.messageTimeout[pointer] = 0;
				/* Acknowledgement by Broker failed in between time period. */
				mqttStackPublishAcknowledge.messageFailed[pointer] ++;
			}
		}
	}
}



/***************************************************************************************
** \brief		Function that is called by the interface when a published message with
** 				QOS > 0 is received. The message is released after acknowledgement
** \param		Start of the contend in the receive buffer (index)
** \return		None
**
****************************************************************************************/
void MqttStack_PublishAcknowledgedByServer(uint8_t startOfContent)
{

	uint16_t messageId;
	messageId  = (uint8_t)receiveBuffer.data[startOfContent +2]<<8;
	messageId |= (uint8_t)receiveBuffer.data[startOfContent +3];

	for(uint8_t pointer = 0; pointer < MAXACKNOWLEDGEMENTPENDING; pointer ++)
	{
		if(mqttStackPublishAcknowledge.messageId[pointer] == messageId)
		{
			mqttStackPublishAcknowledge.messageId[pointer] = 0;
			mqttStackPublishAcknowledge.messageLocation[pointer] = 0;
			mqttStackPublishAcknowledge.messageLengthRemaining[pointer] = 0;
			break;
		}
	}
}


/***************************************************************************************
** \brief		Function that is called by the interface when a published message with
** 				QOS == 2 is received by the server.  This mechanism is not tested yet.
** \param		Start of the contend in the receive buffer (index)
** \return		None
**
****************************************************************************************/
void MqttStack_PublishReceivedByServer(uint8_t startOfContent)
{
	mqttStackPublish.messageIdAcknowledged  = (uint8_t)receiveBuffer.data[startOfContent +2]<<8;
	mqttStackPublish.messageIdAcknowledged |= (uint8_t)receiveBuffer.data[startOfContent +3];
	mqttStackPublish.messagePending = 0;

	uint16_t	messageStart  	= sendBuffer.writePointer;

	sendBuffer.data[sendBuffer.writePointer++] =  PUBREL|  QOS1;
	sendBuffer.data[sendBuffer.writePointer++] =  2;

	sendBuffer.data[sendBuffer.writePointer++] = receiveBuffer.data[startOfContent +2];
	sendBuffer.data[sendBuffer.writePointer++] = receiveBuffer.data[startOfContent +3];

	MqttInterface_LoadSendQueue(messageStart, 4, MQTTHIGHPRIORITY);
}


/***************************************************************************************
** \brief		Function that is called by the interface when a published message with
** 				QOS == 2 is acknowledged. This mechanism is not tested yet.
** \param		Start of the contend in the receive buffer (index)
** \return		None
**
****************************************************************************************/
void MqttStack_PublishCompletedByServer(uint8_t startOfContent)
{
	mqttStackPublish.messageIdAcknowledged  = (uint8_t)receiveBuffer.data[startOfContent +2]<<8;
	mqttStackPublish.messageIdAcknowledged |= (uint8_t)receiveBuffer.data[startOfContent +3];
	mqttStackPublish.messagePending = 0;
}


/***************************************************************************************
** \brief		Function that generates a message ID and prohibit a message ID from 0
** \param		None
** \return		None
**
****************************************************************************************/
static void MqttStack_MessageIdGen(void)
{
	mqttStack.messageId++;

	if(mqttStack.messageId == 0)
	{
	mqttStack.messageId++;
	}
}


/***************************************************************************************
** \brief		MQTT function that becomes active if the reinitialization flag is set.
** 				This function is called by MqttStackScheduler
** \param		None
** \return		None
**
****************************************************************************************/
static void MqttStack_ReinitializeConnection(void)
{
	if(mqttStack.reinitializeConnection ==  MQTTFALSE){return;}

	mqttStackConnection.active = 0;
	mqttStackConnection.acknowledgePendingCounter = 0;

	mqttStackPing.acknowledgePending = 0;
	mqttStackPing.timer = 0;

	mqttStackSubscribe.acknowledgePending = 0;
	mqttStackSubscribe.acknowledgePendingTimer = 0;
	mqttStackSubscribe.topic = 0;

	mqttStackPublish.messageIdAcknowledgePending = 0;
	mqttStackPublish.messageIdAcknowledged = 0;
	mqttStackPublish.messagePending = 0;

	for(uint8_t pointer = 0 ; pointer <MAXACKNOWLEDGEMENTPENDING; pointer ++)
	{
	mqttStackPublishAcknowledge.messageId[pointer] = 0;
	}

	queue.readPointer = 0;
	queue.writePointer = 0;

	receiveBuffer.readPointer = 0;
	receiveBuffer.writePointer = 0;

	sendBuffer.readPointer = 0;
	sendBuffer.writePointer = 0;

	mqttStack.mqttConnectionsLost++;

	mqttStack.reinitializeConnection =  MQTTFALSE;
}


/***************************************************************************************
** \brief		MQTT stack engine function. This function needs to be called each 10ms
** \param		None
** \return		None
**
****************************************************************************************/
void MqttStack_Scheduler(void)
{
	MqttStack_ConnectToBroker();
	MqttStack_PingToBroker();
	MqttStack_SubscribeToTopic();
	MqttStack_PublishToTopic();
	MqttStack_PublishAcknowledgedCheck();

	MqttInterface_SendQueue();
	MqttInterface_ExtractReceiveBuffer();

	MqttStack_ReinitializeConnection();
}
