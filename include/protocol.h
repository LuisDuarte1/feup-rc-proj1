#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>

// Baudrate settings are defined in <asm/termbits.h>, which is
// included by <termios.h>
#define _POSIX_SOURCE 1 // POSIX compliant source

#define BUF_SIZE 5
#define FLAG 0x7E
#define ADDR_SEND 0x03
#define ADDR_RECV 0X01
#define CONTROL_SET 0x03
#define CONTROL_UA 0x07
#define CONTROL_I0 0x00
#define CONTROL_I1 0x40 

extern bool alarmEnabled;
extern int alarmCount;

typedef enum {
    PACKET_BEGIN = 0,
    ADDR,
    CONTROL,
    BCC,
    DATA,
    BCC2,
    PACKET_END,
    SUCCESS
} packet_status_t;

typedef struct packet_t {
    unsigned char addr;
    unsigned char control;
    unsigned char bcc;
    unsigned char * data;
    unsigned char bcc2;
    int data_size;
    int alloc_size;
    packet_status_t status;
} packet_t;


void init_packet(packet_t * packet);

int write_st(int fd);
int write_ua(int fd);
int write_data(int fd, unsigned char * buf, int size);

void read_packet(int fd, packet_t * packet);

bool validate_packet(packet_t * packet);
void alarmHandler(int signal);


#endif
