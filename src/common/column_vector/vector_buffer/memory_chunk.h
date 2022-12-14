//
// Created by JinHai on 2022/11/30.
//

#pragma once

#include "common/memory/allocator.h"
#include "common/types/internal_types.h"

namespace infinity {

struct MemoryChunk {
public:
    inline explicit
    MemoryChunk(u64 capacity) : capacity_(capacity), current_offset_(0), object_count_(0) {
        GlobalResourceUsage::IncrObjectCount();
        ptr_ = Allocator::allocate(capacity);
    }

    inline
    ~MemoryChunk() {
        Allocator::deallocate(ptr_);
        ptr_ = nullptr;
        capacity_ = 0;
        current_offset_ = 0;
        object_count_ = 0;
        GlobalResourceUsage::DecrObjectCount();
    }

    ptr_t ptr_{nullptr};
    u64 current_offset_{0};
    u64 capacity_{0};
    u64 object_count_{0};
};

struct StringChunkMgr {
    // Use to store string.
    static constexpr u64 CHUNK_SIZE = 4096;
public:
    inline explicit
    StringChunkMgr(u64 chunk_size = CHUNK_SIZE) : current_chunk_size_(chunk_size) {
        GlobalResourceUsage::IncrObjectCount();
    }

    inline
    ~StringChunkMgr() {
        GlobalResourceUsage::DecrObjectCount();
    }

    ptr_t
    Allocate(size_t nbytes);

    [[nodiscard]] String
    Stats() const;

public:
    [[nodiscard]] inline size_t
    chunks() const { return chunks_.size(); }

    [[nodiscard]] inline u64
    current_chunk_idx() const {
        return current_chunk_idx_;
    }

    [[nodiscard]] inline u64
    current_chunk_size() const {
        return current_chunk_size_;
    }
private:
    Vector<UniquePtr<MemoryChunk>> chunks_;
    u64 current_chunk_size_{CHUNK_SIZE};
    u64 current_chunk_idx_{std::numeric_limits<u64>::max()};
};

}