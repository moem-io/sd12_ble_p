#ifndef __SENSOR_COMMUNICATION_H__
#define __SENSOR_COMMUNICATION_H__

#include <stdbool.h>
#include "app_uart.h"
#include "app_error.h"
#include "nrf_delay.h"
#include "nrf.h"
#include "bsp.h"

#define UART_TX_BUF_SIZE 256                         /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE 256                         /**< UART RX buffer size. */

#define BUFFER_SIZE  20
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
	Request = '1',
	Command,
	Reponse,
	Ack,
	Set_Address,
}Packet_Mode;

typedef enum{
 Th = 'T',
 Pressure = 'P',
 Light = 'L',
 Button = 'B',
 Human = 'H',
 Sound = 'S',
 Rgb = 'R',
 Ir = 'I',
 Buzzer = 'Z',
}Sensor_Type;

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
	uint16_t bufferPacket[BUFFER_SIZE];
}flagUART;

typedef struct {
	uint8_t length;
	char mode;
	char sensor;
	uint8_t id;
	char data[MAX_DATA];
}Packet;
static Packet transmitPacket;
static Packet receivePacket;


uint8_t data[UART_RX_BUF_SIZE];

__WEAK void UART_Receive_CallBack(void);

ret_code_t Sensor_Communication_Init(void);

ret_code_t Send_Packet_Polling(void);

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
bool checkPacket(Packet* packet, char mode, char sensor, uint8_t state);
static void initializePacket(Packet* packet);

#endif
