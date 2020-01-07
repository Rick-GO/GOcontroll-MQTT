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
#include "MqttBuffer.h"

typedef struct{
uint16_t messageId;
uint16_t mqttConnectionsMade;
}_mqttStack;

_mqttStack mqttStack;


typedef struct{
uint8_t acknowledgePending;
uint32_t timer;
}_mqttStackPing;

_mqttStackPing mqttStackPing;


typedef struct{
uint16_t messageId[MAXACKNOWLEDGEMENTPENDING];
uint8_t messageLocation[MAXACKNOWLEDGEMENTPENDING];
uint8_t messageLengthRemaining[MAXACKNOWLEDGEMENTPENDING];
uint16_t messageTimeout[MAXACKNOWLEDGEMENTPENDING];
uint8_t messageFailed[MAXACKNOWLEDGEMENTPENDING];
uint8_t messagePointer;
}_mqttStackPublishAcknowledge;

_mqttStackPublishAcknowledge mqttStackPublishAcknowledge;


_mqttStackConnection mqttStackConnection;
_mqttStackSubscribe mqttStackSubscribe;
_mqttStackPublish mqttStackPublish;


/* MQTT stack functions */
static void MqttStack_ConnectToBroker(void);
static void MqttStack_PingToBroker(void);
static void MqttStack_SubscribeToTopic(void);
static void MqttStack_PublishToTopic(void);
static void MqttStack_MessageIdGen(void);
static void MqttStack_PublishAcknowledgedCheck(void);

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
			MqttApplication_ConnectionFail(MQTT_CONNECT_TIMEOUT);
		}
		return;
	}

	if(mqttStackConnection.infoComplete == MQTTFALSE || mqttStackConnection.acknowledgePending == MQTTTRUE || mqttStackConnection.active == MQTTTRUE)
	{
	return;
	}


	/* Make connection with the server. This function checks for DHCP address so it can take a while */
	if (MqttInterface_ConnectToServer(mqttStackConnection.mqttBroker.address,mqttStackConnection.mqttBroker.port) == 0){return;}

	/* Be sure a client ID of 0 does not occur */
	MqttStack_MessageIdGen();

	uint8_t 	clientIdLength 		= strlen(mqttStackConnection.mqttBroker.clientId);
	uint16_t 	protocolLength 		= strlen(PROTOCOL);
	uint8_t		messageStart		= MqttBuffer_GetWritePointerSendBuffer();

	 MqttBuffer_AddByteToSendBuffer(		CONNECT);

	 MqttBuffer_AddByteToSendBuffer(		clientIdLength + (uint8_t)protocolLength + 8);
	 MqttBuffer_AddByteToSendBuffer( 		(uint8_t)protocolLength<<8);
	 MqttBuffer_AddByteToSendBuffer(		(uint8_t)protocolLength);

	MqttBuffer_AddStringToSendBuffer(PROTOCOL, protocolLength);

	 MqttBuffer_AddByteToSendBuffer(		PROTOCOLVERSION);
	 MqttBuffer_AddByteToSendBuffer( 		mqttStackConnection.mqttBroker.connectionFlags);
	 MqttBuffer_AddByteToSendBuffer( 		(uint8_t)mqttStackConnection.mqttBroker.keepAlive>>8);
	 MqttBuffer_AddByteToSendBuffer( 		(uint8_t)mqttStackConnection.mqttBroker.keepAlive);
	 MqttBuffer_AddByteToSendBuffer(  		0);
	 MqttBuffer_AddByteToSendBuffer( 		clientIdLength);

	MqttBuffer_AddStringToSendBuffer(mqttStackConnection.mqttBroker.clientId, clientIdLength);

	MqttBuffer_LoadSendQueue(messageStart, 10 + clientIdLength + protocolLength , MQTTHIGHPRIORITY);

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
	if((mqttStackPing.timer++) < 100 * mqttStackConnection.mqttBroker.keepAlive){return;}

	/* Set pingtimer PINGACHNOWLEDGETIMEOUT ms back to schedule an extra ping */
	mqttStackPing.timer = mqttStackPing.timer - (PINGACHNOWLEDGETIMEOUT/10);
	mqttStackPing.acknowledgePending ++;

	/* In case PINGRETRIES pings are not acknowledged by broker, restart connection */
	if(mqttStackPing.acknowledgePending >=PINGRETRIES)
	{
		MqttApplication_ConnectionFail(MQTT_PING_TIMEOUT);
	return;
	}

	uint16_t	messageStart  = MqttBuffer_GetWritePointerSendBuffer();

	 MqttBuffer_AddByteToSendBuffer( PINGREQ);
	 MqttBuffer_AddByteToSendBuffer( 0);

	MqttBuffer_LoadSendQueue(messageStart, 2, MQTTHIGHPRIORITY);

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
	if(mqttStackSubscribe.topic >= mqttStackSubscribe.topicCounter){return;}

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
		MqttApplication_ConnectionFail(MQTT_SUBSCRIBE_TIMEOUT);
		return;
		}

		dupFlag = DUP;
	}


	uint8_t	messageStart  		= MqttBuffer_GetWritePointerSendBuffer();
	uint8_t 	topicLength 	= strlen(mqttStackSubscribe.mqttSubscribe[mqttStackSubscribe.topic].topic);

	 MqttBuffer_AddByteToSendBuffer( SUBSCRIBE | QOS1 | dupFlag);
	 MqttBuffer_AddByteToSendBuffer( 5+topicLength);
	 MqttBuffer_AddByteToSendBuffer( (uint8_t) mqttStack.messageId>>8);
	 MqttBuffer_AddByteToSendBuffer( (uint8_t) mqttStack.messageId);

	 MqttBuffer_AddByteToSendBuffer( topicLength>>8);
	 MqttBuffer_AddByteToSendBuffer( topicLength);

	MqttBuffer_AddStringToSendBuffer(mqttStackSubscribe.mqttSubscribe[mqttStackSubscribe.topic].topic, topicLength);

	 MqttBuffer_AddByteToSendBuffer( mqttStackSubscribe.mqttSubscribe[mqttStackSubscribe.topic].qos);

	MqttBuffer_LoadSendQueue(messageStart, 7 +topicLength, MQTTHIGHPRIORITY);

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
	/* Check if there are subscribtions that need to be acknowledged */
	if(mqttStackSubscribe.topic < mqttStackSubscribe.topicCounter)
	{
	mqttStackSubscribe.topic ++;
	}
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
	qos = (uint8_t)(MqttBuffer_ReadByteFromReceiveBuffer(startOfContent) & 0x06)>>1;

	/* Retrieve the remaining length from the message */
	uint8_t remainingLength;
	remainingLength = (uint8_t)MqttBuffer_ReadByteFromReceiveBuffer(startOfContent+1);

	/* Retrieve the topic of the received publish message */
	uint16_t topicLength;
	topicLength = (uint8_t)MqttBuffer_ReadByteFromReceiveBuffer(startOfContent+2)<<8;
	topicLength = (uint8_t)MqttBuffer_ReadByteFromReceiveBuffer(startOfContent+3);

	/* Retrieve the message identifier from the received message */
	uint8_t messageIdlength = 0;
	if(qos > 0)
	{
	messageIdlength = 2;
	}

	char topic[MAXTOPICCHARACTERS];

	MqttBuffer_ExtractStringfromReceiveBuffer(topic,startOfContent+4,topicLength);

	/* Check which location owns this topic */
	uint8_t pointer;
	for(pointer = 0; pointer < MAXSUBSCIPTIONS; pointer ++)
	{
		/* Check if there is a topic on the specified location */
		if(mqttStackSubscribe.mqttSubscribe[pointer].topic[0] == 0)
		{
		continue;
		}

		/* If stored topic meets the received topic */
		if(strcmp( topic, mqttStackSubscribe.mqttSubscribe[pointer].topic) == 0)
		{
			mqttStackSubscribe.mqttSubscribe[pointer].newDataReceived =  MQTTTRUE;
			MqttBuffer_ExtractStringfromReceiveBuffer(mqttStackSubscribe.mqttSubscribe[pointer].data,startOfContent+4+topicLength+messageIdlength,remainingLength-topicLength-messageIdlength);
			break;
		}
	}

	if(qos == 1)
	{
		uint8_t		messageStart  	= MqttBuffer_GetWritePointerSendBuffer();

		mqttStackSubscribe.mqttSubscribe[pointer].messageId  = (uint8_t)MqttBuffer_ReadByteFromReceiveBuffer(startOfContent+4+topicLength)<<8;
		mqttStackSubscribe.mqttSubscribe[pointer].messageId += (uint8_t)MqttBuffer_ReadByteFromReceiveBuffer(startOfContent+5+topicLength);

		 MqttBuffer_AddByteToSendBuffer(  PUBACK);
		 MqttBuffer_AddByteToSendBuffer(  2);
		 MqttBuffer_AddByteToSendBuffer(  (uint8_t)mqttStackSubscribe.mqttSubscribe[pointer].messageId>>8);
		 MqttBuffer_AddByteToSendBuffer(  (uint8_t)mqttStackSubscribe.mqttSubscribe[pointer].messageId);

		MqttBuffer_LoadSendQueue(messageStart, 4 , MQTTHIGHPRIORITY);
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
	/* Limit the maximal publish frequency */
	static uint16_t publishIntervalCounter = 0;
	if(publishIntervalCounter > 0){publishIntervalCounter--;return;}
	/* Exit publish routine if there are no publish messages pending */
	if(mqttStackPublish.readPointer == mqttStackPublish.writePointer){return;}

	publishIntervalCounter = MAXPUBLISHINTERVAL/10;


	uint8_t		messageStart  	= MqttBuffer_GetWritePointerSendBuffer();
	uint8_t 	topicLength 	= strlen(mqttStackPublish.mqttPublish[mqttStackPublish.readPointer].topic);
	uint8_t 	dataLength 		= strlen(mqttStackPublish.mqttPublish[mqttStackPublish.readPointer].data);
	uint8_t		messageIdLength = 0;
	uint8_t		sendPriority	= MQTTLOWPRIORITY;

	if(mqttStackPublish.mqttPublish[mqttStackPublish.readPointer].qos > 0)
	{
	messageIdLength = 2;
	sendPriority = MQTTHIGHPRIORITY;
	}

	 MqttBuffer_AddByteToSendBuffer(  PUBLISH |  mqttStackPublish.mqttPublish[mqttStackPublish.readPointer].qos<<1 | mqttStackPublish.mqttPublish[mqttStackPublish.readPointer].retain);

	 MqttBuffer_AddByteToSendBuffer(  2+topicLength+dataLength+messageIdLength);
	 MqttBuffer_AddByteToSendBuffer( topicLength>>8);
	 MqttBuffer_AddByteToSendBuffer( topicLength);

	MqttBuffer_AddStringToSendBuffer(mqttStackPublish.mqttPublish[mqttStackPublish.readPointer].topic, topicLength);

	if(mqttStackPublish.mqttPublish[mqttStackPublish.readPointer].qos > 0)
	{
		 MqttBuffer_AddByteToSendBuffer( (uint8_t) mqttStack.messageId>>8);
		 MqttBuffer_AddByteToSendBuffer( (uint8_t) mqttStack.messageId);

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

	MqttBuffer_AddStringToSendBuffer(mqttStackPublish.mqttPublish[mqttStackPublish.readPointer].data, dataLength);

	MqttBuffer_LoadSendQueue(messageStart, 4+topicLength+dataLength+messageIdLength, sendPriority);

	MqttStack_MessageIdGen();

	/* Increase the read pointer because the message is send */
	if(++mqttStackPublish.readPointer >= MAXPUBLISHQUEUELENGTH)
	{
	mqttStackPublish.readPointer = 0;
	}
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
				if((MqttBuffer_ReadByteFromSendBuffer(mqttStackPublishAcknowledge.messageLocation[pointer]) && PUBLISH) != 1 ||
				   (MqttBuffer_ReadByteFromSendBuffer(mqttStackPublishAcknowledge.messageLocation[pointer]+1) != mqttStackPublishAcknowledge.messageLengthRemaining[pointer]))
				{
					/* The data is possibly overwritten by new data */
					mqttStackPublishAcknowledge.messageId[pointer] = 0;
					continue;
				}

				/* Redeliver a QOS 1 message so add duplicate flag */
				MqttBuffer_AddByteToSendBuffer( mqttStackPublishAcknowledge.messageLocation[pointer] |= DUP);

				MqttBuffer_LoadSendQueue(mqttStackPublishAcknowledge.messageLocation[pointer],mqttStackPublishAcknowledge.messageLengthRemaining[pointer], MQTTHIGHPRIORITY);
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
	messageId  = (uint8_t)MqttBuffer_ReadByteFromReceiveBuffer(startOfContent +2)<<8;
	messageId |= (uint8_t)MqttBuffer_ReadByteFromReceiveBuffer(startOfContent +3);

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
	mqttStackPublish.messageIdAcknowledged  = (uint8_t)MqttBuffer_ReadByteFromReceiveBuffer(startOfContent +2)<<8;
	mqttStackPublish.messageIdAcknowledged |= (uint8_t)MqttBuffer_ReadByteFromReceiveBuffer(startOfContent +3);
	mqttStackPublish.messagePending = 0;

	uint16_t	messageStart  	= MqttBuffer_GetWritePointerSendBuffer();;

	 MqttBuffer_AddByteToSendBuffer(  PUBREL|  QOS1);
	 MqttBuffer_AddByteToSendBuffer(  2);

	 MqttBuffer_AddByteToSendBuffer( MqttBuffer_ReadByteFromReceiveBuffer(startOfContent +2));
	 MqttBuffer_AddByteToSendBuffer( MqttBuffer_ReadByteFromReceiveBuffer(startOfContent +3));

	MqttBuffer_LoadSendQueue(messageStart, 4, MQTTHIGHPRIORITY);
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
	mqttStackPublish.messageIdAcknowledged  = (uint8_t)MqttBuffer_ReadByteFromReceiveBuffer(startOfContent +2)<<8;
	mqttStackPublish.messageIdAcknowledged |= (uint8_t)MqttBuffer_ReadByteFromReceiveBuffer(startOfContent +3);
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
void MqttStack_Initialize(void)
{
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

	mqttStack.mqttConnectionsMade++;
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

	MqttBuffer_SendQueue();
	MqttBuffer_ExtractReceiveBuffer();
}


/***************************************************************************************
** \brief		Utility function to provide acces to the connection information
** \param		None
** \return		pointer to the memory location of the connection information structure
**
****************************************************************************************/
_mqttStackConnection* MqttStack_ConnectionInformation(void)
{
return (_mqttStackConnection*) &mqttStackConnection;
}


/***************************************************************************************
** \brief		Utility function to provide acces to the subscribe information
** \param		None
** \return		Pointer to the memory location of the subscribe informationstructure
**
****************************************************************************************/
_mqttStackSubscribe* MqttStack_SubscribeInformation(void)
{
return (_mqttStackSubscribe*) &mqttStackSubscribe;
}


/***************************************************************************************
** \brief		Utility function to provide acces to the publish information
** \param		None
** \return		Pointer to memory location of hte publish information structure
**
****************************************************************************************/
_mqttStackPublish* MqttStack_PublishInformation(void)
{
return (_mqttStackPublish*) &mqttStackPublish;
}


