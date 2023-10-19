#include "protocol.h"


bool alarmEnabled = false;
int alarmCount = 0;

bool information_toggle = false;


void init_packet(packet_t * packet) {
    packet->alloc_size = 0;
    packet->data_size = 0;
}

int write_command(int fd, bool tx, char command)
{
    unsigned char buf[BUF_SIZE] = {0};
    buf[0] = FLAG;
    buf[1] = tx ? ADDR_SEND : ADDR_RECV;
    buf[2] = command;
    buf[3] = buf[1] ^ buf[2];
    buf[4] = FLAG;
    return write(fd, buf, BUF_SIZE);

}

void read_packet(int fd, packet_t * packet, bool tx){
    unsigned char recv_buf;
    int recv_status = read(fd, &recv_buf, 1);
    packet->status = PACKET_BEGIN;
    while(recv_status >= 0 && (alarmEnabled || !tx)){
        packet_status_t new_status = packet->status;
        if(recv_status == 0){
            recv_status = read(fd, &recv_buf, 1);
            
            continue;
        }
        
        switch (packet->status){
            case PACKET_BEGIN:
                if(recv_buf == FLAG){
                    new_status = ADDR;
                }
                break;
            case ADDR:
                packet->addr = recv_buf;
                new_status = CONTROL;
                break;
            
            case CONTROL:
                packet->control = recv_buf;
                new_status = BCC;
                break;
            
            case BCC:
                packet->bcc = recv_buf;
                if(packet->control == CONTROL_I0 || packet->control == CONTROL_I1){
                    new_status = DATA;
                } else {
                    new_status = PACKET_END;
                }
                break;
            case DATA:
                if(packet->data_size == packet->alloc_size){
                    int new_size = packet->alloc_size == 0 ? 10 : packet->alloc_size*2;
        
                    // unsigned char * new_data = (unsigned char *) malloc(new_size);
                    // if(new_data == NULL){
                    //     perror("Adoro mallocar");
                    //     exit(1);
                    // }
                    // if(packet->alloc_size != 0){
                    //     memcpy(new_data, packet->data, packet->data_size);
                    //     free(packet->data);
                    // }
                    
                    packet->data = realloc(packet->data, new_size);
                    if(packet->data == NULL){
                        perror("Adoro mallocar");
                        exit(1);
                    }
                    packet->alloc_size = new_size;
                }
                if(recv_buf == FLAG){
                    new_status = SUCCESS;
                } else {
                    packet->data[packet->data_size] = recv_buf;
                    packet->data_size++;
                }
                break;

            case PACKET_END:
                if(recv_buf == FLAG){
                    new_status = SUCCESS;  
                }
                break;
        }
        packet->status = new_status;
        if(packet->status == SUCCESS) break;
        recv_status = read(fd, &recv_buf, 1);
    }
    if(packet->status == SUCCESS){
        if(packet->control == CONTROL_I0 || packet->control == CONTROL_I1){
            if(packet->data[packet->data_size - 2] == 0x7d){
                packet->bcc2 = FLAG;
                packet->data_size -= 2;
            } else {
                packet->bcc2 = packet->data[packet->data_size-1];
                packet->data_size--;
            }
        }
        alarm(0);
        alarmEnabled = false;
    }
}

bool validate_packet(packet_t *packet)
{
    if(packet->control == CONTROL_I0 || packet->control == CONTROL_I1) {
        int xor = packet->data[0];
        for(int i = 1; i < packet->data_size; i++) xor ^= packet->data[i];
        return (packet->addr ^ packet->control) == packet->bcc && xor == packet->bcc2;
    } else {
        return (packet->addr ^ packet->control) == packet->bcc;
    }
}

int write_data(int fd, unsigned char *buf, int size)
{
    unsigned char packet_start[4];
    packet_start[0] = FLAG;
    packet_start[1] = ADDR_SEND;
    packet_start[2] = information_toggle ? CONTROL_I1 : CONTROL_I0;
    packet_start[3] = packet_start[1] ^ packet_start[2];
    
    unsigned char bcc2 = 0;
    for(int i = 0; i < size; i++) bcc2 ^= buf[i];
    
    if(write(fd, packet_start, 4) == -1) return -1;
    

    unsigned char * stuffed_buf = stuff_packet(buf, &size);
    if(write(fd, stuffed_buf, size) == -1) return -1;
    free(stuffed_buf);
    stuffed_buf = NULL;
    

    if(bcc2 == FLAG){
        const unsigned char bcc_flag[2] = {ESCAPE_CHAR, ESCAPE_FLAG};
        if(write(fd, bcc_flag, 2) == -1) return -1;        
    } else {
        if(write(fd, &bcc2, 1) == -1) return -1;
    } 
    unsigned char flag = FLAG;
    if(write(fd, &flag, 1) == -1) return -1;
    return 0;
}

void alarmHandler(int signal){

    alarmEnabled = false;
    alarmCount++;
       
}

unsigned char * stuff_packet(unsigned char * buf, int * size){

    unsigned char * nbuffer = (unsigned char*) malloc(*size);
    if(nbuffer == NULL){
        perror("rip mallocanss");
        exit(1);
    }
    int old_size = *size;
    int new_size = *size;
    int nbuffer_pos = 0;
    for(int i = 0; i < old_size; i++, nbuffer_pos++){
     if(buf[i] == FLAG || buf[i] == ESCAPE_CHAR){
        new_size++;
        nbuffer = realloc(nbuffer, new_size);
        
        nbuffer[nbuffer_pos] = ESCAPE_CHAR;
        nbuffer_pos++;
        nbuffer[nbuffer_pos] = buf[i] == FLAG ? ESCAPE_FLAG : ESCAPE_ESCAPE;
     } else {
        nbuffer[nbuffer_pos] = buf[i];
     }
    }
    
    *size = new_size;
    return nbuffer; 
}


void destuff_packet(packet_t * packet){
    if(!(packet->control == CONTROL_I0 || packet->control == CONTROL_I1)) 
        return;
    unsigned char * nbuffer = (unsigned char*) malloc(packet->data_size);
 
    if(nbuffer == NULL){
        perror("rip mallocanss");
        exit(1);
    }
    
    int new_size = packet->data_size;
    int nbuffer_pos = 0;
    
    for(int i = 0; i < packet->data_size; i++, nbuffer_pos++){
        if(packet->data[i] == ESCAPE_CHAR){
            nbuffer[nbuffer_pos] = 
                packet->data[i+1] == ESCAPE_FLAG ? FLAG : ESCAPE_CHAR;
            
            i++;
            new_size--;
        } else {
            nbuffer[nbuffer_pos] = packet->data[i];
        }
    }
    
    free(packet->data);
    packet->data = nbuffer;
    packet->data_size = new_size;    
}

