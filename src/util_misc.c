#include "util_misc.h"

char *uint8_t_to_str(uint8_t *data_addr, uint8_t length, bool rev) {
    static char string[256];
    char temp[2];
    memset(&string, 0, sizeof(string));

    for (int i = 0; i < length; i++) {
        sprintf(temp, "%02x", data_addr[rev ? length - 1 - i : i]);
        for (int j = 0; j < 2; j++)
            temp[j] = toupper(temp[j]);
        strcat(string, temp);
    }

    return string;
}

//Struct to uint8_t
//uint8_t* struct_to_uint8_t(void * struct_addr. int size){
//    static uint8_t buff[20];
//    memset(&buff, 0, sizeof(buff));

//    memcpy(buff, struct_addr, size);
//    
//    return buff;
//}
