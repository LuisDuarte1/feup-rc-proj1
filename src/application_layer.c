// Application layer protocol implementation

#include <string.h>
#include <stdlib.h>
#include <stdio.h>


#include "application_layer.h"
#include "link_layer.h"

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
        if(llwrite("o luis e canss", 15) != 0){
            printf("A lena merdou\n");
        }
    } else {
        char pog[MAX_PAYLOAD_SIZE] = {0};
        llread(pog);
        printf("Data: %s\n", pog);

    }
}
