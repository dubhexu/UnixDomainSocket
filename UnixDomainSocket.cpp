#include "UnixDomainSocket.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>

UnixDomainSocket::UnixDomainSocket()
    : _fd(-1)
{
}

UnixDomainSocket::~UnixDomainSocket()
{
    if (_fd > 0) {
        close(_fd);
        _fd = -1;
    }
}

UnixDomainSocketServer::UnixDomainSocketServer()
    : _pid(0)
{
    pthread_mutex_init(&_mutex, NULL);
}

UnixDomainSocketServer::~UnixDomainSocketServer()
{
    pthread_mutex_destroy(&_mutex); 
}

int UnixDomainSocketServer::Listen(const char * path, const mode_t mode)
{
    unlink(path);  //remove old file

    _fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (_fd < 0) {
        perror("socket() failed");
        return -1;
    }

    struct sockaddr_un local;
    memset(&local, 0, sizeof(struct sockaddr_un));
    local.sun_family = AF_UNIX;
    snprintf(local.sun_path, sizeof(local.sun_path), "%s", path);
    if (bind(_fd, (struct sockaddr *)&local, sizeof(local)) < 0)  {
        perror("bind() failed");
        return -1;
    }

    if (chmod(path, mode) < 0){
        perror("chmod() failed");
        return -1;
    }
    
    if (listen(_fd, 10) < 0) {
        perror("listen() failed");
        return -1;
    }
    
    if (pthread_create(&_pid, NULL, ServerThread, (void *)this) != 0) {
        perror("pthread_create() failed");
        return -1;
    }
    
    return 0;
}

int UnixDomainSocketServer::Send(const unsigned char * buffer, int len)
{
    int clients = 0;
    
    pthread_mutex_lock(&_mutex);
    for (list<int>::iterator it=_clients.begin(); it!=_clients.end(); ) {
        int client_fd = *it;
        if (send(client_fd, buffer, len, 0) < 0) {  //if failed, remove fd from clients
            perror("send() failed");
            close(client_fd);
            _clients.erase(it++);
            printf("client %d removed\n", client_fd);
        }
        else {
            ++it;
            clients++;
        }
    }
    pthread_mutex_unlock(&_mutex);
    
    return clients;
}

void * UnixDomainSocketServer::ServerThread(void * p)
{
    printf("listener start\n");
    
    UnixDomainSocketServer * pSelf = (UnixDomainSocketServer *)p;
    while (1) {
        int client_fd = accept(pSelf->GetFd(), NULL, NULL);
        if (client_fd < 0) {
            perror("accept() failed");
            continue;
        }
        else
            pSelf->AddClient(client_fd);
    }
} 

void UnixDomainSocketServer::AddClient(int client_fd)
{
    pthread_mutex_lock(&_mutex);
    _clients.push_back(client_fd);
    pthread_mutex_unlock(&_mutex);
    
    printf("client %d added\n", client_fd);
}

int UnixDomainSocketClient::Connect(const char * path)
{
    _fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (_fd < 0) {
        perror("socket() failed");
        return -1;
    }
    
    struct sockaddr_un local;
    memset(&local, 0, sizeof(struct sockaddr_un));
    local.sun_family = AF_UNIX;
    snprintf(local.sun_path, sizeof(local.sun_path), "%s", path);
    if (connect(_fd, (struct sockaddr *)&local, sizeof(local)) < 0) {
        perror("connect() failed");
        return -1;
    }
    
    printf("connect sucessful\n");
    return 0;
}

int UnixDomainSocketClient::Recv(unsigned char * buffer, int max_len)
{
    return recv(_fd, buffer, max_len, 0);
}