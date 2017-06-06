#include "LED.h"
#include "Button.h"

APP_PWM_INSTANCE(battery,1);                   // Create the instance "PWM1" using TIMER1.
APP_PWM_INSTANCE(connect,2);

static volatile bool ready_flag;            // A flag indicating PWM status.

void pwm_ready_callback(uint32_t pwm_id)    // PWM callback function
{
    ready_flag = true;
}

ret_code_t LED_Init(){
	
		ret_code_t err_code;
	
//		nrf_drv_gpiote_in_config_t out_config_red = GPIOTE_CONFIG_IN_SENSE_HITOLO(true);
//		out_config_red.pull = NRF_GPIO_PIN_PULLUP;

//		err_code = nrf_drv_gpiote_in_init(Red, &out_config_red, NULL);
//	
//		nrf_drv_gpiote_in_config_t out_config_blue = GPIOTE_CONFIG_IN_SENSE_HITOLO(true);
//		out_config_blue.pull = NRF_GPIO_PIN_PULLUP;

//		err_code = nrf_drv_gpiote_in_init(Blue, &out_config_blue, NULL);
//		
//		nrf_drv_gpiote_in_config_t out_config_green = GPIOTE_CONFIG_IN_SENSE_HITOLO(true);
//		out_config_green.pull = NRF_GPIO_PIN_PULLUP;

//		err_code = nrf_drv_gpiote_in_init(Green, &out_config_green, NULL);
		
		/* 2-channel PWM, 200Hz, output on DK LED pins. */
    app_pwm_config_t pwm1_cfg = APP_PWM_DEFAULT_CONFIG_2CH(5000L, Red, Green);

    /* Switch the polarity of the second channel. */
		pwm1_cfg.pin_polarity[0] = APP_PWM_POLARITY_ACTIVE_LOW;
    pwm1_cfg.pin_polarity[1] = APP_PWM_POLARITY_ACTIVE_LOW;
		
    /* Initialize and enable PWM. */
    err_code = app_pwm_init(&battery,&pwm1_cfg,pwm_ready_callback);
    if(err_code != NRF_SUCCESS){
			return err_code;
		}
	
	 /* 2-channel PWM, 200Hz, output on DK LED pins. */
    app_pwm_config_t pwm2_cfg = APP_PWM_DEFAULT_CONFIG_1CH(5000L, Blue);

    /* Switch the polarity of the second channel. */
    pwm2_cfg.pin_polarity[0] = APP_PWM_POLARITY_ACTIVE_LOW;

    /* Initialize and enable PWM. */
    err_code = app_pwm_init(&connect,&pwm2_cfg,pwm_ready_callback);
    if(err_code != NRF_SUCCESS){
			return err_code;
		}
		
		return err_code;
}

void LED_Not_Enough(void){
	app_pwm_enable(&battery);
	while (app_pwm_channel_duty_set(&battery, 0, 25) == NRF_ERROR_BUSY);
	while (!ready_flag);
	nrf_delay_ms(500);
	app_pwm_disable(&battery);
}
void LED_Charging(void){
	LED_Not_Enough();
}
void LED_Enough(void){
	app_pwm_enable(&battery);
	while (app_pwm_channel_duty_set(&battery, 1, 25) == NRF_ERROR_BUSY);
	while (!ready_flag);
	nrf_delay_ms(500);
	app_pwm_disable(&battery);
}

void LED_Not_Connect(void){
	app_pwm_enable(&connect);
	while (app_pwm_channel_duty_set(&connect, 0, 25) == NRF_ERROR_BUSY);
	while (!ready_flag);
	nrf_delay_ms(500);
	app_pwm_disable(&connect);
}
void LED_Connect(void){
	app_pwm_enable(&battery);
	while (app_pwm_channel_duty_set(&battery, 1, 25) == NRF_ERROR_BUSY);
	while (!ready_flag);
	nrf_delay_ms(500);
	app_pwm_disable(&battery);
}

void LED_Red(bool state, uint8_t value){
    if(state){
         app_pwm_enable(&battery);
        value = (100 * (uint16_t)value) / 255;
        while (app_pwm_channel_duty_set(&battery,1, value) == NRF_ERROR_BUSY);
        while (!ready_flag);
    }else{
        app_pwm_disable(&battery);
    }
}
void LED_Green(bool state, uint8_t value){
    if(state){
         app_pwm_enable(&battery);
        value = (100 * (uint16_t)value) / 255;
        while (app_pwm_channel_duty_set(&battery, 0, value) == NRF_ERROR_BUSY);
        while (!ready_flag);
    }else{
        app_pwm_disable(&battery);
    }
}
void LED_Blue(bool state, uint8_t value){
    if(state){
         app_pwm_enable(&connect);
        value = (100 * (uint16_t)value) / 255;
        while (app_pwm_channel_duty_set(&connect, 0, value) == NRF_ERROR_BUSY);
        while (!ready_flag);
    }else{
        app_pwm_disable(&connect);
    }
}