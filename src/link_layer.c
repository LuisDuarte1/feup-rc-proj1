// Link layer protocol implementation

#include "link_layer.h"
#include "protocol.h"

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

int fd;

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
        if(write_st(fd) == -1){
            perror("error while writing");
            continue;
        }
        alarm(connectionParameters.timeout);
        packet_t packet;
        init_packet(&packet);
        read_packet(fd, &packet);
        if(packet.status != SUCCESS){
            alarmEnabled = true;
        }
        printf("Status: %d\n", packet.status);
        if(!validate_packet(&packet)) continue;
        if(packet.control != CONTROL_UA) continue;
        break;
    }
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

    unsigned char recv_buf;
    int recv_status = read(fd, &recv_buf, 1);
    printf("recv_status: %d", recv_status);
        
    //TODO(luisd): timeout after a few seconds? 
    packet_t packet;
    init_packet(&packet);
    alarmEnabled = true;
    while(true){
        while(true){

            read_packet(fd, &packet);

            if(packet.status == SUCCESS) break;
            //recv_status = read(fd, &recv_buf, 1);     
        }
        if(!validate_packet(&packet)) continue;
        if(packet.control != CONTROL_SET) continue;
        printf("RECVed SET... sending UA\n");
        if(write_ua(fd) == -1){
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
        return openRead(connectionParameters);
    } else if(connectionParameters.role == LlTx){
        return openWrite(connectionParameters);
    }
    return 1;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    // TODO

    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    // TODO

    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{
    // TODO

    return 1;
}
