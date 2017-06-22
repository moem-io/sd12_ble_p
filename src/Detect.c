#include "Detect.h"

static app_gpiote_user_id_t user_id;

void sensor_dectect_handler(const uint32_t* event_pins_low_to_high,const uint32_t* event_pins_high_to_low){
	Detect_CallBack(*event_pins_low_to_high, *event_pins_high_to_low);
}

__WEAK void Detect_CallBack(uint32_t rising, uint32_t falling){
	
}

uint32_t Detect_Init(void){
	
	uint32_t err_code;
	uint8_t id= 0;
	
	uint32_t LOW_TO_HIGH_BITMASK = PIN_ID1_BITMASK | PIN_ID2_BITMASK | PIN_ID3_BITMASK | PIN_ID4_BITMASK | PIN_ID5_BITMASK  | PIN_ID_CHG_BITMASK | PIN_ID_GPOUT_BITMASK;
	uint32_t HIGH_TO_LOW_BITMASK = PIN_ID1_BITMASK | PIN_ID2_BITMASK | PIN_ID3_BITMASK | PIN_ID4_BITMASK | PIN_ID5_BITMASK  | PIN_ID_CHG_BITMASK | PIN_ID_GPOUT_BITMASK;

//	uint32_t LOW_TO_HIGH_BITMASK = PIN_ID1_BITMASK | PIN_ID2_BITMASK | PIN_ID3_BITMASK | PIN_ID4_BITMASK | PIN_ID5_BITMASK ;
//	uint32_t HIGH_TO_LOW_BITMASK = PIN_ID1_BITMASK | PIN_ID2_BITMASK | PIN_ID3_BITMASK | PIN_ID4_BITMASK | PIN_ID5_BITMASK ;
//	
	
	APP_GPIOTE_INIT(MAX_CHANNEL);
	
	err_code = app_gpiote_user_register(&user_id, &LOW_TO_HIGH_BITMASK, &HIGH_TO_LOW_BITMASK, sensor_dectect_handler);
	if(err_code != NRF_SUCCESS){
		return err_code;
	}
	
	app_gpiote_user_enable(user_id);
	
	for(id = 0; id <= ID_CHG; id++){
		if(checkChannel(id) == Falling){
			setState_Channel(id, Falling); 
		}else{
			setState_Channel(id, Rising); 
		}
	}
	
	for(id = ID1; id <= ID5; id++){
		setSensor_Channel(id, 0);
	}
	
	return  err_code;
}

uint8_t getState_Channel(uint8_t id){
	if(id >= ID1 && id <= MAX_CHANNEL){
		return channel[id];
	}
	return 2;
}

void setState_Channel(uint8_t id, uint8_t state){
	if(id >= ID1 && id < MAX_CHANNEL){
		channel[id] = state;
	}
}

uint8_t getSensor_Channel(uint8_t id){
	return channel_Type[id];
}
void		setSensor_Channel(uint8_t id, uint8_t type){
	channel_Type[id] = type;
}

bool checkChannel(uint8_t pin){
	
	uint32_t pinState;
	
	nrf_delay_ms(100);
	
	switch(pinState){
		case ID1: 							return nrf_gpio_pin_read(pinID1) > 0;
		case ID2: 							return nrf_gpio_pin_read(pinID2) > 0;
		case ID3: 							return nrf_gpio_pin_read(pinID3) > 0;
		case ID4: 							return nrf_gpio_pin_read(pinID4) > 0;
		case ID5: 							return nrf_gpio_pin_read(pinID5) > 0;
		case ID_GPOUT: 		return nrf_gpio_pin_read(pinGPOUT) > 0;
		case ID_CHG: 				return nrf_gpio_pin_read(pinCHG) > 0;
		default: 									return false;
	}
}

void checkEdge(__IO flagDetect* flag){
		uint32_t mask[MAX_CHANNEL] = { PIN_ID1_BITMASK,
																											PIN_ID2_BITMASK,
																											PIN_ID3_BITMASK,
																											PIN_ID4_BITMASK,
																											PIN_ID5_BITMASK,
																											PIN_ID_GPOUT_BITMASK,
																											PIN_ID_CHG_BITMASK};																										
	uint8_t id = 0;																						
	
	for(id = ID1; id <= ID_CHG; id++){

		if(flag->rising & mask[id]){
			flag->pin = id;
			flag->state = Rising;
			break;
		}else if(flag->falling & mask[id]){
			flag->pin = id;
			flag->state = Falling;
			break;
		}
	}
	nrf_delay_ms(10);
}

void InitalizeQueue(void){
	int id =0 ;
	
	for(id = ID1; id <= ID5; id++){
		queue.buffer[id] = ID_None;
	}
	
	queue.start = 0;
	queue.end = 0;
}

bool push(uint8_t id){
	
	if( (queue.end + 1) % MAX_QUEUE == queue.start){
		return false;
	}
	
	queue.buffer[queue.end] = id;
	queue.end = (queue.end + 1) % MAX_QUEUE;
	
	return true;
}
uint8_t pop(void){
	uint8_t id = 0;
	
	if( queue.start == queue.end ){
		return ID_None;
	}
	
	id = queue.buffer[queue.start ];
	queue.start = (queue.start + 1) % MAX_QUEUE;
	
	return id;
}

bool checkItem(uint8_t id){
	uint8_t index = 0;
	
	for(index = queue.start; index != queue.end; index = (index + 1) % MAX_QUEUE){
		if(queue.buffer[index] == id){
			return true;
		}
	}
	
	return false;
}

bool removeItem(uint8_t id){
	uint8_t index = 0;
	
	for(index = queue.start; index != queue.end; index = (index + 1) % MAX_QUEUE){
		if(queue.buffer[index] == id){
			break;
		}
	}
	
	if(index == queue.end){
		return false;
	}else{
		for(; (index + 1) % MAX_QUEUE != queue.end; index = (index + 1) % MAX_QUEUE){
			queue.buffer[index] = queue.buffer[index + 1];
		}
		queue.end = (index + 1) % MAX_QUEUE;
	}
	
	return true;
}

void setState_Set_Address(uint8_t id){
	fsmSensor[id] = Set_Address;
}
void setState_Request(uint8_t id){
	fsmSensor[id] = Request;
}
void setState_Command(uint8_t id){
	fsmSensor[id] = Command;
}
void setState_ACK(uint8_t id){
	fsmSensor[id] = Ack;
}
void setState_Response(uint8_t id){
	fsmSensor[id] = Response;
}

uint8_t getState_Sensor(uint8_t id){
	return fsmSensor[id];
}
