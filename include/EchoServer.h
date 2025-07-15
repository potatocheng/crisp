#pragma  once
#include <liburing.h>
#include <vector>

enum class EventType {
    ACCEPT,
    SEND,
    RECV,
};

struct Request {
    EventType type;
    std::vector<char> buffer;
};

class EchoServer {
public:
    EchoServer(int port);
    ~EchoServer();
    void run();

private:
    void setup_listening_socket();
    void handle_accept();
    void handle_send();
    void handle_recv();

private:
    io_uring ring_;
    int listening_fd_;
    int port_;
};