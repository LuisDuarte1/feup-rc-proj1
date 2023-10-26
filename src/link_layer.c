// Link layer protocol implementation

#include "link_layer.h"
#include "protocol.h"

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

int fd;
LinkLayer link_layer;

int open_write(LinkLayer connectionParameters){
    fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY);
    (void) signal(SIGALRM, alarm_handler);
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
    alarm_count = 0;
    alarm_enabled = true;
    while(alarm_count < connectionParameters.nRetransmissions && alarm_enabled == true) {
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
            alarm_enabled = true;
            continue;
        }
        printf("Status: %d\n", packet.status);
        if(!validate_packet(&packet)) continue;
        // aqui se o control enviado não for o UA, o loop não falha logo? Porque o alarm foi desativado
        if(packet.control != CONTROL_UA) continue;
        break;
    }
    link_layer = connectionParameters;
    return !(alarm_count < connectionParameters.nRetransmissions);
}

int open_read(LinkLayer connectionParameters){
    

    // aqui lembro-me de termos adicionado o alarme para o read tbm, mas acho que não se deu push
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
        return open_read(connectionParameters) == 0 ? 1 : -1;
    } else if(connectionParameters.role == LlTx){
        return open_write(connectionParameters) == 0 ? 1 : -1;
    }
    return -1;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    alarm_count = 0;
    while(alarm_count < link_layer.nRetransmissions){
        if(write_data(fd, buf, bufSize) == -1) return -1;
        
        packet_t packet;
        alarm_enabled = true;
        alarm(link_layer.timeout);
        init_packet(&packet);
        while(alarm_enabled){
            printf("Reading Packet...\n");
            read_packet(fd, &packet, true);
            if(packet.status == SUCCESS) break;
        }

        if(!validate_packet(&packet)) continue;
        printf("Packet validated: toggle %d, control: 0x%x\n", information_toggle, packet.control); 
        if((packet.control == CONTROL_RR0 && information_toggle) ||
            (packet.control == CONTROL_RR1 && !information_toggle)) break;
    }

    if(alarm_count >= link_layer.nRetransmissions) return -1;
    information_toggle = !information_toggle;
    return bufSize;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    packet_t packet_struct;
    while(true){
        init_packet(&packet_struct);
        read_packet(fd, &packet_struct, false);
        if(packet_struct.status != SUCCESS) continue;
        if(!validate_packet(&packet_struct)){
            if(packet_struct.control == CONTROL_I0 || packet_struct.control == CONTROL_I1){
                printf("Data rejected...\n");
                write_command(fd, true, packet_struct.control == CONTROL_I0 ? CONTROL_REJ0 : CONTROL_REJ1);
                free(packet_struct.data);
            }
            continue;
        }
        if(packet_struct.control == CONTROL_SET){
            write_command(fd, true, CONTROL_UA);
            continue;
        }
        if(packet_struct.control == CONTROL_I0 || packet_struct.control == CONTROL_I1) break;
    }
    
    write_command(fd, true, packet_struct.control == CONTROL_I0 ? CONTROL_RR1 : CONTROL_RR0);
    memcpy(packet, packet_struct.data, packet_struct.data_size);
    free(packet_struct.data);

    return packet_struct.data_size;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{
    /*if (showStatistics) {
        printf("No statistics\n");
    }

    if (link_layer.role == LlTx) {
        
    }
    else {
    
    }
    
    close(fd);
    return 1;*/
}
