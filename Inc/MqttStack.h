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
* Author: 					Rick Gijsberts 				  <rickgijsberts@gocontroll.com>
****************************************************************************************/

#ifndef GOCONTROLL_INC_MQTTSTACK_H_
#define GOCONTROLL_INC_MQTTSTACK_H_


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
/* Define maximum number of messages that wait for acknowledgement by broker (QOS 1) */
#define MAXACKNOWLEDGEMENTPENDING						5


/* Define the timeout for a connection acknowledgement by the broker in ms */
#define CONNECTIONACKNOWLEDGEMENTTIMEOUT				10000
/* Define the timeout for subscribtion acknowledgement by the broker in ms */
#define SUBSCRIBTIONACHNOWLEDGETIMEOUT					5000
/* Define the number of subscribtion retries*/
#define SUBSCRIBERETRIES								2
/* Define the timeout for a publish acknowledgement by the broker (qos1) */
#define PUBLISHACKNOWLEDGETIMEOUT						5000
/* Define the number of publish retries if not acknowledged within timeout (qos1) */
#define PUBLISHRETRIES									2
/* Define the timeout for a ping acknowledgement by the broker */
#define PINGACHNOWLEDGETIMEOUT							5000
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

/* Protocol defines */
#define CONNECT					0x10
#define CONNACK					0x20
#define PUBLISH					0x30
#define PUBACK					0x40
#define PUBREC					0x50
#define PUBREL					0x60
#define PUBCOMP					0x70
#define SUBSCRIBE				0x80
#define SUBACK					0x90
#define UNSUBSCRIBE				0xA0
#define UNSUBACK				0xB0
#define PINGREQ					0xC0
#define PINGRESP				0xD0
#define DISCONNECT				0xE0

/* Message configurations */
#define DUP						0x04
#define QOS0					0x00
#define QOS1					0x02
#define QOS2					0x04
#define QOS3					0x06
#define RETAIN					0x01

#define MQTTTRUE				1
#define MQTTFALSE				0

#define SETMESSAGEID			1
#define ACKNOWLEDGEMESSAGEID	2

#define MQTTHIGHPRIORITY		1
#define MQTTNORMALPRIORITY		2
#define MQTTLOWPRIORITY			3

typedef struct{
char		address[MAXADDRESSCHARACTERS];
uint16_t 	port;
char 		clientId[MAXCLIENTIDCHARACTERS];
uint16_t 	keepAlive;
uint8_t		connectionFlags;
}_mqttBroker;


typedef struct{
uint16_t messageId;
uint8_t reinitializeConnection;
uint16_t mqttConnectionsLost;
}_mqttStack;

typedef struct{
uint8_t infoComplete;
uint8_t acknowledgePending;
uint16_t acknowledgePendingCounter;
uint8_t active;
}_mqttStackConnection;

typedef struct{
uint8_t acknowledgePending;
uint32_t timer;
}_mqttStackPing;

typedef struct{
uint8_t topicCounter;
uint8_t topic;
uint8_t acknowledgePending;
uint16_t acknowledgePendingTimer;
}_mqttStackSubscribe;

typedef struct{
uint8_t messagePending;
uint16_t messageIdAcknowledgePending;
uint16_t messageIdAcknowledged;
}_mqttStackPublish;

typedef struct{
uint16_t messageId[MAXACKNOWLEDGEMENTPENDING];
uint8_t messageLocation[MAXACKNOWLEDGEMENTPENDING];
uint8_t messageLengthRemaining[MAXACKNOWLEDGEMENTPENDING];
uint16_t messageTimeout[MAXACKNOWLEDGEMENTPENDING];
uint8_t messageFailed[MAXACKNOWLEDGEMENTPENDING];
uint8_t messagePointer;
}_mqttStackPublishAcknowledge;

typedef struct{
char topic[MAXTOPICCHARACTERS];
uint8_t qos;
uint16_t messageId;
uint8_t newDataReceived;
char* data;
}_mqttSubscribe;

typedef struct{
char topic[MAXTOPICCHARACTERS];
uint8_t qos;
uint8_t retain;
char* data;
}_mqttPublish;


/* Broker information */
_mqttBroker mqttBroker;

/* Stack information */
_mqttStack mqttStack;
_mqttStackConnection mqttStackConnection;
_mqttStackPing mqttStackPing;
_mqttStackSubscribe mqttStackSubscribe;
_mqttStackPublish mqttStackPublish;
_mqttStackPublishAcknowledge mqttStackPublishAcknowledge;

/* subscribe and publish information */
_mqttSubscribe mqttSubscribe[MAXSUBSCIPTIONS];
_mqttPublish mqttPublish;

/* Functions taht needs to be omplemented by user (called each 10m*/
void MqttStack_Scheduler(void);

/* Functions needed by MqttInterface */
void MqttStack_ConnectionAcknowledge(void);
void MqttStack_PingResponseFromBroker(void);
void MqttStack_SubscribeAcknowledge(void);
void MqttStack_ReceivePublishedMessage(uint8_t startOfContent);
void MqttStack_PublishAcknowledgedByServer(uint8_t startOfContent);
void MqttStack_PublishReceivedByServer(uint8_t startOfContent);
void MqttStack_PublishCompletedByServer(uint8_t startOfContent);


#endif /* GOCONTROLL_INC_MQTTSTACK_H_ */
