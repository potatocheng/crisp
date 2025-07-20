#include "include/EchoServer.h"

int main() {
    EchoServer server{9852};
    server.run();
    
    return 0;
}