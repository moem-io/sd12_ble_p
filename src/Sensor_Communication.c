#include "Sensor_Communication.h"

void uart_event_handle(app_uart_evt_t * p_event){

    switch (p_event->evt_type)
    {
        case APP_UART_DATA_READY:
						
            break;
        case APP_UART_COMMUNICATION_ERROR:
            APP_ERROR_HANDLER(p_event->data.error_communication);
            break;

        case APP_UART_FIFO_ERROR:
            APP_ERROR_HANDLER(p_event->data.error_code);
            break;

        default:
            break;
    };
}

__WEAK void UART_Receive_CallBack(void){
	
}


ret_code_t Sensor_Communication_Init(void){
	uint32_t err_code;

	const app_uart_comm_params_t comm_params =
	{
				RX_PIN_NUMBER,
				TX_PIN_NUMBER,
				RTS_PIN_NUMBER,
				CTS_PIN_NUMBER,
				APP_UART_FLOW_CONTROL_DISABLED,
				false,
				UART_DEFAULT_CONFIG_BAUDRATE
	};
	
	APP_UART_FIFO_INIT(&comm_params,
											 UART_RX_BUF_SIZE,
											 UART_TX_BUF_SIZE,
											 uart_event_handle,
											 APP_IRQ_PRIORITY_LOWEST,
											 err_code);

	return err_code;
}

Packet_Status makePacket(char mode, uint8_t sensor, uint8_t id, char* data){
	initializePacket(&transmitPacket);
	
	if(mode == Request || mode == Command || mode == Reponse || mode == Ack || mode == Set_Address){
		transmitPacket.mode = mode;
	}else{
		return Packet_Mode_ERROR;
	}
	
	if(sensor == Th || sensor == Pressure || sensor == Light || sensor == Button || sensor == Human
		|| sensor == Sound || sensor == Rgb || sensor == Ir || sensor == Buzzer){
		transmitPacket.sensor = sensor;
	}else{
		return Packet_Sensor_ERROR;
	}
	
	
}
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



