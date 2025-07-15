#include "../include/EchoServer.h"
#include <asm-generic/socket.h>
#include <liburing.h>
#include <stdexcept>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

EchoServer::EchoServer(int port): port_(port) {
    io_uring_queue_init(1024, &ring_, 0);
}

EchoServer::~EchoServer() {
    close(listening_fd_);
    io_uring_queue_exit(&ring_);
}

void EchoServer::run() {

}

void EchoServer::setup_listening_socket() {
    listening_fd_ = socket(AF_INET6, SOCK_STREAM, 0);
    if (listening_fd_ < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    // 启用 dual-stack 支持
    int no = 0;
    setsockopt(listening_fd_, IPPROTO_IPV6, IPV6_V6ONLY, &no, sizeof(no));

    // 允许地址重用
    int yes = 1;
    setsockopt(listening_fd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    sockaddr_in6 addr{};
    addr.sin6_family = AF_INET6;
    addr.sin6_addr = in6addr_any;
    addr.sin6_port = htons(port_);

    if (bind(listening_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        throw std::runtime_error("Failed to bind socket");
    }

    if (listen(listening_fd_, SOMAXCONN) < 0) {
        throw std::runtime_error("Failed to listen on socket");
    }
}

void EchoServer::handle_accept() {
    sockaddr_in6 client_addr{};
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(listening_fd_, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
    if (client_fd < 0) {
        throw std::runtime_error("Failed to accept connection");
    }

    io_uring_sqe* sqe = io_uring_get_sqe(&ring_);
    if (!sqe) {
        throw std::runtime_error("Failed to get SQE");
    }

    io_uring_prep_accept(sqe, listening_fd_, reinterpret_cast<sockaddr*>(&client_addr), &client_len, 0);
}

void EchoServer::handle_send() {

}

void EchoServer::handle_recv() {

}