#pragma once

#include "sw_fwd.h"  // Forward declaration
#include <utility>
#include <cstddef>  // std::nullptr_t

struct BaseBlock {
    virtual void IncCounter() = 0;
    virtual void DecCounter() = 0;
    virtual ~BaseBlock(){};
    virtual size_t GetCounter() = 0;
};
template <typename T>
struct ControlBlockPointer : BaseBlock {
    ControlBlockPointer() = default;
    ControlBlockPointer(T* object) : object_(object), counter_(1){};
    void DecCounter() override {
        if (this != nullptr) {
            if (counter_ != 0) {
                --counter_;
            }
            if (counter_ == 0) {
                if (object_ != nullptr) {
                    delete object_;
                }
                delete this;
            }
        }
    }

    void IncCounter() override {
        ++counter_;
    }

    size_t GetCounter() override {
        return counter_;
    }
    ~ControlBlockPointer() {
        object_ = nullptr;
        counter_ = 0;
    }
    T* object_ = nullptr;
    size_t counter_ = 0;
};
template <typename T>
struct ControlBlockObject : BaseBlock {
    ControlBlockObject(T object) : object_(object), counter_(1){};
    template <typename... Args>
    ControlBlockObject(Args&&... args) : object_(std::forward<Args>(args)...), counter_(1){};
    void DecCounter() override {
        if (this != nullptr) {
            if (counter_ != 0) {
                --counter_;
            }
            if (counter_ == 0) {
                delete this;
            }
        }
    }

    void IncCounter() override {
        ++counter_;
    }

    size_t GetCounter() override {
        return counter_;
    }
    ~ControlBlockObject() {
        counter_ = 0;
    }
    T object_;
    size_t counter_ = 0;
};

// https://en.cppreference.com/w/cpp/memory/shared_ptr
template <typename T>
class SharedPtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    SharedPtr() : block_(nullptr), observed_(nullptr){};
    SharedPtr(std::nullptr_t) : block_(nullptr), observed_(nullptr){};
    explicit SharedPtr(T* ptr) : observed_(ptr) {
        block_ = new ControlBlockPointer<T>(ptr);
    };
    template <typename S>
    SharedPtr(S* ptr) : observed_(ptr), block_(new ControlBlockPointer<S>(ptr)){};
    SharedPtr(const SharedPtr& other) {
        if (other.block_ == nullptr) {
            block_ = nullptr;
            observed_ = nullptr;
        }
        if (block_ != nullptr) {
            block_->DecCounter();
        }
        if (other.observed_ != observed_) {
            observed_ = other.observed_;
            block_ = other.block_;
            block_->IncCounter();
        }
    }
    SharedPtr(SharedPtr&& other) {
        if (other.block_ == nullptr) {
            block_ = nullptr;
            observed_ = nullptr;
        }
        if (block_ != nullptr) {
            block_->DecCounter();
        }
        if (other.block_ != nullptr) {
            block_ = other.block_;
            observed_ = other.observed_;
            block_->IncCounter();
            other.Reset();
        }
    };

    template <typename S>
    SharedPtr(SharedPtr<S> ptr) : observed_(ptr.observed_), block_(ptr.block_) {
        block_->IncCounter();
    };

    // Aliasing constructor
    // #8 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other, T* ptr) : observed_(ptr), block_(other.block_) {
        block_->IncCounter();
    };

    // Promote `WeakPtr`
    // #11 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    explicit SharedPtr(const WeakPtr<T>& other);

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    SharedPtr& operator=(const SharedPtr& other) {
        if (other.block_ == nullptr) {
            block_ = nullptr;
            observed_ = nullptr;
            return *this;
        }
        if (block_ != nullptr) {
            block_->DecCounter();
        }
        if (other.observed_ != observed_) {
            observed_ = other.observed_;
            block_ = other.block_;
            block_->IncCounter();
        }
        return *this;
    }
    SharedPtr& operator=(SharedPtr&& other) {
        if (other.block_ == nullptr) {
            block_ = nullptr;
            observed_ = nullptr;
        }
        if (block_ != nullptr) {
            block_->DecCounter();
        }
        if (other.block_ != nullptr) {
            block_ = other.block_;
            observed_ = other.observed_;
            block_->IncCounter();
            other.Reset();
        }
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~SharedPtr() {
        if (block_ != nullptr) {
            block_->DecCounter();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        if (block_ != nullptr) {
            block_->DecCounter();
            block_ = nullptr;
            observed_ = nullptr;
        }
    }

    template <typename S>
    void Reset(S* ptr) {
        if (block_ != nullptr) {
            block_->DecCounter();
        }
        block_ = new ControlBlockPointer<S>(ptr);
        observed_ = ptr;
    }
    void Reset(T* ptr) {
        if (block_ != nullptr) {
            block_->DecCounter();
        }
        block_ = new ControlBlockPointer<T>(ptr);
        observed_ = ptr;
    }
    void Swap(SharedPtr& other) {
        std::swap(block_, other.block_);
        std::swap(observed_, other.observed_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        if (block_ == nullptr) {
            return nullptr;
        }
        return observed_;
    }
    T& operator*() const {
        return *observed_;
    }
    T* operator->() const {
        return observed_;
    }
    size_t UseCount() const {
        if (block_ == nullptr) {
            return 0;
        }
        return block_->GetCounter();
    }
    explicit operator bool() const {
        return Get() != nullptr;
    }
    BaseBlock* block_ = nullptr;
    T* observed_ = nullptr;
};

template <typename T, typename U>
inline bool operator==(const SharedPtr<T>& left, const SharedPtr<U>& right);

// Allocate memory only once
template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args) {
    SharedPtr<T> result;
    ControlBlockObject<T>* block_object = new ControlBlockObject<T>(std::forward<Args>(args)...);
    result.observed_ = reinterpret_cast<T*>(&block_object->object_);
    result.block_ = block_object;
    return result;
}

// Look for usage examples in tests
template <typename T>
class EnableSharedFromThis {
public:
    SharedPtr<T> SharedFromThis();
    SharedPtr<const T> SharedFromThis() const;

    WeakPtr<T> WeakFromThis() noexcept;
    WeakPtr<const T> WeakFromThis() const noexcept;
};
