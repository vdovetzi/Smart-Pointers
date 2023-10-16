#pragma once

#include "sw_fwd.h"  // Forward declaration
#include <utility>
#include <cstddef>  // std::nullptr_t

class ESFTBase {};

struct BaseBlock {
    virtual void IncStrongCounter() = 0;
    virtual void IncWeakCounter() = 0;
    virtual void DecStrongCounter() = 0;
    virtual void DecWeakCounter() = 0;
    virtual ~BaseBlock(){};
    virtual size_t GetStrongCounter() = 0;
    [[maybe_unused]] virtual size_t& GetWeakCounter() = 0;
};
template <typename T>
struct ControlBlockPointer : BaseBlock {
    ControlBlockPointer() = default;
    ControlBlockPointer(T* object) : object_(object), strong_counter_(1){};
    void DecStrongCounter() override {
        if (this != nullptr) {
            if (strong_counter_ > 1) {
                --strong_counter_;
            } else if ((strong_counter_ == 1 && weak_counter_ == 0) ||
                       (strong_counter_ == 0 && weak_counter_ == 0)) {
                delete object_;
                delete this;
            } else if ((strong_counter_ == 0 && weak_counter_ != 0) ||
                       (weak_counter_ != 0 && strong_counter_ == 1)) {
                --strong_counter_;
                delete object_;
                if (weak_counter_ == 0) {
                    delete this;
                }
            }
        }
    }

    void DecWeakCounter() override {
        if (this != nullptr) {
            if (weak_counter_ > 1) {
                --weak_counter_;
            } else if ((weak_counter_ == 1 && strong_counter_ == 0) ||
                       (weak_counter_ == 0 && strong_counter_ == 0)) {
                //                if (!std::is_convertible_v<T*, ESFTBase*>) {
                delete this;
                //                }
            } else if ((weak_counter_ == 1 && strong_counter_ != 0) ||
                       (weak_counter_ == 0 && strong_counter_ != 0)) {
                --weak_counter_;
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
    size_t& GetWeakCounter() override {
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
            if (strong_counter_ > 1) {
                --strong_counter_;
            } else if ((strong_counter_ == 1 && weak_counter_ == 0) ||
                       (strong_counter_ == 0 && weak_counter_ == 0)) {
                reinterpret_cast<T*>(&buffer_)->~T();
                delete this;
            } else if ((strong_counter_ == 0 && weak_counter_ != 0) ||
                       (weak_counter_ != 0 && strong_counter_ == 1)) {
                --strong_counter_;
                reinterpret_cast<T*>(&buffer_)->~T();
                if (weak_counter_ == 0) {
                    delete this;
                }
            }
        }
    }

    void DecWeakCounter() override {
        if (this != nullptr) {
            if (weak_counter_ > 1) {
                --weak_counter_;
            }
            if ((weak_counter_ == 1 && strong_counter_ == 0) ||
                (weak_counter_ == 0 && strong_counter_ == 0)) {
                //                if (!std::is_convertible_v<T*, ESFTBase*>) {
                delete this;
                //                }
            } else if ((weak_counter_ == 1 && strong_counter_ != 0) ||
                       (weak_counter_ == 0 && strong_counter_ != 0)) {
                --weak_counter_;
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
    size_t& GetWeakCounter() override {
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
    explicit SharedPtr(T* ptr) {
        Reset();
        observed_ = ptr;
        block_ = new ControlBlockPointer<T>(ptr);
        if constexpr (std::is_convertible_v<T*, ESFTBase*>) {
            ptr->weak_this.block_ = block_;
            ptr->weak_this.observed_ = ptr;
            ptr->weak_this.block_->IncWeakCounter();
        }
    };
    template <typename S = T>
    SharedPtr(S* ptr) {
        if (!std::is_convertible_v<T*, ESFTBase*>) {
            Reset();
        }
        observed_ = ptr;
        block_ = new ControlBlockPointer<S>(ptr);
        if constexpr (std::is_convertible_v<T*, ESFTBase*>) {
            ptr->weak_this.block_ = block_;
            ptr->weak_this.observed_ = ptr;
            ptr->weak_this.block_->IncWeakCounter();
        }
    };
    SharedPtr(const SharedPtr& other) {
        Reset();
        if (other.block_ == nullptr) {
            block_ = nullptr;
            observed_ = nullptr;
        }
        if (other.observed_ != observed_) {
            observed_ = other.observed_;
            block_ = other.block_;
            block_->IncStrongCounter();
        }
    }
    SharedPtr(SharedPtr&& other) {
        Reset();
        if (other.block_ == nullptr) {
            block_ = nullptr;
            observed_ = nullptr;
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
            Reset();
            block_ = nullptr;
            observed_ = nullptr;
            return *this;
        }
        if (other.observed_ != observed_) {
            Reset();
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
        Reset();
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
        Reset();
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
        Reset();
        block_ = new ControlBlockPointer<S>(ptr);
        observed_ = ptr;
    }
    void Reset(T* ptr) {
        Reset();
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
inline bool operator==(const SharedPtr<T>& left, const SharedPtr<U>& right) {
    return left.block_ == right.block_ && left.observed_ == right.observed_;
}

// Allocate memory only once
template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args) {
    SharedPtr<T> result;
    ControlBlockObject<T>* block_object = new ControlBlockObject<T>(std::forward<Args>(args)...);
    result.observed_ = reinterpret_cast<T*>(&block_object->buffer_);
    result.block_ = block_object;
    if constexpr (std::is_convertible_v<T*, ESFTBase*>) {
        result->weak_this.block_ = block_object;
        result->weak_this.observed_ = result.observed_;
        result->weak_this.block_->IncWeakCounter();
    }
    return result;
}

// Look for usage examples in tests
template <typename T>
class EnableSharedFromThis : public ESFTBase {
public:
    SharedPtr<T> SharedFromThis() {
        return weak_this.Lock();
    }
    SharedPtr<const T> SharedFromThis() const {
        return weak_this.Lock();
    }

    WeakPtr<T> WeakFromThis() noexcept {
        return weak_this;
    }
    WeakPtr<const T> WeakFromThis() const noexcept {
        return weak_this;
    }
    ~EnableSharedFromThis() {
        if (weak_this.block_ != nullptr) {
            --weak_this.block_->GetWeakCounter();
        }
        weak_this.block_ = nullptr;
        weak_this.observed_ = nullptr;
    }
    WeakPtr<T> weak_this;
};
