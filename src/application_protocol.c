#include "application_protocol.h"

void write_control_packet(control_packet_t control_packet){

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

    //pode-se apagar?
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

void write_data_packet(data_packet_t dataPacket){
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
        
        } else {
            packet.parameters[packet.length - 1].parameter[parameter_value_it] = buf[i];
            parameter_value_it++;
            parameter_size--;
        }

    }
    printf("\n");
    return packet;
}

void process_data_packet(char * buf, int size, int fd){
    buf += 3;
    size -= 3;
    if(write(fd, buf, size) == -1){
        perror("rip");
        exit(1);
    }
}