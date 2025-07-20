#pragma  once
#include <liburing.h>
#include "RequestPool.h"

class EchoServer {
public:
    EchoServer(int port);
    ~EchoServer();
    void run();

private:
    void setup_listening_socket();
    void add_accept_request();
    void add_send_request(Request* req);
    void add_recv_request(int client_fd);
    void handle_cqe(io_uring_cqe* cqe);

private:
    io_uring ring_;
    int listening_fd_;
    int port_;
};