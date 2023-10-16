#pragma once

#include "compressed_pair.h"

#include <cstddef>  // std::nullptr_t

template <typename T>
struct Slug {
    template <typename F>
    Slug(Slug<F>&& elem){};  // NOLINT
    Slug(T&& elem){};
    Slug(const T& elem){};  // NOLINT
    Slug() = default;
    template <typename F>
    Slug(F&& elem){};

    void operator()(T* ptr) {
        delete ptr;
    };
};

template <typename T>
struct Slug<T[]> {  // NOLINT

    Slug(const T&& elem){};
    Slug(const T& elem){};
    Slug() = default;
    template <typename F>
    Slug(const F&& elem){};

    void operator()(T* ptr) {
        delete[] ptr;
    };
};

// Primary template
template <typename T, typename Deleter = Slug<T>>
class UniquePtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors
    explicit UniquePtr(T* ptr = nullptr) noexcept : ptr_(ptr, Deleter()){};
    UniquePtr(T* ptr, const Deleter& deleter) noexcept : ptr_(ptr, deleter){};
    UniquePtr(T* ptr, Deleter&& deleter) noexcept : ptr_(ptr, std::move(deleter)){};
    template <typename F, typename AnotherDeleter = Deleter>
    UniquePtr(UniquePtr<F, AnotherDeleter>&& other) noexcept
        : ptr_(other.Release(), std::move((other.GetDeleter()))){};
    UniquePtr(UniquePtr&& other) noexcept
        : ptr_(other.Release(), std::forward<Deleter>(other.GetDeleter())){};
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s
    template <typename F, typename AnotherDeleter = Deleter>
    UniquePtr& operator=(UniquePtr<F, AnotherDeleter>&& other) noexcept {
        Reset(other.Release());
        ptr_.GetSecond() = std::move(other.GetDeleter());
        return *this;
    }
    UniquePtr& operator=(UniquePtr&& other) noexcept {
        Reset(other.Release());
        ptr_.GetSecond() = std::forward<Deleter>(other.GetDeleter());
        return *this;
    }
    UniquePtr& operator=(std::nullptr_t) {
        Reset();
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~UniquePtr() {
        Reset();
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    T* Release() {
        T* p = Get();
        ptr_.GetFirst() = nullptr;
        return p;
    }
    void Reset(T* ptr = nullptr) {
        T* temporary = Get();
        ptr_.GetFirst() = ptr;
        if (temporary != nullptr) {
            GetDeleter()(temporary);
        }
    }
    void Swap(UniquePtr& other) {
        std::swap(ptr_.GetFirst(), other.ptr_.GetFirst());
        std::swap(ptr_.GetSecond(), other.ptr_.GetSecond());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return ptr_.GetFirst();
    }
    Deleter& GetDeleter() {
        return ptr_.GetSecond();
    };
    const Deleter& GetDeleter() const {
        return ptr_.GetSecond();
    }
    explicit operator bool() const {
        return Get() != nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Single-object dereference operators

    std::add_lvalue_reference<T> operator*() const {
        return *ptr_.GetFirst();
    }
    const T operator*() {
        return *ptr_.GetFirst();
    }
    T* operator->() const {
        return ptr_.GetFirst();
    }

private:
    CompressedPair<T*, Deleter> ptr_;
};

// Specialization for arrays
template <typename T, typename Deleter>
class UniquePtr<T[], Deleter> {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors
    explicit UniquePtr(T* ptr) noexcept : ptr_(ptr, Deleter()){};
    UniquePtr(T* ptr, const Deleter& deleter) noexcept : ptr_(ptr, deleter){};
    UniquePtr(T* ptr, Deleter&& deleter) noexcept : ptr_(ptr, std::move(deleter)){};

    UniquePtr(UniquePtr&& other) noexcept
        : ptr_(other.Release(), std::forward<Deleter>(other.GetDeleter())){};

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    UniquePtr& operator=(UniquePtr&& other) noexcept {
        Reset(other.Release());
        ptr_.GetSecond() = std::forward<Deleter>(other.GetDeleter());
        return *this;
    }
    UniquePtr& operator=(std::nullptr_t) {
        Reset();
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~UniquePtr() {
        Reset();
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    T* Release() {
        T* p = Get();
        ptr_.GetFirst() = nullptr;
        return p;
    }
    void Reset(T* ptr = nullptr) {
        T* temporary = Get();
        ptr_.GetFirst() = ptr;
        if (temporary != nullptr) {
            GetDeleter()(temporary);
        }
    }
    void Swap(UniquePtr& other) {
        std::swap(ptr_.GetFirst(), other.ptr_.GetFirst());
        std::swap(ptr_.GetSecond(), other.ptr_.GetSecond());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return ptr_.GetFirst();
    }
    Deleter& GetDeleter() {
        return ptr_.GetSecond();
    };
    const Deleter& GetDeleter() const {
        return ptr_.GetSecond();
    }
    explicit operator bool() const {
        return Get() != nullptr;
    }

    T& operator[](size_t index) {
        return ptr_.GetFirst()[index];
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Single-object dereference operators

    std::add_lvalue_reference<T> operator*() const {
        return *ptr_.GetFirst();
    }
    const T operator*() {
        return *ptr_.GetFirst();
    }
    T* operator->() const {
        return ptr_.GetFirst();
    }

private:
    CompressedPair<T*, Deleter> ptr_;
};
