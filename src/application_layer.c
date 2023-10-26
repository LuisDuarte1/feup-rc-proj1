// Application layer protocol implementation


#include "application_layer.h"
#include "application_protocol.h"


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
    size_parameter.type = FILESIZE;
    size_parameter.parameter_size = 4; // sizeof(file_size) ?
    *((int *) ((void *) size_parameter.parameter)) = file_size;
    
    control_parameter_t filename_parameter;
    filename_parameter.type = FILENAME;
    strcpy(filename_parameter.parameter, filename);
    strcat(filename_parameter.parameter, ".received");
    printf("recv_filename: %s", filename_parameter.parameter);
    filename_parameter.parameter_size = strlen(filename_parameter.parameter);


    control_packet_t control_packet;
    control_packet.packet_type = START;
    control_packet.length = 2;
    control_packet.parameters = (control_parameter_t *) malloc(sizeof(control_parameter_t) * 2);
    if(control_packet.parameters == NULL){
        perror("adoro mallocar");
    }
    control_packet.parameters[0] = size_parameter;
    control_packet.parameters[1] = filename_parameter;

    printf("writting control packet...\n");
    write_control_packet(control_packet);
    printf("writting control packet...\n");
    
    char buf[MAX_PAYLOAD_SIZE] = {0};
    int bytes_read = read(fd, &buf, MAX_PAYLOAD_SIZE);
    while(bytes_read != 0){
        data_packet_t data_packet;
        data_packet.data = buf;
        data_packet.data_size = bytes_read;
        printf("writting data packet...\n");
        write_data_packet(data_packet);
        bytes_read = read(fd, &buf, MAX_PAYLOAD_SIZE);
    }   

    control_packet_t end_packet;
    end_packet.length = 0;
    end_packet.packet_type = END;
    write_control_packet(end_packet);
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
    while(bytes = llread(&buf), true){
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
                    if(packet.parameters[i].type == FILESIZE){
                        file_size = *(int *)((void*) packet.parameters[i].parameter);
                    } else if (packet.parameters[i].type == FILENAME){
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
