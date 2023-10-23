// Application layer protocol implementation

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>

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
    unsigned char * buffer = (unsigned char *) realloc(NULL, buffer_alloc);
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
    for(int i = 0; i < buffer_size; i++){
        printf("0x%X ", buffer[i]);
    }
    printf("\n");
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

control_packet_t read_control_packet(char * buf, int packet_size){
    control_packet_t packet;
    packet.packet_type = buf[0];
    packet.length = 0;
    packet.parameters = NULL;
    int parameter_size = 0;
    int parameter_value_it = 0;
    for(int i = 1; i < packet_size; i++){
        if(parameter_size == 0){
            packet.length++;
            packet.parameters = (control_parameter_t *) realloc(packet.parameters, packet.length);
            packet.parameters[packet.length - 1].type = buf[i];
            i++;
            packet.parameters[packet.length - 1].parameter_size = buf[i];
            parameter_size = buf[i];
            parameter_value_it = 0;
            continue;
        } else {
            packet.parameters[packet.length - 1].parameter[parameter_value_it] = buf[i];
            parameter_value_it++;
            parameter_size--;
        }

    }
    printf("\n");
    return packet;
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
    printf("Filesize: %d bytes\n", file_size);
    control_parameter_t size_parameter;
    size_parameter.type = 0;
    size_parameter.parameter_size = 4;
    *((int *) ((void *) size_parameter.parameter)) = file_size;
    
    control_parameter_t filename_parameter;
    filename_parameter.type = 0;
    strcpy(filename_parameter.parameter, filename);
    strcat(filename_parameter.parameter, ".received");
    printf("recv_filename: %s", filename_parameter.parameter);
    filename_parameter.parameter_size = strlen(filename_parameter.parameter);


    control_packet_t control_packet;
    control_packet.packet_type = 2;
    control_packet.length = 2;
    control_packet.parameters = (control_parameter_t *) malloc(sizeof(control_parameter_t) * 2);
    if(control_packet.parameters == NULL){
        perror("adoro mallocar");
    }
    control_packet.parameters[0] = size_parameter;
    control_packet.parameters[1] = filename_parameter;

    printf("writting control packet...\n");
    writeControlPacket(control_packet);
    printf("writting control packet...\n");
    
    char buf[1000] = {0};
    int bytes_read = read(fd,&buf,1000);
    while(bytes_read != 0){
        data_packet_t data_packet;
        data_packet.data = buf;
        data_packet.data_size = bytes_read;
        printf("writting data packet...\n");
        writeDataPacket(data_packet);
        bytes_read = read(fd,&buf,1000);
    }   

    control_packet_t end_packet;
    end_packet.length = 0;
    end_packet.packet_type = 3;
    writeControlPacket(end_packet);
}

void process_data_packet(char * buf, int size, int fd){
    buf += 3;
    size -= 3;
    if(write(fd, buf, size) == -1){
        perror("rip");
        exit(1);
    };
}

void mainRx(const char * filename){
    char buf[1024] = {0};

    int read_state = -1;
    int bytes = 0;
    int read_bytes = 0;
    int file_size = 0;
    char recv_filename[256] = {0};
    bool control_exit = false;
    int fd = -1;
    control_packet_t packet;
    while(bytes = llread(&buf), bytes != -1){
        printf("bytes: %d\n", bytes);
        if(bytes == 0) continue;
        read_state = buf[0];
        switch (read_state)
        {
            case 1:
                process_data_packet(buf, bytes, fd);
                break;
            case 2:
                packet = read_control_packet(buf, bytes);
                for(int i = 0; i < packet.length; i++){
                    if(packet.parameters[i].type == 0){
                        file_size = *(int *)((void*) packet.parameters[i].parameter);
                    } else if (packet.parameters[i].type == 1){
                        memcpy(recv_filename, packet.parameters[i].parameter, packet.parameters[i].parameter_size);
                    }
                }
                fd = open(filename, O_WRONLY | O_CREAT, 0666);
                if(fd == -1){
                    perror("failed to open file");
                    exit(1);
                }
                break;
            case 3:
                control_exit = true;
                break;
            default:
                printf("State: %d not recognized...\n", read_state);
                exit(1);
        }
        read_bytes += bytes;
        if(control_exit == true) break;
    }
    close(fd);
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
