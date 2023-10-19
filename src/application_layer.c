// Application layer protocol implementation

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "application_layer.h"
#include "link_layer.h"


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

void writeControlPacket(control_packet_t control_packet){

    int buffer_alloc = 10;
    int buffer_size = 0;
    unsigned char * buffer = (unsigned char *) realloc(NULL, buffer_size);
    if (buffer == NULL){
        perror("adoro reallocanss");
        exit(1);
    }
    
    buffer[buffer_size] = control_packet.packet_type;
    buffer_size++;
    
    for(int i = 0; i < control_packet.length; i++){
    
        if(buffer_alloc < (buffer_size + 2 + control_packet.parameters[i].parameter_size)){
            buffer_alloc = buffer_size + 2 + control_packet.parameters[i].parameter_size + 40;
            buffer = (unsigned char *) realloc(buffer, buffer_alloc);
        }
    
        buffer[buffer_size] = control_packet.parameters[i].type;
        buffer_size++;
        *(buffer + buffer_size) = 
            control_packet.parameters[i].parameter_size;
        buffer_size++;
        memcpy(buffer + buffer_size, control_packet.parameters[i].parameter, 
            control_packet.parameters[i].parameter_size);
        buffer_size += control_packet.parameters[i].parameter_size;
    }
    
    if(llwrite(buffer, buffer_size) == -1){
        perror("Something went wrong while writing application control packet");
        exit(1); 
    }
    free(buffer);
    
} 


void writeDataPacket(data_packet_t dataPacket){
    unsigned char * buffer = realloc(NULL, 3 + dataPacket.data_size);
    if (buffer == NULL){
        perror("adoro reallocanss");
        exit(1);
    }
    
    buffer[0] = 1;
    *((unsigned short *)(buffer + 1)) = dataPacket.data_size;
    memcpy(buffer + 3, dataPacket.data, dataPacket.data_size);
    if(llwrite(buffer, 3 + dataPacket.data_size) == -1){
        perror("Something went wrong while writing application data packet");
        exit(1); 
    }
    free(buffer);
    
}

void mainTx(const char * filename){
    int fd = open(filename, O_RDONLY);
    if(fd == -1){
        perror("couldn't open file");
        //TODO(luisd): close connectiion maybe?
        exit(1);
    }
    struct stat st;
    stat(filename, &st);
    
    unsigned int file_size = st.st_size;
    control_parameter_t size_parameter;
    size_parameter.type = 0;
    size_parameter.parameter_size = 4;
    *((int *) ((void *) size_parameter.parameter)) = file_size;
    
    control_packet_t control_packet;
    control_packet.packet_type = 2;
    control_packet.length = 1;
    control_packet.parameters = &size_parameter;
    
    printf("writting data packet...\n");
    writeControlPacket(control_packet);
    
    char buf[1024] = {0};
    int bytes_read = read(fd,&buf,1024);
    while(bytes_read != 0){
        //bloaldt
        int bytes_read = read(fd,&buf,1024);
    }   
}
void mainRx(const char * filename){
    char buf[1024] = {0};
    if(llread(buf) == -1) {
        perror("Something went wrong while reading packet");
    }
    for(int i = 0; i < 20; i++){
        printf("0x%X\n", buf[i]);
    }
    
}
void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    LinkLayer connectionParameters;
    strncpy(connectionParameters.serialPort, serialPort, 50);
    
    if(strcmp(role, "tx") == 0){
        connectionParameters.role = LlTx;
    } else if (strcmp(role, "rx") == 0){
        connectionParameters.role = LlRx;
    } else {
        exit(1);
    }
    connectionParameters.baudRate = baudRate;
    connectionParameters.nRetransmissions = nTries;
    connectionParameters.timeout = timeout;

    if(llopen(connectionParameters) != 1){
        printf("Couldn't connect to peer\n");
        exit(1);
    }

    if(connectionParameters.role == LlTx){
        mainTx(filename);
    } else {
        mainRx(filename);
    }
}
