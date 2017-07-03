#include "Button.h"

void pin_button_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action) {
    Button_Click_CallBack();
}

__WEAK void Button_Click_CallBack() {

}

ret_code_t Button_Init(void) {

    ret_code_t err_code;

    nrf_drv_gpiote_in_config_t in_config = GPIOTE_CONFIG_IN_SENSE_TOGGLE(true);
    in_config.pull = NRF_GPIO_PIN_PULLUP;

    err_code = nrf_drv_gpiote_in_init(Button_IN, &in_config, pin_button_handler);
    if (err_code != NRF_SUCCESS) {
        return err_code;
    }

    nrf_drv_gpiote_in_event_enable(Button_IN, true);

    return err_code;
}

bool checkButton(void) {
    nrf_delay_ms(10);
    return nrf_gpio_pin_read(Button_IN) > 0;
}
