#include "Fuel_Gauge.h"

static volatile char buffer[100] = {0,};

static volatile bool m_xfer_done = false;																																/* Indicates if operation on TWI has ended. */
static const nrf_drv_twi_t m_twi = NRF_DRV_TWI_INSTANCE(APP_TWI_DRIVER_INSTANCE);					/* TWI instance. */

static uint8_t userConfigControl = false;
static uint8_t sealFlag = false;

static const unsigned int BATTERY_CAPACITY = 400; // e.g. 400mAh battery

static const uint8_t SOCI_SET = 15; // Interrupt set threshold at 20%
static const uint8_t SOCI_CLR = 20; // Interrupt clear threshold at 25%

/**
 * @brief I2C events handler.
 */
void twi_handler(nrf_drv_twi_evt_t const * p_event, void * p_context){
    switch (p_event->type)
    {
        case NRF_DRV_TWI_EVT_DONE:
            if (p_event->xfer_desc.type == NRF_DRV_TWI_XFER_RX)
            {
                //data_handler(m_sample);
            }
            m_xfer_done = true;
            break;
        default:
            break;
    }
}

ret_code_t Fuel_Gauge_Init(void){
	ret_code_t error_code;

	const nrf_drv_twi_config_t twi_bq27441_config = {
		 .scl                = ARDUINO_SCL_PIN,
		 .sda                = ARDUINO_SDA_PIN,
		 .frequency          = NRF_TWI_FREQ_100K,
		 .interrupt_priority = APP_IRQ_PRIORITY_LOW,
		 .clear_bus_init     = false
	};

	error_code = nrf_drv_twi_init(&m_twi, &twi_bq27441_config, twi_handler, NULL);

	nrf_drv_twi_enable(&m_twi);
	
	return error_code;

}

bool Fuel_Gauge_Config(void){
	if(Fuel_Gauge_begin()){
		
			if(Fuel_Gauge_EnterConfig(true)){
				
				FG_setCapacity(BATTERY_CAPACITY); // Set the battery capacity
				FG_setGPOUT_Polarity(LOW); // Set GPOUT to active-high
				FG_setGPOUT_Function(BAT_LOW); // Set GPOUT to BAT_LOW mode
				FG_setSOC_Thresholds(SOCI_SET, SOCI_CLR); // Set SOCI set and clear thresholds
				
			}
			
		}else{
			
			return false;
			
		}
		return true;
}

static bool Fuel_Gauge_begin(){
	
	uint16_t deviceType = 0;

	deviceType = readControlWord(BQ27441_CONTROL_DEVICE_TYPE);
	return deviceType == BQ27441_DEVICE_ID;
}
static bool Fuel_Gauge_EnterConfig(uint8_t flag){
	
	if(flag) userConfigControl = true;
	
	if (FG_isSeal())
	{
		sealFlag = true;
		FG_setUnseal(); // Must be unsealed before making changes
	}
	
	if (executeControlWord(BQ27441_CONTROL_SET_CFGUPDATE))
	{
		int16_t timeout = BQ72441_I2C_TIMEOUT;
		while ((timeout--) && (!(FG_getStatus() & BQ27441_FLAG_CFGUPMODE)))
			nrf_delay_ms(1);
		
		if (timeout > 0)
			return true;
	}
	
	return false;
}

static bool Fuel_Gauge_ExitConfig(void){
	
	if (FG_softReset())
	{
		int16_t timeout = BQ72441_I2C_TIMEOUT;
		while ((timeout--) && ((FG_getFlag() & BQ27441_FLAG_CFGUPMODE)))
			nrf_delay_ms(1);
		if (timeout > 0)
		{
			if (sealFlag) FG_setSeal(); // Seal back up if we IC was sealed coming in
			return true;
		}
	}
	return false;
}

static bool FG_setCapacity(uint16_t capacity){
	uint8_t capMSB = capacity >> 8;
	uint8_t capLSB = capacity & 0x00FF;
	uint8_t capacityData[2] = {capMSB, capLSB};
	
	return writeExtendedData(BQ27441_ID_STATE, 10, capacityData, 2);
}
static bool FG_setGPOUT_Polarity(signal sig){
	
	ret_code_t error_code;
	
	uint16_t oldOpConfig = opConfig();
	
	nrf_drv_gpiote_in_config_t in_config = GPIOTE_CONFIG_IN_SENSE_HITOLO(true);
	in_config.pull = NRF_GPIO_PIN_PULLUP;

	error_code = nrf_drv_gpiote_in_init(GPOUT_PIN, &in_config, NULL);
	if(error_code != NRF_SUCCESS){
		return false;
	}

	nrf_drv_gpiote_in_event_enable(GPOUT_PIN, true);
	
	// Check to see if we need to update opConfig:
	if ((sig && (oldOpConfig & BQ27441_OPCONFIG_GPIOPOL)) ||
        (!sig && !(oldOpConfig & BQ27441_OPCONFIG_GPIOPOL)))
		return true;
		
	uint16_t newOpConfig = oldOpConfig;
	if (sig)
		newOpConfig |= BQ27441_OPCONFIG_GPIOPOL;
	else
		newOpConfig &= ~(BQ27441_OPCONFIG_GPIOPOL);
	
	return writeOpConfig(newOpConfig);	
}
static bool FG_setGPOUT_Function(gpout_function func){
	uint16_t oldOpConfig = opConfig();
	
	// Check to see if we need to update opConfig:
	if ((func && (oldOpConfig & BQ27441_OPCONFIG_BATLOWEN)) ||
        (!func && !(oldOpConfig & BQ27441_OPCONFIG_BATLOWEN)))
		return true;
	
	// Modify BATLOWN_EN bit of opConfig:
	uint16_t newOpConfig = oldOpConfig;
	if (func)
		newOpConfig |= BQ27441_OPCONFIG_BATLOWEN;
	else
		newOpConfig &= ~(BQ27441_OPCONFIG_BATLOWEN);

	// Write new opConfig
	return writeOpConfig(newOpConfig);
}
static bool FG_setSOC_Thresholds(uint8_t belowVoltage, uint8_t aboveVoltage){
	
	uint8_t thresholds[2];
	thresholds[0] = belowVoltage;
	thresholds[1] = aboveVoltage;
	
	return writeExtendedData(BQ27441_ID_DISCHARGE, 0, thresholds, 2);
}

static bool FG_setSeal(){
	
	return readControlWord(BQ27441_CONTROL_SEALED);
}
static bool FG_setUnseal(){
	
	if (readControlWord(BQ27441_UNSEAL_KEY)){
		return readControlWord(BQ27441_UNSEAL_KEY);
	}
	return false;
}

static bool FG_isSeal( ){
	uint16_t state = 0;
	
	state = FG_getStatus();
	return state & BQ27441_STATUS_SS;
}

static bool FG_softReset(void){
	return executeControlWord(BQ27441_CONTROL_SOFT_RESET);
}


char* Fuel_Gauge_getBatteryStatus(void){
	
	uint8_t capacity = 0;
	uint16_t voltage = 0;
	
	capacity = FG_getCapacity();
	voltage = FG_getVoltage();
	
	clearBuffer();
	
	sprintf(buffer, "%d%%, %dmV\n", capacity, voltage);
	
	return buffer;
}

static uint16_t FG_getStatus(void){
	return readControlWord(BQ27441_CONTROL_STATUS);
}

static uint16_t FG_getFlag(void){
	return readWord(BQ27441_COMMAND_FLAGS);
}

static uint8_t FG_getCapacity(){
	return readWord(BQ27441_COMMAND_SOC_UNFL);
}
static uint16_t FG_getVoltage(){
	return readWord(BQ27441_COMMAND_VOLTAGE);
}
static uint16_t FG_getCurrent(){
	return readWord(BQ27441_COMMAND_AVG_CURRENT);
}

static bool executeControlWord(uint16_t function){
	
	ret_code_t error_code;
	
	uint8_t subCommandMSB = (function >> 8);
	uint8_t subCommandLSB = (function & 0x00FF);
	uint8_t command[2] = {subCommandLSB, subCommandMSB};
	
	error_code = writeBytes((uint8_t) 0, command, 2);
	return error_code;
}

static uint16_t readWord(uint16_t subAddress){
	uint8_t data[2];
	readBytes(subAddress, data, 2);
	return ((uint16_t) data[1] << 8) | data[0];
}

static uint16_t readControlWord(uint16_t function){
	
	ret_code_t error_code;
	
	uint8_t subCommandMSB = (function >> 8);
	uint8_t subCommandLSB = (function & 0x00FF);
	uint8_t command[2] = {subCommandLSB, subCommandMSB};
	uint8_t temp[2] = {0, 0};
	uint16_t data = 0;
	
	error_code = writeBytes((uint8_t) 0, command, 2);
	if(error_code != NRF_SUCCESS){
		return error_code;
	}
	error_code = readBytes((uint8_t) 0, temp, sizeof(temp));
	if (error_code == NRF_SUCCESS){
		 data = ((uint16_t)temp[1] << 8) | temp[0];
	}else{
		return error_code;
	}
	return data;
}

static ret_code_t writeBytes(uint8_t subAddress, uint8_t* subCommand, uint8_t length){
	
	ret_code_t error_code;
	
	uint8_t data[3] = { 0,};
	data[0] = subAddress;
	data[1] = subCommand[0];
	if(length == 2){
		data[2] = subCommand[1];
	}
	
	m_xfer_done = false;
	error_code = nrf_drv_twi_tx(&m_twi, BQ27441_ADDR, data, length + 1, false);
	if(error_code != NRF_SUCCESS){
		return error_code;
	}
	while (m_xfer_done == false);
	return error_code;
}
static bool writeExtendedData(uint8_t classID, uint8_t offset, uint8_t * data, uint8_t len){
	if (len > 32)
		return false;
	
	if (!userConfigControl) Fuel_Gauge_EnterConfig(false);
	
	if (!blockDataControl()) // // enable block data memory control
		return false; // Return false if enable fails
	if (!blockDataClass(classID)) // Write class ID using DataBlockClass()
		return false;
	
	blockDataOffset(offset / 32); // Write 32-bit block offset (usually 0)
	computeBlockChecksum(); // Compute checksum going in
	uint8_t oldCsum = blockDataChecksum();

	// Write data bytes:
	for (int i = 0; i < len; i++)
	{
		// Write to offset, mod 32 if offset is greater than 32
		// The blockDataOffset above sets the 32-bit block
		writeBlockData((offset % 32) + i, data[i]);
	}
	
	// Write new checksum using BlockDataChecksum (0x60)
	uint8_t newCsum = computeBlockChecksum(); // Compute the new checksum
	writeBlockChecksum(newCsum);

	if (!userConfigControl) Fuel_Gauge_ExitConfig();
	
	return true;
}
static bool writeBlockData(uint8_t offset, uint8_t data){
	
	uint8_t address = offset + BQ27441_EXTENDED_BLOCKDATA;
	return writeBytes(address, &data, 1);
}
static bool writeBlockChecksum(uint8_t csum){
	return writeBytes(BQ27441_EXTENDED_CHECKSUM, &csum, 1);	
}

static ret_code_t readBytes(uint8_t subAddress, uint8_t* data, uint8_t length){
	
	ret_code_t error_code;
	
	uint8_t temp_subAddress = subAddress;
	
	m_xfer_done = false;
	error_code = nrf_drv_twi_tx(&m_twi, BQ27441_ADDR, &temp_subAddress, sizeof(temp_subAddress), false);
	if(error_code != NRF_SUCCESS){
		return error_code;
	}
	while (m_xfer_done == false);
	
	m_xfer_done = false;
	error_code = nrf_drv_twi_rx(&m_twi, BQ27441_ADDR, data, length);
	if(error_code != NRF_SUCCESS){
		return error_code;
	}
	while (m_xfer_done == false);
	
	return error_code;
}

static bool blockDataControl(void){
	uint8_t enableByte = 0x00;
	return writeBytes(BQ27441_EXTENDED_CONTROL, &enableByte, 1);
}

static bool blockDataClass(uint8_t id){
	return writeBytes(BQ27441_EXTENDED_DATACLASS, &id, 1);
}

static bool blockDataOffset(uint8_t offset){
	return writeBytes(BQ27441_EXTENDED_DATABLOCK, &offset, 1);
}

static uint8_t blockDataChecksum(void){
	uint8_t csum;
	writeBytes(BQ27441_EXTENDED_CHECKSUM, &csum, 1);
	return csum;
}

static uint8_t computeBlockChecksum(void){
	
	uint8_t data[32];
	readBytes(BQ27441_EXTENDED_BLOCKDATA, data, 32);

	uint8_t csum = 0;
	for (int i=0; i<32; i++)
	{
		csum += data[i];
	}
	csum = 255 - csum;
	
	return csum;
}

static uint16_t opConfig(void){
	return readWord(BQ27441_EXTENDED_OPCONFIG);
}
static bool writeOpConfig(uint16_t value){
	uint8_t opConfigMSB = value >> 8;
	uint8_t opConfigLSB = value & 0x00FF;
	uint8_t opConfigData[2] = {opConfigMSB, opConfigLSB};
	
	// OpConfig register location: BQ27441_ID_REGISTERS id, offset 0
	return writeExtendedData(BQ27441_ID_REGISTERS, 0, opConfigData, 2);	
}

static void clearBuffer(void){
	buffer[0] = '\0';
}
