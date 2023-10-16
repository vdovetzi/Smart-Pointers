#pragma once

#include "sw_fwd.h"  // Forward declaration
#include "shared.h"

// https://en.cppreference.com/w/cpp/memory/weak_ptr
template <typename T>
class WeakPtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    WeakPtr() : observed_(nullptr), block_(nullptr){};

    WeakPtr(const WeakPtr& other) : block_(other.block_), observed_(other.observed_) {
        if (block_ != nullptr) {
            block_->IncWeakCounter();
        }
    }
    WeakPtr(WeakPtr&& other) : observed_(other.observed_), block_(other.block_) {
        other.block_ = nullptr;
        other.observed_ = nullptr;
    }

    // Demote `SharedPtr`
    // #2 from https://en.cppreference.com/w/cpp/memory/weak_ptr/weak_ptr
    WeakPtr(const SharedPtr<T>& other) : block_(other.block_), observed_(other.observed_) {
        if (block_ != nullptr) {
            block_->IncWeakCounter();
        }
    }

    template <class S>
    WeakPtr(const WeakPtr<S>& other) : block_(other.block_), observed_(other.observed_) {
        if (block_ != nullptr) {
            block_->IncWeakCounter();
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    WeakPtr& operator=(const WeakPtr& other) {
        Reset();
        block_ = other.block_;
        observed_ = other.observed_;
        if (block_ != nullptr) {
            block_->IncWeakCounter();
        }
        return *this;
    };
    WeakPtr& operator=(WeakPtr&& other) {
        Reset();
        block_ = other.block_;
        observed_ = other.observed_;
        other.block_ = nullptr;
        other.observed_ = nullptr;
        return *this;
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~WeakPtr() {
        Reset();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        if (block_ != nullptr) {
            block_->DecWeakCounter();
            block_ = nullptr;
            observed_ = nullptr;
        }
    }
    void Swap(WeakPtr& other) {
        std::swap(block_, other.block_);
        std::swap(observed_, other.observed_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    size_t UseCount() const {
        if (block_ == nullptr) {
            return 0;
        }
        return block_->GetStrongCounter();
    }
    bool Expired() const {
        if (block_ == nullptr) {
            return true;
        }
        return block_->GetStrongCounter() == 0;
    }
    SharedPtr<T> Lock() const {
        SharedPtr<T> result;
        if (!Expired()) {
            result.block_ = block_;
            result.observed_ = observed_;
            block_->IncStrongCounter();
        }
        return result;
    }
    BaseBlock* block_ = nullptr;
    T* observed_ = nullptr;
};