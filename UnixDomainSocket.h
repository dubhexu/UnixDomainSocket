#ifndef _UNIXDOMAINSOCKET_H_
#define _UNIXDOMAINSOCKET_H_

#include <limits.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>

#include <list>
using namespace std;

class UnixDomainSocket {
public: 
    UnixDomainSocket();
    ~UnixDomainSocket();

public:
    //get socket fd
    int GetFd() { return _fd; }
    
protected:
    int _fd;  //server socket FD
    char _path[NAME_MAX];
};

class UnixDomainSocketServer : public UnixDomainSocket {
public:
    UnixDomainSocketServer();
    ~UnixDomainSocketServer();

public: 
    //listen and start accept thread
    int Listen(const char * path, const mode_t mode = 0666);

    //send data to all clients
    int Send(const unsigned char * buffer, int len);

protected:
    void AddClient(int client_fd);
    static void * ServerThread(void *p); 
    
protected:
    pthread_mutex_t _mutex;  //protect clients list
    pthread_t _pid;  //accept thread pid
    list <int> _clients;  //clients socket FD
};

class UnixDomainSocketClient : public UnixDomainSocket {
public:
    //connect to server
    int Connect(const char * path);

    //recv data from server
    int Recv(unsigned char * buffer, int max_len);
};
#endif