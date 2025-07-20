#include <cstddef>
#include <new>
#include <vector>
#include <memory>
#include <list>

enum class EventType {
    ACCEPT,
    SEND,
    RECV,

    UNDEFINED,
};

struct Request {
    EventType type_{EventType::UNDEFINED};
    int client_fd_{0}; // accept event
    std::vector<char> buffer_;
    Request* next_{nullptr};
    struct Chunk* ower_chunk_{nullptr};

    Request() = default;
    Request(EventType type, int fd):type_(type), client_fd_(fd), buffer_(1024), next_(nullptr) {}
};

struct Chunk {
    static constexpr size_t kChunkSize = 256;

    Request* memory_ = nullptr;
    size_t free_count_ = 0;

    Chunk() : free_count_(kChunkSize) {
        memory_ = static_cast<Request*>(::operator new(sizeof(Request) * kChunkSize));

        for(int i = 0; i < kChunkSize; ++ i) {
            new (&memory_[i]) Request();
            memory_[i].ower_chunk_ = this;
        }
    }

    ~Chunk() {
        for(int i = 0; i < kChunkSize; ++ i) {
            memory_[i].~Request();
        }
        ::operator delete(memory_);
    }
};

class RequestPool {
public:
    explicit RequestPool(size_t initial_chunk = 1);
    ~RequestPool() = default;

    RequestPool(const RequestPool&) = delete;
    RequestPool& operator=(const RequestPool&) = delete;

    Request* acquire();
    void release(Request* req);

    void scavenge(); 

private:
    bool grow(); // 扩容逻辑
    std::list<std::unique_ptr<Chunk>> chunks_;
    Request* free_head_{nullptr};
    size_t total_free_count_{0};
};