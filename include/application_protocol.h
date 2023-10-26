#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>

#include "link_layer.h"

#define DATA 1
#define START 2
#define END 3

#define FILESIZE 0
#define FILENAME 1



typedef struct control_parameter_t{
   unsigned char type;
   unsigned char parameter_size;
   unsigned char parameter[256];
     
} control_parameter_t;

typedef struct control_packet_t{
    unsigned char packet_type;
    control_parameter_t * parameters;
    int length;
    
} control_packet_t;

typedef struct data_packet_t{
    unsigned short data_size;
    unsigned char * data;
    
} data_packet_t;


void write_control_packet(control_packet_t control_packet);
void write_data_packet(data_packet_t dataPacket);
control_packet_t read_control_packet(char * buf, int packet_size);
void process_data_packet(char * buf, int size, int fd);

#endif