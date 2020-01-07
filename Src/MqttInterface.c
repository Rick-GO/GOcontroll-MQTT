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

/* MQTT stack includes */
#include "MqttInterface.h"
#include "MqttBuffer.h"
#include "MqttStack.h"


/* NETCONN related includes */
#if MQTTNETCONNINTERFACE == 1
/* LwIP includes */
#include "lwip/dhcp.h"
#include "lwip.h"
#include "lwip/dns.h"
#include "api.h"
#endif

/* GOcontroll AT stack related includes */
#if MQTTGOCONTROLLATINTERFACE == 1
#include "AtCommandInterface.h"
#include "AtCommandLibrary.h"
#endif


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
	if((dhcp_supplied_address(&gnetif))!= 1){return 0;}

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
/* If this function is called it means that the connection with the TCP server is already established
 * since the AT stack takes care of a TCP connection. So return 1 to let the MqttStack know that there
 * is a connection
 */
	return 1;
#endif
}

#if MQTTNETCONNINTERFACE == 1
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

	struct netbuf *buf;

	if(conn == NULL){return 0;}

	if(netconn_recv(conn, &buf) == ERR_OK)
	{
		uint8_t dataPointer = receiveBuffer.writePointer;

		for(uint8_t bufferPointer = 0; bufferPointer < buf->p->tot_len; bufferPointer++)
		{
			netbuf_copy_partial(buf,&receiveBuffer.data[dataPointer++],1,0+bufferPointer);
		}

		receiveBuffer.writePointer = dataPointer;
		netbuf_delete(buf);

		return 1;
	}
	else
	{
		/* Implement a task delay otherwise this task/thread will consume 100 cpu
		 * if disconnected by the remote */
		return 0;
	}
}
#endif

#if MQTTGOCONTROLLATINTERFACE == 1
/***************************************************************************************
** \brief		Function that checks for new incoming data over the TCP connection. This
** 				function needs to be called from a dedicated thread/task that is allowed
** 				to be blocked. Don't use other routines in this thread/task since data can
** 				be missed on the TCP connection.
** \param		None
** \return		1 if proper TCP connection 0 otherwise
**
****************************************************************************************/
void MqttInterface_ReceiveFromServer(uint8_t* bufferLocation,uint8_t bufferPointer, uint8_t length)
{
	//uint8_t dataPointer = MqttBuffer_GetWritePointerReceiveBuffer();

	for(uint8_t packetCopy = 0; packetCopy < length; packetCopy++)
	{
		 MqttBuffer_AddByteToReceiveBuffer(bufferLocation[bufferPointer++]);
	//	receiveBuffer.data[dataPointer++] = bufferLocation[bufferPointer++];
		//bufferLocation[*bufferPointer++] =
	}
	//receiveBuffer.writePointer = dataPointer;
}
#endif

/***************************************************************************************
** \brief		Function that constructs the data to be send over the connection. It
** 				constructs the data array and when ready is send the amount of bytes away
** \param		Pointer to the 0 index location of the send buffer
** \param		Data location (index) from the sendbuffer that is filled by the stack
** \param		The length of the data that needs to be send
** \return		1 If message was send succesfully
**
****************************************************************************************/
uint8_t MqttInterface_SendToServer(uint8_t* data, uint8_t dataPointer, uint8_t length)
{
#if MQTTNETCONNINTERFACE == 1
	static uint8_t sendBuf[256] ={0};
	sendBuffer.readPointer = dataLocation;

	for(uint8_t bufferPointer = 0; bufferPointer < length; bufferPointer ++)
	{
		sendBuf[bufferPointer] = sendBuffer.data[sendBuffer.readPointer++];
	}



	if(netconn_write(conn, &sendBuf[0], length, NETCONN_NOFLAG)==ERR_OK)
	{
		/* Let the sendQueue know that the message was send properly */
		MqttBuffer_SendQueueSendSucces();
	}

return 0;

#endif
#if MQTTGOCONTROLLATINTERFACE == 1
/* This AT interface function sends a piece of data in the dataBuffer tot the TCP layer */
AtCommandInterface_SendTcpPacket (data,dataPointer, length, 1);
return 0;
#endif
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



