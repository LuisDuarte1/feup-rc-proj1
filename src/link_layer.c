// Link layer protocol implementation

#include "link_layer.h"
#include "protocol.h"

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

int fd;
LinkLayer linkLayer;

int openWrite(LinkLayer connectionParameters){
    fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY);

    if (fd < 0)
    {
        perror(connectionParameters.serialPort);
        exit(-1);
    }

    struct termios oldtio;
    struct termios newtio;

    // Save current port settings
    if (tcgetattr(fd, &oldtio) == -1)
    {
        perror("tcgetattr");
        exit(-1);
    }

    // Clear struct for new port settings
    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = B9600 | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 2; // Inter-character timer unused
    newtio.c_cc[VMIN] = 0;  // Blocking read until 5 chars received

    // VTIME e VMIN should be changed in order to protect with a
    // timeout the reception of the following character(s)

    // Now clean the line and activate the settings for the port
    // tcflush() discards data written to the object referred to
    // by fd but not transmitted, or data received but not read,
    // depending on the value of queue_selector:
    //   TCIFLUSH - flushes data received but not read.
    tcflush(fd, TCIOFLUSH);

    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }
    alarmCount = 0;
    alarmEnabled = true;
    while(alarmCount < connectionParameters.nRetransmissions && alarmEnabled == true) {
        printf("Writing SET...\n");
        if(write_command(fd, true, CONTROL_SET) == -1){
            perror("error while writing");
            continue;
        }
        alarm(connectionParameters.timeout);
        packet_t packet;
        init_packet(&packet);
        read_packet(fd, &packet, true);
        if(packet.status != SUCCESS){
            alarmEnabled = true;
            continue;
        }
        printf("Status: %d\n", packet.status);
        if(!validate_packet(&packet)) continue;
        if(packet.control != CONTROL_UA) continue;
        break;
    }
    linkLayer = connectionParameters;
    return !(alarmCount < connectionParameters.nRetransmissions);
}

int openRead(LinkLayer connectionParameters){
    
    fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        perror(connectionParameters.serialPort);
        exit(-1);
    }

    struct termios oldtio;
    struct termios newtio;

    // Save current port settings
    if (tcgetattr(fd, &oldtio) == -1)
    {
        perror("tcgetattr");
        exit(-1);
    }

    // Clear struct for new port settings
    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = B9600 | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 2; // Inter-character timer unused
    newtio.c_cc[VMIN] = 0;  // Blocking read until 5 chars received

    // VTIME e VMIN should be changed in order to protect with a
    // timeout the reception of the following character(s)

    // Now clean the line and activate the settings for the port
    // tcflush() discards data written to the object referred to
    // by fd but not transmitted, or data received but not read,
    // depending on the value of queue_selector:
    //   TCIFLUSH - flushes data received but not read.
    tcflush(fd, TCIOFLUSH);

    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }
        
    //TODO(luisd): timeout after a few seconds? 
    packet_t packet;
    init_packet(&packet);
    while(true){
        while(true){

            read_packet(fd, &packet, false);

            if(packet.status == SUCCESS) break;
            //recv_status = read(fd, &recv_buf, 1);     
        }
        printf("recved something\n");
        if(!validate_packet(&packet)) continue;
        if(packet.control != CONTROL_SET) continue;
        printf("RECVed SET... sending UA\n");
        if(write_command(fd, true, CONTROL_UA) == -1){
            perror("error writing UA");
            continue;
        } 
        break;
    }
    return 0;
}

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{
    if(connectionParameters.role == LlRx){
        return openRead(connectionParameters) == 0 ? 1 : -1;
    } else if(connectionParameters.role == LlTx){
        return openWrite(connectionParameters) == 0 ? 1 : -1;
    }
    return -1;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    alarmCount = 0;
    while(alarmCount < linkLayer.nRetransmissions){
        if(write_data(fd, buf, bufSize) == -1) {
            return -1;
        }
        packet_t packet;
        alarmEnabled = true;
        alarm(linkLayer.timeout);
        init_packet(&packet);
        while(alarmEnabled){
            printf("Reading Packet...\n");
            read_packet(fd, &packet, true);
            if(packet.status == SUCCESS) break;
            
        }
        if(!validate_packet(&packet)) continue;
        printf("Packet validated: toggle %d, control: 0x%x\n", information_toggle, packet.control); 
        if((packet.control == CONTROL_RR0 && information_toggle) ||
            (packet.control == CONTROL_RR1 && !information_toggle)) break;
    }
    //FIXME (luisd): this can break, but oh well it's almost impossible
    if(alarmCount >= linkLayer.nRetransmissions) return -1;
    information_toggle = !information_toggle;
    return bufSize;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    
    packet_t packetStruct;
    while(true){
        init_packet(&packetStruct);
        read_packet(fd, &packetStruct, false);
        if(packetStruct.status != SUCCESS) continue;
        if(!validate_packet(&packetStruct)){
            if(packetStruct.control == CONTROL_I0 || packetStruct.control == CONTROL_I1){
                printf("Data rejected...\n");
                write_command(fd, true, packetStruct.control == CONTROL_I0 ? CONTROL_REJ0 : CONTROL_REJ1);
                free(packetStruct.data);
            }
            continue;
        }
        if(packetStruct.control == CONTROL_SET){
            write_command(fd, true, CONTROL_UA);
            continue;
        }
        if(packetStruct.control == CONTROL_I0 || packetStruct.control == CONTROL_I1) break;
    }
    
    write_command(fd, true, packetStruct.control == CONTROL_I0 ? CONTROL_RR1 : CONTROL_RR0);
    memcpy(packet, packetStruct.data, packetStruct.data_size);
    free(packetStruct.data);

    return packetStruct.data_size;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{
    // TODO

    return 1;
}
