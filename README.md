**GOcontroll-MQTT**

GOcontroll-MQTT is a lightweight, client oriëntated MQTT stack to construct 
messages to broker and read messages from the broker.

***Features:***

- Ping mechanism to keep connection alive
- QOS 1 and 2
- Retain flag for messages
- Reinitialize connection if lost

- Configurable timeout and retries for:	
- connection acknowledge
- subscribe acknowledge
- publish acknowledge (QOS == 1)
- ping response


**Stack build-up**

There are 3 source files which their corresponding header files which needs 
to be added to a project.


***MqttApplication.c***

This file contains the functions that can be used in the user application. 
These functions provide the user with the ability to set Broker/server details,
subscribe to topics and publish to topics. Since JSON strings are used frequently,
this file also contains a function to extract an int value that belongs to a give 
key, from the received data. 


***MqttInterface.c***

This file contains the functions that handle specific communication with the 
communication interface. This is the place to add your own communication 
interface between the conditional macro's 


***MqttStack.c***

This file contains the core functions that handle the actual MQTT functionality. 
Changing this file is only interesting when the user decide to add or improve 
functionality.
The corresponding header file provides some tweak functions to change timeouts, 
retries and some other parameters. This can be useful if another communication 
interface is used which has to deal with more latency.


**Stack implementation**

The stack in the repositoty is build on top of the LwIP netconn API. If another
communication interface is required, the user need to implement this in the 
file: *MqttInterface.c* Add your own interface to the following functions:

- uint8_t MqttInterface_ConnectToServer(char* address, uint16_t port)
- void MqttInterface_ReceiveFromServer(void)
- uint8_t MqttInterface_SendToServer(uint8_t dataLocation, uint8_t length)

In *MqttInterface.h* add your own interfae macro


***Stack engine***

*MqttStackScheduler()*

To power up the stack, the user needs to call this function each 10 ms. It is 
important to provide a steady timing otherwise timeouts and ping actions are 
not executed properly.

*MqttInterface_ReceiveFromServer()*

To receive data over the LwIP netconn TCP connection, this function needs to be 
called in a loop. Be aware, this function is blocking so best result wil be to 
implement this function in a thread or task. In case another interface is used, 
calling this function in a loop may not be manditory.    


*TODO*
- Add QOS 2
- Add secure MQTT login



 MQTT stack - C language - Embedded devices
