/****************************************************************************************
* \file         MqttBuffer.c
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


#include "MqttInterface.h"
#include "MqttStack.h"




/* Define the maximal length of the message queue
 * Explanation:
 * The queue is loaded with information about packets that need to be send over TCP. It uses
 * the cyclic send buffer to store */
#define QUELENGTH 					10


typedef struct{
uint8_t* 				bufferLocation[QUELENGTH];
uint16_t 				dataLocation[QUELENGTH];
uint16_t 				dataLength[QUELENGTH];
_mqttMessagePriority 	dataPriority[QUELENGTH];
uint8_t 				writePointer;
uint8_t 				readPointer;
}_queue;

_queue queue;


/* Keep these buffer sizes 256 since the read and
 * write pointer make use of natural overflow.
 */
typedef struct{
uint8_t data[256];
uint8_t writePointer;
uint8_t readPointer;
}_receiveBuffer;

_receiveBuffer receiveBuffer;



typedef struct{
uint8_t data[256];
uint8_t writePointer;
uint8_t readPointer;
}_sendBuffer;

_sendBuffer sendBuffer;



/***************************************************************************************
** \brief		This routine check if there are new messages added to the send queue. A
** 				message in the queue contains the start location in the sendbuffer, the
** 				length of a message and the priority of the message.
** \param		None
** \return		None
**
****************************************************************************************/
void MqttBuffer_SendQueue(void)
{

	/* Check if there is data in the que to send */
	if(queue.readPointer == queue.writePointer){return;}

	/* Check if there are high priority messages in the queue. If so, send them directly */
	uint8_t tempReadPointer = queue.readPointer;
	while(tempReadPointer != queue.writePointer)
	{
		if(queue.dataPriority[tempReadPointer] == MQTTHIGHPRIORITY)
		{
			queue.readPointer = tempReadPointer;
			break;
		}

		if(tempReadPointer++ >= QUELENGTH)
		{
			tempReadPointer = 0;
		}
	}


	/* Send the data to the server */
	MqttInterface_SendToServer(&sendBuffer.data[0],queue.dataLocation[queue.readPointer], queue.dataLength[queue.readPointer]);
}

/***************************************************************************************
** \brief		Function that is called by the transportation layer to let the queue know
** 				that the message was send properly
** \param		None
** \return		None
**
****************************************************************************************/
void MqttBuffer_SendQueueSendSucces(void)
{
	if(++queue.readPointer >= QUELENGTH)
	{
	queue.readPointer = 0;
	}
}


/***************************************************************************************
** \brief		Function that is called by the stack when new data is added to the
** 				sendBuffer. The stack gives the start location of a specific message in
** 				the sendBuffer, the length of it and the priority.
** \param		The pointer value (index) to the start location
** \param		The length of the message
** \param		The priority of the message
** \return		None
**
****************************************************************************************/
void MqttBuffer_LoadSendQueue(uint8_t bufferPointer, uint8_t length, _mqttMessagePriority priority)
{
	queue.dataLocation[queue.writePointer] 		= bufferPointer;
	queue.dataLength[queue.writePointer] 		= length;
	queue.dataPriority[queue.writePointer]		= priority;

	if(++queue.writePointer >= QUELENGTH)
	{
		queue.writePointer = 0;
	}
}


/***************************************************************************************
** \brief		Function that extracts the messages from incomming data. This function is
**				called by the MqttStackScheduler.
** \param		None
** \return		None
**
****************************************************************************************/
void MqttBuffer_ExtractReceiveBuffer(void)
{
	/* First retrieve some information about the received messages */
	uint8_t startOfContent 		= receiveBuffer.readPointer;
	uint8_t lengthOfContent 	= receiveBuffer.writePointer-receiveBuffer.readPointer;
	receiveBuffer.readPointer	= receiveBuffer.writePointer;
	uint8_t lengthCounter 		= 0;

	/* As long as there is data to be read out */
	while(lengthCounter < lengthOfContent)
	{

		switch(receiveBuffer.data[startOfContent])
		{
		case CONNACK: 		MqttStack_ConnectionAcknowledge(); 							break;
		case PUBACK: 		MqttStack_PublishAcknowledgedByServer(startOfContent);		break;
		case PUBREC: 		MqttStack_PublishReceivedByServer(startOfContent);			break;
		case PUBCOMP: 		MqttStack_PublishCompletedByServer(startOfContent);			break;
		case PINGRESP: 		MqttStack_PingResponseFromBroker(); 						break;
		case PUBLISH|QOS0: 	MqttStack_ReceivePublishedMessage(startOfContent);	 		break;
		case PUBLISH|QOS1:  MqttStack_ReceivePublishedMessage(startOfContent); 			break;
		case PUBLISH|QOS2:  MqttStack_ReceivePublishedMessage(startOfContent);			break;
		case SUBACK:		MqttStack_SubscribeAcknowledge();							break;
		}

		/* Increase the length of total read bytes */
		lengthCounter += 2 + receiveBuffer.data[(uint8_t)startOfContent+1];

		/* Point to the next message location */
		startOfContent += 2 + receiveBuffer.data[(uint8_t)startOfContent+1];
	}
}

/***************************************************************************************
** \brief		Function that adds one byte to the send buffer of the MQTT stack.
** \param		The uint8_t value that needs to be added to the receive buffer
** \return		None
**
****************************************************************************************/
void MqttBuffer_AddByteToSendBuffer(uint8_t byte)
{
sendBuffer.data[sendBuffer.writePointer++] = byte;
}

/***************************************************************************************
** \brief		Function that provides a single byte from the send buffer
** \param		The index pointer from where the byte needs to be read
** \return		The data byte on the specified location in the send buffer
**
****************************************************************************************/
uint8_t MqttBuffer_ReadByteFromSendBuffer(uint8_t bufferPointer)
{
return sendBuffer.data[bufferPointer];
}

/***************************************************************************************
** \brief		Function that provides the write pointer of the circulair send buffer
** \param		None
** \return		The index writepointer of the send buffer
**
****************************************************************************************/
uint8_t MqttBuffer_GetWritePointerSendBuffer(void)
{
return sendBuffer.writePointer;
}


/***************************************************************************************
** \brief		Function that adds one byte to the receive buffer of the MQTT stack. This
** 				function can be used to load new data coming from the transportation layer.
** \param		The uint8_t value that needs to be added to the receive buffer
** \return		None
**
****************************************************************************************/
void MqttBuffer_AddByteToReceiveBuffer(uint8_t byte)
{
receiveBuffer.data[receiveBuffer.writePointer++] = byte;
}

/***************************************************************************************
** \brief		Function that provides a single byte from the receive buffer
** \param		The index pointer from where the byte needs to be read
** \return		The data byte on the specified location in the receive buffer
**
****************************************************************************************/
uint8_t MqttBuffer_ReadByteFromReceiveBuffer(uint8_t bufferPointer)
{
return receiveBuffer.data[bufferPointer];
}

/***************************************************************************************
** \brief		Function that provides the write pointer of the circulair receive buffer
** \param		None
** \return		The index writepointer of the receive buffer
**
****************************************************************************************/
uint8_t MqttBuffer_GetWritePointerReceiveBuffer(void)
{
return receiveBuffer.writePointer;
}


/***************************************************************************************
** \brief		Function to add a string to the sendbuffer. Since we use a circular
** 				sendBuffer, strcpy can overwrite the buffer This function always writes
** 				inside the circular buffer
** \param		Pointer to the string that needs to be written in the sendbuffer
** \param		The length of the string that needs to be written in the sendBuffer
** \return		The number of written bytes to the buffer.
**
****************************************************************************************/
uint8_t MqttBuffer_AddStringToSendBuffer(char* string, uint8_t length)
{
uint8_t writtenBytes = 0;
	for(uint8_t stringPointer = 0; stringPointer<length; stringPointer ++)
	{
		writtenBytes++;
		sendBuffer.data[sendBuffer.writePointer++] = (uint8_t) string[stringPointer];
	}
return writtenBytes;
}


/***************************************************************************************
** \brief		Function to retrieve a string from the circular buffer.
** \param		Pointer to a char data holder where to store the string on
** \param		Start location from the string in the receiveBuffer
** \param		The length of the string that need to be read from the receiveBuffer
** \return		The number of bytes that were read
**
****************************************************************************************/
uint8_t MqttBuffer_ExtractStringfromReceiveBuffer(char* string, uint8_t startLocation, uint8_t length)
{
uint8_t readBytes = 0;
	for(uint8_t stringPointer = 0; stringPointer<length; stringPointer ++)
	{
		readBytes++;
		string[stringPointer] = receiveBuffer.data[startLocation++];
	}
return readBytes;
}

/***************************************************************************************
** \brief		Function that is called when the MQTT connection is lost and probably
** 				Will be initialized again
** \param		none
** \return		none
**
****************************************************************************************/
void MqttBuffer_Initialize(void)
{
	queue.readPointer = 0;
	queue.writePointer = 0;


	receiveBuffer.readPointer = 0;
	receiveBuffer.writePointer = 0;

	sendBuffer.readPointer = 0;
	sendBuffer.writePointer = 0;
}
