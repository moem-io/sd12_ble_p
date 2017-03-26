#include "util.h"

char* uint8_t_to_str(uint8_t *data_addr,uint8_t length, bool rev){
    static char string[256];
    char temp[2];
    memset(&string, 0, sizeof(string));
    
    for(int i=0;i<length;i++){
        sprintf(temp,"%02x",data_addr[rev?length-1-i:i]);
        strcat(string,temp);
    }
    
    return string;
}


int8_t gap_disc_addr_check(uint8_t *p_data){
    for(int i=0;i<app_state.net.disc.count;i++){
        if(!memcmp(app_state.net.disc.peer[i].p_addr.addr,p_data, BLE_GAP_ADDR_LEN)){
            NRF_LOG_DEBUG("ADDR FOUND!\r\n");
            return i;
        }
    }
    NRF_LOG_DEBUG("ADDR NOT FOUND!\r\n");
    return GAP_DISC_ADDR_NOT_FOUND;
}


ble_gap_addr_t* gap_disc_id_check(uint8_t *id){
    if(*id == 0){
        return &app_state.dev.parent_addr;
    }
    for(int i=0;i<app_state.net.disc.count;i++){
        if(!memcmp(&app_state.net.disc.peer[i].id,id, sizeof(uint8_t))){
            NRF_LOG_DEBUG("ID FOUND!\r\n");
            return &app_state.net.disc.peer[i].p_addr;
        }
    }
    NRF_LOG_DEBUG("ID NOT FOUND!\r\n");
    return NULL;
}
