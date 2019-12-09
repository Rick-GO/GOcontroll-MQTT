/****************************************************************************************
* \file         MqttInterface.c
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

/* General includes for data types and string functions */
#include "stdio.h"
#include "string.h"

/* LwIP includes */
#include "lwip/dhcp.h"
#include "lwip.h"
#include "lwip/dns.h"
#include "api.h"

/* MQTT stack includes */
#include "MqttStack.h"
#include "MqttInterface.h"

/* construct the dataholder for the connection */
struct netconn *conn;

/***************************************************************************************
** \brief		Interface function to connect to a TCP server
** \param		Pointer to a string that holds the address of the broker
** \param		The port number of the broker to connect to
** \return		1 if connected successful otherwise 0;
**
****************************************************************************************/
uint8_t MqttInterface_ConnectToServer(char* address, uint16_t port)
{
#if MQTTNETCONNINTERFACE == 1
	ip_addr_t addr;

	/* portSimulation is used to bind to another port during reconnection
	 * This mechanism speeds up the reconnection (bind) process */
	static uint8_t portSimulation = 0;

	/* Check if DHCP has retrieved an IP addres */
	if(dhcp_supplied_address(&gnetif)!= 1){return 0;}

	/* Initialize DNS to use a DNS addres instead of an IP addres */
	dns_init();

	/* If there was already a connection, close it */
	if(conn != NULL)
	{
	netconn_close(conn);
	netconn_delete(conn);
	}

	/* If failed to retrieve DNS address exit function */
	if(netconn_gethostbyname(address, &addr) != 0 ){return 0;}

	conn 						= netconn_new ( NETCONN_TCP );
	/* If failed to bind to TCP server exit function */
	if(netconn_bind ( conn, IP_ADDR_ANY, portSimulation++ )!= 0 ){return 0;}
	/* If failed to connect to TCP server exit function */
	if( netconn_connect ( conn, &addr, port )!= 0 ){return 0;}

	return 1;

#else
/* Space for other interface to use MQTT on */
#endif
}


/***************************************************************************************
** \brief		Function that checks for new incoming data over the TCP connection. This
** 				function needs to be called from a dedicated thread/task that is allowed
** 				to be blocked. Don't use other routines in this thread/task since data can
** 				be missed on the TCP connection.
** \param		None
** \return		1 if proper TCP connection 0 otherwise
**
****************************************************************************************/
uint8_t MqttInterface_ReceiveFromServer(void)
{
#if MQTTNETCONNINTERFACE == 1
	struct netbuf *buf;

	if(netconn_recv(conn, &buf) == ERR_OK)
	{
		netbuf_copy(buf, &receiveBuffer.data[receiveBuffer.writePointer], buf->p->tot_len);
		receiveBuffer.writePointer += buf->p->tot_len;
		netbuf_delete(buf);
		return 1;
	}
	else
	{
		/* Implement a task delay otherwise this task/thread will consume 100 cpu
		 * if disconnected by the remote */
		return 0;
	}
#else
	/* Space for other interface to use MQTT on */
#endif
}


/***************************************************************************************
** \brief		Function that constructs the data to be send over the connection. It
** 				constructs the data array and when ready is send the amount of bytes away=
** \param		Data location (index) from the sendbuffer that is filled by the stack
** \param		The length of the data that needs to be send
** \return		The length of the message that was send
**
****************************************************************************************/
uint8_t MqttInterface_SendToServer(uint8_t dataLocation, uint8_t length)
{
	static uint8_t sendBuf[256] ={0};
	sendBuffer.readPointer = dataLocation;

	for(uint8_t bufferPointer = 0; bufferPointer < length; bufferPointer ++)
	{
		sendBuf[bufferPointer] = sendBuffer.data[sendBuffer.readPointer++];
	}
#if MQTTNETCONNINTERFACE == 1
netconn_write(conn, &sendBuf[0], length, NETCONN_NOFLAG);
#else
/* Space for other interface to use MQTT on */
#endif
return length;
}


/***************************************************************************************
** \brief		This routine check if there are new messages added to the send queue. A
** 				message in the queue contains the start location in the sendbuffer, the
** 				length of a message and the priority of the message.
** \param		None
** \return		None
**
****************************************************************************************/
void MqttInterface_SendQueue(void)
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
	MqttInterface_SendToServer(queue.dataLocation[queue.readPointer], queue.dataLength[queue.readPointer] );

	if(++queue.readPointer>= QUELENGTH)
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
void MqttInterface_LoadSendQueue(uint8_t dataPointer, uint8_t length, uint8_t priority)
{
	/* Exit function if there are items in the que and the message has a low priority */
	if(queue.writePointer != queue.readPointer && priority == MQTTLOWPRIORITY){return;}

	queue.dataLocation[queue.writePointer] 	= dataPointer;
	queue.dataLength[queue.writePointer] 	= length;
	queue.dataPriority[queue.writePointer]	= priority;

	if(++queue.writePointer>= QUELENGTH)
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
void MqttInterface_ExtractReceiveBuffer(void)
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
** \brief		Function to add a string to the sendbuffer. Since we use a circular
** 				sendBuffer, strcpy can overwrite the buffer This function always writes
** 				inside the circular buffer
** \param		Pointer to the string that needs to be written in the sendbuffer
** \param		The length of the string that needs to be written in the sendBuffer
** \return		The number of written bytes to the buffer.
**
****************************************************************************************/
uint8_t MqttInterface_AddStringToStringToSendBuffer(char* string, uint8_t length)
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
uint8_t MqttInterface_ExtractStringfromReceiveBuffer(char* string, uint8_t startLocation, uint8_t length)
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
** \brief		Function that extract an int32 value from a string
** \param		The number of characters that represent a number
** \param		Pointer to a string that only contains the numbers that needs to be converted
** \return		The converted int32 value
**
****************************************************************************************/
int32_t MqttInterface_ExtractValueFromString(uint8_t numberOfCharacters, char *dataReceived)
{

	int32_t dataCalculated = 0;
	int32_t stringToInt = 0;
	uint8_t negative = 0;;

	for(unsigned char c = 1; c<=numberOfCharacters; c++)
	{
		switch(dataReceived[c-1])
		{
			case '-' : {negative = 1;break;}
			case '0' : {stringToInt = 0;break;}
			case '1' :{stringToInt = 1;break;}
			case '2' : {stringToInt = 2;break;}
			case '3' :{stringToInt = 3;break;}
			case '4' :{stringToInt = 4;break;}
			case '5' :{stringToInt = 5;break;}
			case '6' :{stringToInt = 6;break;}
			case '7' :{stringToInt = 7;break;}
			case '8' :{stringToInt = 8;break;}
			case '9' :{stringToInt = 9;break;}
		}

		switch(numberOfCharacters-c)
		{
			case 0 : {dataCalculated += stringToInt;break;}
			case 1 :{dataCalculated += (stringToInt*10);break;}
			case 2 : {dataCalculated += (stringToInt*100);break;}
			case 3 :{dataCalculated += (stringToInt*1000);break;}
			case 4 :{dataCalculated += (stringToInt*10000);break;}
			case 5 :{dataCalculated += (stringToInt*100000);break;}
			case 6 :{dataCalculated += (stringToInt*1000000);break;}
			case 7 :{dataCalculated += (stringToInt*10000000);break;}
			case 8 :{dataCalculated += (stringToInt*100000000);break;}
			case 9 :{dataCalculated += (stringToInt*1000000000);break;}
		}
	}
	if (negative ==1)
	{
		return -1 * dataCalculated;
	}
return dataCalculated;
}
