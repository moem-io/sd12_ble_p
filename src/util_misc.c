#include "util_misc.h"

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

