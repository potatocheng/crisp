#include "../include/RequestPool.h"
#include <csignal>
#include <cstddef>
#include <list>
#include <memory>
#include <new>

RequestPool::RequestPool(size_t initial_chunk) {
    for(size_t i = 0; i < initial_chunk; ++ i) {
        grow();
    }
}

bool RequestPool::grow() {
    try {
        std::unique_ptr<Chunk> new_chunk{std::make_unique<Chunk>()};
        if (nullptr == new_chunk) {
            return false;
        }
        Request* chunk_start = new_chunk->memory_;

        for(int i = 0; i < Chunk::kChunkSize - 1; ++ i) {
            chunk_start[i].next_ = &chunk_start[i + 1];
        }

        chunk_start[Chunk::kChunkSize - 1].next_ = free_head_;
        free_head_ = &(chunk_start[0]);

        total_free_count_ += Chunk::kChunkSize;
        chunks_.push_back(new_chunk);
        return true;
    } catch (const std::bad_alloc) {
        return false;
    }
}

Request* RequestPool::acquire() {
   if(nullptr == free_head_) {
        if(!grow()) {
            return nullptr;
        }
   }
}

void RequestPool::release(Request* req) {
    if(nullptr == req) {
        return;
    }

    req->next_ = free_head_;
    free_head_ = req;

    ++ total_free_count_;
    req->ower_chunk_->free_count_++;
}

void RequestPool::scavenge() {
    if (chunks_.size() <= 1) {
        return;
    }

    std::list<Chunk*> fully_free_chunks_;
    for(const auto& chunk_ptr : chunks_) {
        fully_free_chunks_.push_back(chunk_ptr.get());
    }

    if(fully_free_chunks_.empty() || fully_free_chunks_.size() == chunks_.size()) {
        return;
    }

    Request** current = &free_head_;
    while(*current) {
        bool should_unlink = false;
        for(Chunk* chunk_to_delete : fully_free_chunks_) {
            if((*current)->ower_chunk_ == chunk_to_delete) {
                should_unlink = true;
            }
        }
        if(should_unlink) {
            *current = (*current)->next_;
        } else {
            current = &((*current)->next_);
        }
    }

    chunks_.remove_if([&](const std::unique_ptr<Chunk>& chunk_ptr)-> bool{
        for(Chunk* chunk_to_delete : fully_free_chunks_) {
            if(chunk_ptr.get() == chunk_to_delete) {
                total_free_count_ -= Chunk::kChunkSize;
                return true;
            }
        }
        return false;
    });
}