#include "Sensor_Communication.h"
#include "util_misc.h"

char sensor_data[UART_RX_BUF_SIZE];

void uart_event_handle(app_uart_evt_t * p_event){

		uint8_t message = 0;
		static uint16_t index = 0;
	
		static bool flagReceive_Start = false;
    switch (p_event->evt_type)
    {
        case APP_UART_DATA_READY:
						if( app_uart_get(&message) == NRF_SUCCESS){
							
							if(flagReceive_Start){
								if(message == END){					
									sensor_data[index] = '\0';
									index = 0;
									flagReceive_Start = false;
									UART_Receive_CallBack(sensor_data);
								}else{
									sensor_data[index] = message;
									index++;
								}
							}else{
								if(message == START){
									flagReceive_Start = true;
								}
							}
						}
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

__WEAK void UART_Receive_CallBack(char* messages){
	
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

uint32_t Send_Packet_Polling(char mode, uint8_t sensor, uint8_t id, char* data){
	
	uint8_t index = 0;
	uint8_t length = 0;
	
	uint32_t err_code = NRF_SUCCESS;
	
	makePacket(mode, sensor, id, data);
	packetTostring();
	
	printf("%s", packet_transmit_buffer);
	
	return NRF_SUCCESS;
}

Packet_Status makePacket(char mode, uint8_t sensor, uint8_t pin, char* data){
	initializePacket(&transmitPacket);
	
	if(mode == Request || mode == Command || mode == Response || mode == Ack || mode == Set_Address){
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
	
	if(mode == Set_Address){
		transmitPacket.id = pin;
	}else{
		switch(pin){
			case ID1:
				transmitPacket.id = 'A';
			break;
			case ID2:
				transmitPacket.id = 'B';
			break;
			case ID3:
				transmitPacket.id = 'C';
			break;
			case ID4:
				transmitPacket.id = 'D';
			break;
			case ID5:
				transmitPacket.id = 'E';
			break;
		}
	}
	
	
	if(data != NULL){
		if(strlen(data) < MAX_DATA){
			sprintf(transmitPacket.data, "%s", data);
		}else{
			return Packet_Data_ERROR;
		}	
	}else{
		return Packet_Data_ERROR;
	}
	
	if(data != NULL){
		transmitPacket.length = 4 + strlen(transmitPacket.data);
	}else{
		transmitPacket.length = 4 ;
	}

}
Packet_Status releasePacket(char* message){
	uint8_t length = 0;
	
	initializePacket(&receivePacket);
	
	if(strlen(message) < 4){
		return Packet_Dropout;
	}
	
	length = message[Length - 1];
	
	if(length != strlen(message)){
		return Packet_Length_ERROR;
	}
	receivePacket.length = length;
		
	switch(message[Mode - 1]){
		case Request:
		case Command:
		case Ack:
		case Set_Address:
		case Response:
			receivePacket.mode = message[Mode - 1];
		break;
		default:
			return Packet_Mode_ERROR;
	}
	
	if(message[Sensor - 1] == Th || message[Sensor - 1] == Pressure || message[Sensor - 1] == Light || 
     message[Sensor - 1] == Button || message[Sensor - 1] == Human || message[Sensor - 1] == Sound ||
     message[Sensor - 1] == Rgb || message[Sensor - 1] == Ir || message[Sensor - 1] == Buzzer	){
		receivePacket.sensor = message[Sensor - 1];
	}else{
		return Packet_Sensor_ERROR;
	}
	
	if(message[Id - 1] != 0){
		receivePacket.id = message[Id - 1];
	}else{
		return Packet_Id_ERROR;
	}
	
	strcpy(receivePacket.data, &message[Data - 1]);
	
	return Packet_Ok;
}
Packet_Status packetTostring(void){
	uint8_t length = 0;
	
	packet_transmit_buffer[Start] = START;
	
	if(transmitPacket.length != 0){
		packet_transmit_buffer[Length] = transmitPacket.length;
	}else{
		return Packet_Length_ERROR;
	}
	
	if(transmitPacket.mode != 0){
		packet_transmit_buffer[Mode] = transmitPacket.mode;
	}else{
		return Packet_Mode_ERROR;
	}
	
	if(transmitPacket.sensor != 0){
		packet_transmit_buffer[Sensor] = transmitPacket.sensor;
	}else{
		return Packet_Sensor_ERROR;
	}
	
	if(transmitPacket.id != 0){
		packet_transmit_buffer[Id] = transmitPacket.id;
	}else{
		return Packet_Id_ERROR;
	}
	
	length = strlen((const char*)transmitPacket.data);
	
	if( length != 0){
		strcpy(&packet_transmit_buffer[Data], (const char*)transmitPacket.data);
	}else{
		if(transmitPacket.mode != Set_Address){
			return Packet_Data_ERROR;
		}
	}
	
	packet_transmit_buffer[length + Id + 1] = END;
	packet_transmit_buffer[length + Id + 2] = '\0';
	
	return Packet_Ok;
}

Packet* getReceivePacket(void){
	return &receivePacket;
}
Packet* getTransmitPacket(void){
	return &transmitPacket;
}

uint8_t getPacket_Length(Packet *packet){
	return packet->length;
}
char getPacket_Mode(Packet *packet){
	return packet->mode;
}
char getPacket_Sensor(Packet *packet){
	return packet->sensor;
}
char getPacket_Id(Packet *packet){
	return packet->id;
}
char* getPacket_Data(Packet *packet){
	return packet->data;
}

void printPacket(Packet* packet){
	printf("Packet.length : %d\r\n", packet->length);
	
	switch(packet->mode){
		case Request: 				printf("Packet.mode : Request\r\n"); break;
		case Command: 			printf("Packet.mode : Command\r\n"); break;
		case Response: 			printf("Packet.mode : Reponse\r\n"); break;
		case Ack: 								printf("Packet.mode : Ack\r\n"); break;
		case Set_Address: 	printf("Packet.mode : Set_Address\r\n"); break;
		default:											printf("Packet.mode : None\r\n");
	}
	
	switch(packet->sensor){
		case Th: 							printf("Packet.sensor : Th\r\n"); break;
		case Pressure: 	printf("Packet.sensor : Pressure\r\n"); break;
		case Light: 					printf("Packet.sensor : Light\r\n"); break;
		case Button: 				printf("Packet.sensor : Button\r\n"); break;
		case Human: 			printf("Packet.sensor : Human\r\n"); break;
		case Sound: 				printf("Packet.sensor : Sound\r\n"); break;
		case Rgb: 						printf("Packet.sensor : Rgb\r\n"); break;
		case Ir: 								printf("Packet.sensor : Ir\r\n"); break;
		case Buzzer: 			printf("Packet.sensor : Buzzer\r\n"); break;
		default:									printf("Packet.sensor : None\r\n");
	}
	
	printf("Packet.id : %d\r\n", packet->id);
	
	printf("Packet.data : %s\r\n", packet->data);
}
bool checkPacket(Packet* packet, char mode, char sensor, uint8_t id, uint8_t state){
	bool flag = false;
	
	uint8_t length = strlen(packet->data);
	
	if(packet->length == length + Id){
		flag = true;
	}else{
		return false;
	}
	
	if(packet->mode == mode){
		flag = true;
	}else{
		return false;
	}
	
	if(packet->sensor == sensor){
		flag = true;
	}else{
		return false;
	}
	
	if(state == Set_Address){
		if(packet->id == id){
			flag = true;
		}
	}else{
		switch(id){
			case pinID1:
				if(packet->id == 'A'){
					flag = true;
				}
			break;
			case pinID2:
				if(packet->id == 'B'){
					flag = true;
				}
			break;
			case pinID3:
				if(packet->id == 'C'){
					flag = true;
				}
			break;
			case pinID4:
				if(packet->id == 'D'){
					flag = true;
				}
			break;
			case pinID5:
				if(packet->id == 'E'){
					flag = true;
				}
			break;
		}
	}
	
	
	return flag;
}
static void initializePacket(Packet* packet){
	
}



//bool 	checkItem(uint8_t pin){
//	bool flag = false;
//	uint8_t index = 0;
//	
//	for(index = 0; index < BUFFER_QUEUE_SIZE; index++){
//		if(queue.queueBuffer[index] == pin){
//			flag = true;
//			break;
//		}
//	}
//	
//	if(index == BUFFER_QUEUE_SIZE){
//		flag = false;
//	}
//	
//	return flag;
//}
//bool 	push(uint8_t pin){
//	bool flag = false;
//	
//	if( ( queue.end + 1) % BUFFER_QUEUE_SIZE == queue.start){
//		flag = false;
//	}
//	if( !flag ){
//		queue.queueBuffer[queue.end] = pin;
//		
//		queue.end = (queue.end + 1) % BUFFER_QUEUE_SIZE;
//		flag = true;
//	}
//	
//	return flag;
//}
//void 		removeItem(uint8_t pin){
//	
//	uint8_t index = 0;
//	
//	for(index = 0; index < BUFFER_QUEUE_SIZE; index++){
//		if(queue.queueBuffer[index] == pin){
//			break;
//		}
//	}
//	
//	for(; index + 1 != queue.end; index = (index + 1 ) % BUFFER_SIZE){
//		queue.queueBuffer[index] = queue.queueBuffer[index + 1];
//	}
//	queue.end = (index + 1) % BUFFER_QUEUE_SIZE;
//	
//}
//uint8_t 	pop(void){

//	uint8_t pin = 0;
//	
//	if(queue.start == queue.end){
//		return 0;
//	}
//	
//	pin = queue.queueBuffer[queue.start];
//	
//	queue.start = (queue.start + 1) % BUFFER_QUEUE_SIZE;
//	
//	return pin;
//	
//}
//void		initializeQueue(void){
//	uint8_t id = 0;
//	
//	queue.start = 0;
//	queue.end = 0;
//	
//	for(id = 0; id < BUFFER_QUEUE_SIZE; id++){
//		queue.queueBuffer[id] = 0;
//	}
//	
//}

