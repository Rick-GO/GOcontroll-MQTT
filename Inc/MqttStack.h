/****************************************************************************************
* \file         MqttStack.h
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

#ifndef GOCONTROLL_INC_MQTTSTACK_H_
#define GOCONTROLL_INC_MQTTSTACK_H_

#include "stdio.h"


/* MQTT protocol definitions */
#define PROTOCOL				"MQTT"
#define PROTOCOLVERSION			3

/* MQTT stack configuration */
/* Define the number of characters from the broker Address */
#define MAXADDRESSCHARACTERS							100
/* Define the maximal length of a client ID */
#define MAXCLIENTIDCHARACTERS							50
/* Define maximum number of characters for topics */
#define MAXTOPICCHARACTERS								50
/* Static reserve of memory for a  maximum number of topics to subscribe to */
#define MAXSUBSCIPTIONS									5
/* Static reserve of memory for a  maximum number of publish messages to be queued */
#define MAXPUBLISHQUEUELENGTH							5
/* Define maximum number of messages that wait for acknowledgement by broker (QOS 1) */
#define MAXACKNOWLEDGEMENTPENDING						5


/* Define the timeout for a connection acknowledgement by the broker in ms */
#define CONNECTIONACKNOWLEDGEMENTTIMEOUT				10000
/* Define the timeout for subscribtion acknowledgement by the broker in ms */
#define SUBSCRIBTIONACHNOWLEDGETIMEOUT					5000
/* Define the number of subscribtion retries*/
#define SUBSCRIBERETRIES								2
/* Define the maximal publish interval in ms */
#define MAXPUBLISHINTERVAL								300
/* Define the timeout for a publish acknowledgement by the broker (qos1) */
#define PUBLISHACKNOWLEDGETIMEOUT						5000
/* Define the number of publish retries if not acknowledged within timeout (qos1) */
#define PUBLISHRETRIES									2
/* Define the timeout for a ping acknowledgement by the broker */
#define PINGACHNOWLEDGETIMEOUT							10000
/* Define the number of ping retries when not acknowledged within time */
#define PINGRETRIES										3


/* Connection Flags */
#define CONNECTIONUSENAME		0x80
#define CONNECTIONPASSWORD		0x40
#define CONNECTIONWILLRETAIN	0x20
#define CONNECTIONWILLQOS0		0x08
#define CONNECTIONWILLQOS1		0x10
#define CONNECTIONWILLQOS2		0x18
#define CONNECTIONWILL			0x04
#define CONNECTIONCLEANSESION	0x02

#define SETMESSAGEID			1
#define ACKNOWLEDGEMESSAGEID	2

/* Protocol defines */
typedef enum
{
CONNECT				=	0x10,
CONNACK				=	0x20,
PUBLISH				=	0x30,
PUBACK				=	0x40,
PUBREC				=	0x50,
PUBREL				=	0x60,
PUBCOMP				=	0x70,
SUBSCRIBE			=	0x80,
SUBACK				=	0x90,
UNSUBSCRIBE			=	0xA0,
UNSUBACK			=	0xB0,
PINGREQ				=	0xC0,
PINGRESP			=	0xD0,
DISCONNECT			=	0xE0
} _mqttMessage;

/* Message configurations */
typedef enum
{
DUP					=	0x04,
QOS0				=	0x00,
QOS1				=	0x02,
QOS2				=	0x04,
QOS3				=	0x06,
RETAIN				=	0x01
} _mqttMessageProperties;

typedef enum
{
MQTTTRUE			=	0x01,
MQTTFALSE			=	0x00
} _mqttLogic;


typedef enum
{
MQTTHIGHPRIORITY	=	0x01,
MQTTNORMALPRIORITY	=	0x02,
MQTTLOWPRIORITY		=	0x03
} _mqttMessagePriority;






typedef struct{
char		address[MAXADDRESSCHARACTERS];
uint16_t 	port;
char 		clientId[MAXCLIENTIDCHARACTERS];
uint16_t 	keepAlive;
uint8_t		connectionFlags;
}_mqttBroker;


typedef struct{
uint8_t infoComplete;
uint8_t acknowledgePending;
uint16_t acknowledgePendingCounter;
uint8_t active;
_mqttBroker mqttBroker;
}_mqttStackConnection;



typedef struct{
char topic[MAXTOPICCHARACTERS];
uint8_t qos;
uint16_t messageId;
uint8_t newDataReceived;
char* data;
}_mqttSubscribe;

typedef struct{
uint8_t topicCounter;
uint8_t topic;
uint8_t acknowledgePending;
uint16_t acknowledgePendingTimer;
_mqttSubscribe mqttSubscribe[MAXSUBSCIPTIONS];
}_mqttStackSubscribe;


typedef struct{
char topic[MAXTOPICCHARACTERS];
uint8_t qos;
uint8_t retain;
char* data;
}_mqttPublish;

typedef struct{
uint8_t writePointer;
uint8_t readPointer;
uint8_t messagePending;
uint16_t messageIdAcknowledgePending;
uint16_t acknowledgePendingTimer;
uint16_t messageIdAcknowledged;
_mqttPublish mqttPublish[MAXPUBLISHQUEUELENGTH];
}_mqttStackPublish;


/* Functions that needs to be implemented by user (called each 10m*/
void MqttStack_Scheduler(void);

/* Functions needed by MqttInterface */
void MqttStack_ConnectionAcknowledge(void);
void MqttStack_PingResponseFromBroker(void);
void MqttStack_SubscribeAcknowledge(void);
void MqttStack_ReceivePublishedMessage(uint8_t startOfContent);
void MqttStack_PublishAcknowledgedByServer(uint8_t startOfContent);
void MqttStack_PublishReceivedByServer(uint8_t startOfContent);
void MqttStack_PublishCompletedByServer(uint8_t startOfContent);
void MqttStack_Initialize(void);

_mqttStackConnection* MqttStack_ConnectionInformation(void);
_mqttStackSubscribe* MqttStack_SubscribeInformation(void);
_mqttStackPublish* MqttStack_PublishInformation(void);

#endif /* GOCONTROLL_INC_MQTTSTACK_H_ */
