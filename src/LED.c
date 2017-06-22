#include "LED.h"
#include "Button.h"

#define NRF_LOG_MODULE_NAME "LED"

APP_PWM_INSTANCE(battery,1);                   // Create the instance "PWM1" using TIMER1.
APP_PWM_INSTANCE(connect,2);

static volatile bool ready_flag = false;            // A flag indicating PWM status.

uint8_t bufferRGB[len_bufferRGB];


void pwm_ready_callback(uint32_t pwm_id)    // PWM callback function
{
    ready_flag = true;
}

ret_code_t LED_Init(){
    
        ret_code_t err_code;
    

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
    nrf_delay_ms(1000);
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

bool LED_Control(uint8_t* string){
    
    if(str_to_int(string) != NULL){
        LED_Red(bufferRGB[0]);
        if(bufferRGB[0] > 0){
            LED_Green(bufferRGB[1], false);
        }else{
            LED_Green(bufferRGB[1], true);
        }
        LED_Blue(bufferRGB[2]);
    }else{
        return false;
    }
    
    return true;
    
}

void LED_Red( uint8_t value){
    if(value != 0){
        ready_flag = false;
         app_pwm_enable(&battery);
        value = ((100 * (uint16_t)value) / 255);
        while (app_pwm_channel_duty_set(&battery, 0, value) == NRF_ERROR_BUSY);
        while (!ready_flag);
        
    }else{
        app_pwm_disable(&battery);
    }
}
void LED_Green( uint8_t value, bool flag){
    if(value != 0){
        ready_flag = false;
         app_pwm_enable(&battery);
        value = ((100 * (uint16_t)value) / 255);
        while (app_pwm_channel_duty_set(&battery, 1, value) == NRF_ERROR_BUSY);
        while (!ready_flag);
        
    }else{
        if(flag){
            app_pwm_disable(&battery);
        }
    }
}
void LED_Blue(uint8_t value){
    if(value != 0){
        ready_flag = false;
         app_pwm_enable(&connect);
        value = ((100 * (uint16_t)value) / 255);
        while (app_pwm_channel_duty_set(&connect, 0, value) == NRF_ERROR_BUSY);
        while (!ready_flag);
        
    }else{
        app_pwm_disable(&connect);
    }
}

uint8_t* str_to_int(uint8_t * string){
    uint8_t i = 0;
    
    char ten = 0;
    char one = 0;
    
    if(strlen(string) != 6){
        return NULL;
    }
    
    for(i = 0; i < 3; i++){
        
        ten = string[ (2*(i+1)) - 2];
        one = string[ 2*i + 1 ];
        
        bufferRGB[i] = 0;
        if(ten >= 'A' && ten < 'G'){
            ten = 10 + (ten - 'A');
        }else if( ten >= '0' && ten <= '9'){
            ten = ten - '0';
        }else{
            return NULL;
        }
        
        if(one >= 'A' && one < 'G'){
            one = 10 + (one - 'A');
        }else if( one >= '0' && one <= '9'){
            one = one - '0';
        }else{
            return NULL;
        }
        
        bufferRGB[i] = (ten << 4) | one;
    }    
    // LOG_D("R : %d, G : %d, B : %d\r\n",bufferRGB[0], bufferRGB[1], bufferRGB[2]);
    
    return bufferRGB;
}
