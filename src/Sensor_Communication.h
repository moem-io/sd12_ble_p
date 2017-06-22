#ifndef __SENSOR_COMMUNICATION_H__
#define __SENSOR_COMMUNICATION_H__

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "Sensor_Config.h"
#include "app_uart.h"
#include "app_error.h"
#include "nrf_delay.h"
#include "nrf.h"
#include "bsp.h"

#define UART_TX_BUF_SIZE 256                         /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE 256                         /**< UART RX buffer size. */

#define BUFFER_SIZE  20
#define BUFFER_RX_TX_SIZE 128
#define MAX_DATA 10


#define START '['
#define END ']'

typedef enum{
	Start,
	Length,
	Mode,
	Sensor,
	Id,
	Data
}Packet_Structure;

typedef enum{
	Packet_Ok = 0,
	Packet_NULL,
	Packet_Length_ERROR,
	Packet_Mode_ERROR,
	Packet_Sensor_ERROR,
	Packet_Id_ERROR,
	Packet_Data_ERROR,
	Packet_Dropout,
	Packet_Already_Exist,
}Packet_Status;

typedef struct {
	bool flag;
	uint8_t bufferPacket[BUFFER_SIZE];
}flagUART;

//typedef struct{
//	uint8_t queueBuffer[BUFFER_QUEUE_SIZE];
//	
//	uint8_t start;
//	uint8_t end;
//}Queue;

typedef struct {
	uint8_t length;
	char mode;
	char sensor;
	uint8_t id;
	char data[MAX_DATA];
}Packet;
static Packet transmitPacket;
static Packet receivePacket;

static char packet_transmit_buffer[BUFFER_RX_TX_SIZE];
static char packet_receive_buffer[BUFFER_RX_TX_SIZE];

//static Queue queue;

__WEAK void UART_Receive_CallBack(char* messages);

ret_code_t Sensor_Communication_Init(void);

uint32_t Send_Packet_Polling(char mode, uint8_t sensor, uint8_t id, char* data);

Packet_Status makePacket(char mode, uint8_t sensor, uint8_t id, char* data);
Packet_Status releasePacket(char* message);
Packet_Status packetTostring(void);

Packet* getReceivePacket(void);
Packet* getTransmitPacket(void);

uint8_t getPacket_Length(Packet *);
char getPacket_Mode(Packet *);
char getPacket_Sensor(Packet *);
char getPacket_Id(Packet *);
char* getPacket_Data(Packet *);

void printPacket(Packet* packet);
bool checkPacket(Packet* packet, char mode, char sensor, uint8_t id, uint8_t state);
static void initializePacket(Packet* packet);



//bool 	checkItem(uint8_t pin);
//bool 	push(uint8_t pin);
//void 		removeItem(uint8_t pin);
//uint8_t pop(void);
//void		initializeQueue(void);
#endif
