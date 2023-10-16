#pragma once

#include "sw_fwd.h"  // Forward declaration
#include <utility>
#include <cstddef>  // std::nullptr_t

struct BaseBlock {
    virtual void IncStrongCounter() = 0;
    virtual void IncWeakCounter() = 0;
    virtual void DecStrongCounter() = 0;
    virtual void DecWeakCounter() = 0;
    virtual ~BaseBlock(){};
    virtual size_t GetStrongCounter() = 0;
    [[maybe_unused]] virtual size_t GetWeakCounter() = 0;
};
template <typename T>
struct ControlBlockPointer : BaseBlock {
    ControlBlockPointer() = default;
    ControlBlockPointer(T* object) : object_(object), strong_counter_(1){};
    void DecStrongCounter() override {
        if (this != nullptr) {
            if (strong_counter_ != 0) {
                --strong_counter_;
            }
            if (strong_counter_ == 0) {
                if (object_ != nullptr) {
                    delete object_;
                }
                if (weak_counter_ == 0) {
                    delete this;
                }
            }
        }
    }

    void DecWeakCounter() override {
        if (this != nullptr) {
            if (weak_counter_ != 0) {
                --weak_counter_;
            }
            if (weak_counter_ == 0) {
                if (strong_counter_ == 0) {
                    delete this;
                }
            }
        }
    }

    void IncStrongCounter() override {
        ++strong_counter_;
    }

    void IncWeakCounter() override {
        ++weak_counter_;
    }

    size_t GetStrongCounter() override {
        return strong_counter_;
    }
    size_t GetWeakCounter() override {
        return weak_counter_;
    }
    ~ControlBlockPointer() {
        object_ = nullptr;
        strong_counter_ = 0;
        weak_counter_ = 0;
    }
    T* object_ = nullptr;
    size_t strong_counter_ = 0;
    size_t weak_counter_ = 0;
};
template <typename T>
struct ControlBlockObject : BaseBlock {
    ControlBlockObject(T object) : strong_counter_(1) {
        new (&buffer_) T(object);
    };
    template <typename... Args>
    ControlBlockObject(Args&&... args) : strong_counter_(1) {
        new (&buffer_) T(std::forward<Args>(args)...);
    };
    void DecStrongCounter() override {
        if (this != nullptr) {
            if (this != nullptr) {
                if (strong_counter_ != 0) {
                    --strong_counter_;
                }
                if (strong_counter_ == 0) {
                    if (&buffer_ != nullptr) {
                        reinterpret_cast<T*>(&buffer_)->~T();
                    }
                    if (weak_counter_ == 0) {
                        delete this;
                    }
                }
            }
        }
    }

    void DecWeakCounter() override {
        if (this != nullptr) {
            if (weak_counter_ != 0) {
                --weak_counter_;
            }
            if (weak_counter_ == 0) {
                if (strong_counter_ == 0) {
                    delete this;
                }
            }
        }
    }

    void IncStrongCounter() override {
        ++strong_counter_;
    }
    void IncWeakCounter() override {
        ++weak_counter_;
    }

    size_t GetStrongCounter() override {
        return strong_counter_;
    }
    size_t GetWeakCounter() override {
        return weak_counter_;
    }
    ~ControlBlockObject() {
        strong_counter_ = 0;
        weak_counter_ = 0;
    }
    std::aligned_storage_t<sizeof(T), alignof(T)> buffer_;
    size_t strong_counter_ = 0;
    size_t weak_counter_ = 0;
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
            block_->DecStrongCounter();
        }
        if (other.observed_ != observed_) {
            observed_ = other.observed_;
            block_ = other.block_;
            block_->IncStrongCounter();
        }
    }
    SharedPtr(SharedPtr&& other) {
        if (other.block_ == nullptr) {
            block_ = nullptr;
            observed_ = nullptr;
        }
        if (block_ != nullptr) {
            block_->DecStrongCounter();
        }
        if (other.block_ != nullptr) {
            block_ = other.block_;
            observed_ = other.observed_;
            block_->IncStrongCounter();
            other.Reset();
        }
    };

    template <typename S>
    SharedPtr(SharedPtr<S> ptr) : observed_(ptr.observed_), block_(ptr.block_) {
        block_->IncStrongCounter();
    };

    // Aliasing constructor
    // #8 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other, T* ptr) : observed_(ptr), block_(other.block_) {
        block_->IncStrongCounter();
    };

    // Promote `WeakPtr`
    // #11 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    explicit SharedPtr(const WeakPtr<T>& other) {
        if (other.Expired()) {
            throw BadWeakPtr{};
        }
        block_ = other.block_;
        observed_ = other.observed_;
        if (block_ != nullptr) {
            block_->IncStrongCounter();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    SharedPtr& operator=(const SharedPtr& other) {
        if (other.block_ == nullptr) {
            block_ = nullptr;
            observed_ = nullptr;
            return *this;
        }
        if (block_ != nullptr) {
            block_->DecStrongCounter();
        }
        if (other.observed_ != observed_) {
            observed_ = other.observed_;
            block_ = other.block_;
            block_->IncStrongCounter();
        }
        return *this;
    }
    SharedPtr& operator=(SharedPtr&& other) {
        if (other.block_ == nullptr) {
            block_ = nullptr;
            observed_ = nullptr;
        }
        if (block_ != nullptr) {
            block_->DecStrongCounter();
        }
        if (other.block_ != nullptr) {
            block_ = other.block_;
            observed_ = other.observed_;
            block_->IncStrongCounter();
            other.Reset();
        }
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~SharedPtr() {
        if (block_ != nullptr) {
            block_->DecStrongCounter();
            block_ = nullptr;
            observed_ = nullptr;
        }
        //        observed_ = nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        if (block_ != nullptr) {
            block_->DecStrongCounter();
            block_ = nullptr;
            observed_ = nullptr;
        }
    }

    template <typename S>
    void Reset(S* ptr) {
        if (block_ != nullptr) {
            block_->DecStrongCounter();
        }
        block_ = new ControlBlockPointer<S>(ptr);
        observed_ = ptr;
    }
    void Reset(T* ptr) {
        if (block_ != nullptr) {
            block_->DecStrongCounter();
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
        return block_->GetStrongCounter();
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
    result.observed_ = reinterpret_cast<T*>(&block_object->buffer_);
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
