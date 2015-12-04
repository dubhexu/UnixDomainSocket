#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "UnixDomainSocket.h"

#define SUN_PATH "test.sck"
#define BUFFER_SIZE (65536)

int _get_short_file_name(const char * szLongName, char * szShortName, int nLen)
{
    char * p = NULL;
    if (!szLongName || !szShortName || nLen <= 0)
        return -1;
    
    p = strrchr((char *)szLongName, '/');
    if (!p)
        p = (char *)szLongName;
    else
        p++; 

    strncpy(szShortName, p, nLen - 1);

    return strlen(szShortName);
}

void _signal_handler( int sig )
{
    switch(sig) {
    case SIGPIPE:   //important, ignore broken pipe
        printf("Received signal SIGPIPE\n");
        return;     //do not exit!
    default:        //other singals
        printf("Received signal 0x%X, exiting...\n", sig);
        break;      //exit
    }
    
    exit(0);
}

int server(int argc, char * argv[])
{
    signal(SIGPIPE, _signal_handler);

    UnixDomainSocketServer server;
    if (server.Listen(SUN_PATH) < 0) {
        printf("server.Listen() failed\n");
        return -1;
    }
    
    unsigned char temp_buffer[BUFFER_SIZE] = {0};
    for (unsigned int i = 0; ; i++) {
        sprintf((char *)temp_buffer, "%d", i);
        int clients = server.Send(temp_buffer, sizeof(temp_buffer));
        if (clients > 0)
            printf("clients:%d, packet:%d\n", clients, i);
        
        usleep(1000);
    }
    
    return 0;
}

int client_simple(int argc, char * argv[])
{
    UnixDomainSocketClient client;
    if (client.Connect(SUN_PATH) < 0) {
        printf("client.Connect() failed\n");
        return -1;
    }
    
    unsigned char temp_buffer[BUFFER_SIZE] = {0};
    while (1) {
        int rc = client.Recv(temp_buffer, sizeof(temp_buffer));
        if (rc <= 0) {
            perror("client.Recv() failed\n");
            break;
        }
        else
            printf("received bytes:%d\n", rc);
    }
}

int client_select(int argc, char * argv[])
{
    UnixDomainSocketClient client;

    if (client.Connect(SUN_PATH) < 0) {
        printf("client.Connect() failed\n");
        return -1;
    }
    int fd = client.GetFd();
    int flag = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, O_NONBLOCK | flag);
    
    unsigned char temp_buffer[BUFFER_SIZE] = {0};
    fd_set fdsetRead;
    timeval timeout;
    while (1) {
        FD_ZERO(&fdsetRead);
        FD_SET(fd, &fdsetRead);
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        int rc = select(fd + 1, &fdsetRead, NULL, NULL, &timeout);
        if (rc < 0) {
            perror("select failed\n");
            return -1;
        }
        else if (rc == 0)  //timeout
            continue;
        else {
            if (FD_ISSET(fd, &fdsetRead)) {
                int total = 0;
                for (unsigned char * p = temp_buffer; ; ) {
                    int len = client.Recv(p, sizeof(temp_buffer) - total);
                    if (len < 0) {
                        if (errno == EAGAIN)
                            break;
                        else {
                            perror("client.Recv() failed");
                            return -1;
                        }
                    }
                    else if (len == 0) {  //if server closed, client recv will return 0
                        perror("client.Recv()==0");
                        return -1;
                    }
                    else {
                        p += len;
                        total += len;
                    }
                }
                printf("received bytes:%d\n", total);
            }
        }
    }
}

int main(int argc, char *argv[])
{
    char program_name[NAME_MAX];
    _get_short_file_name(argv[0], program_name, sizeof(program_name));

    if (strcmp(program_name, "server") == 0)
        return server(argc, argv);
    else if (strcmp(program_name, "client_simple") == 0)
        return client_simple(argc, argv);
    else if (strcmp(program_name, "client_select") == 0)
        return client_select(argc, argv);
    else {
        fprintf(stderr, "illegal app name!\n");
        return -1;
    }
}

