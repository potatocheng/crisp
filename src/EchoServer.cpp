#include "../include/EchoServer.h"
#include <asm-generic/socket.h>
#include <liburing.h>
#include <stdexcept>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <iostream>

EchoServer::EchoServer(int port): port_(port) {
    io_uring_queue_init(1024, &ring_, 0);
    setup_listening_socket();
    add_accept_request();
}

EchoServer::~EchoServer() {
    close(listening_fd_);
    io_uring_queue_exit(&ring_);
}

void EchoServer::run() {
    while(true) {
        io_uring_submit_and_wait(&ring_, 1);

        io_uring_cqe* cqe;
        unsigned head;
        unsigned count = 0;

        io_uring_for_each_cqe(&ring_, head, cqe) {
            count++;
            handle_cqe(cqe);
        }

        io_uring_cq_advance(&ring_, count);
    }
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

void EchoServer::add_accept_request() {
    io_uring_sqe* sqe = io_uring_get_sqe(&ring_);
    if (!sqe) {
        throw std::runtime_error("Failed to get SQE");
    }
    
    io_uring_prep_accept(sqe, listening_fd_, nullptr, nullptr, 0);
    auto* req = new Request(EventType::ACCEPT, 0);
    io_uring_sqe_set_data(sqe, req);
    io_uring_submit(&ring_);
}

void EchoServer::add_send_request(Request* req) {
    io_uring_sqe* sqe = io_uring_get_sqe(&ring_);
    if (!sqe) {
        throw std::runtime_error("Failed to get SQE");
    }

    req->type_ = EventType::SEND;
    io_uring_prep_send(sqe, req->client_fd_, req->buffer_.data(), req->buffer_.size(), 0);
    io_uring_sqe_set_data(sqe, req);
}

void EchoServer::add_recv_request(int client_fd) {
    io_uring_sqe* sqe = io_uring_get_sqe(&ring_);
    if(!sqe) {
        throw std::runtime_error("Failed to get SQE");
    }

    Request* req = new Request(EventType::RECV, client_fd);
    io_uring_prep_recv(sqe, client_fd, req->buffer_.data(), req->buffer_.size(), 0);
    io_uring_sqe_set_data(sqe, req);
}

void EchoServer::handle_cqe(io_uring_cqe* cqe) {
    Request* req = reinterpret_cast<Request*>(cqe->user_data);

    switch (req->type_) {
        case EventType::ACCEPT:
        {
            int client_fd = cqe->res;
            std::cout << "accept client fd: " << client_fd << std::endl;
            add_accept_request();
            
            add_recv_request(client_fd);
            delete req;
            break;
        }
        case EventType::RECV:
        {
            if(req->buffer_.size() > 0) {
                std::cout << "recv data:" << req->buffer_.data() << std::endl;
                add_send_request(req);
            } else {
                close(req->client_fd_);
                delete req;
            }
            break;
        }
        case EventType::SEND:
        {
            std::cout << "send data:" << req->buffer_.data() << std::endl;
            add_recv_request(req->client_fd_);
            
            delete req;
            break;
        }
    }
}