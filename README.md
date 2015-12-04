Single producer/multiple consumers AF_UNIX socket program
# Build
  g++ UnixDomainSocket.cpp test.cpp -o server -lpthread
  ln -s server client_simple
  ln -s server client_select
