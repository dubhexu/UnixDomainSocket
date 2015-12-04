Single producer/multiple consumers AF_UNIX socket program
# Build
g++ UnixDomainSocket.cpp test.cpp -o server -lpthread<br>
ln -s server client_simple<br>
ln -s server client_select<br>
