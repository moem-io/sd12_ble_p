#ifndef __SENSOR_CONFIG_H__
#define __SENSOR_CONFIG_H__

#define pinID1 23
#define pinID2 24
#define pinID3 25
#define pinID4 3
#define pinID5 17

#define BUFFER_QUEUE_SIZE 6

typedef enum {
    Request = '1',
    Command,
    Response,
    Ack,
    Set_Address,
    State_None
} Packet_Mode;

typedef enum {
    Th = 'T',
    Pressure = 'P',
    Light = 'L',
    Button = 'B',
    Human = 'H',
    Sound = 'S',
    Rgb = 'R',
    Ir = 'I',
    Buzzer = 'Z',
		Sensor_None = 0,
} Sensor_Type;

enum num_ID {
    ID1 = 1,
    ID2,
    ID3,
    ID4,
    ID5,
    ID_GPOUT,
    ID_CHG,
    ID_BUTTON,
    ID_None
};

#endif
