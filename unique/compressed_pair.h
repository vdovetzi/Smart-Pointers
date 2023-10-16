#pragma once

#include <type_traits>
#include <utility>
#include <cstdio>

template <typename T1, typename T2, bool = std::is_empty<T1>::value && !std::is_final_v<T1>,
          bool = std::is_empty<T2>::value && !std::is_final_v<T2>>
class CompressedPair;

// число и непустая строка
template <typename F, typename S>
class CompressedPair<F, S, false, false> {
public:
    CompressedPair() : first_(F()), second_(S()){};

    CompressedPair(const F& first, const S& second)
        : first_(std::move(first)), second_(std::move(second)){};

    CompressedPair(F&& first, S&& second) : first_(first), second_(std::move(second)){};

    CompressedPair(F&& first, const S& second) : first_(std::move(first)), second_(second){};

    CompressedPair(const F& first, S&& second) : first_(first), second_(std::move(second)){};

    F& GetFirst() {
        return first_;
    }

    const F& GetFirst() const {
        return first_;
    }

    const S& GetSecond() const {
        return second_;
    };

    S& GetSecond() {
        return second_;
    };

    S& operator[](size_t ind) {
        return second_[ind];
    };

private:
    F first_;
    S second_;
};

// число и пустая строка
template <typename F, typename S>
class CompressedPair<F, S, false, true> : private S {
public:
    CompressedPair(const F& first) : first_(first){};

    CompressedPair() : first_(F()){};

    CompressedPair(const F& first, const S& second) : S(second), first_(first){};

    CompressedPair(const F&& first) : first_(std::move(first)){};

    F& GetFirst() {
        return first_;
    }

    const F& GetFirst() const {
        return first_;
    }

    const S& GetSecond() const {
        return static_cast<const S&>(*this);
    };

    S& GetSecond() {
        return static_cast<S&>(*this);
    };

private:
    F first_;
};
// ничего и строка
template <typename F, typename S>
class CompressedPair<F, S, true, false> : private F {
public:
    CompressedPair(const F& first, const S& second) : F(first), second_(second) {
    }

    CompressedPair() = default;

    CompressedPair(const F&& first, const S&& second) : F(first), second_(second){};

    const S& GetSecond() const {
        return second_;
    };

    S& GetSecond() {
        return second_;
    };

    const F& GetFirst() const {
        return static_cast<const F&>(*this);
    };

    F& GetFirst() {
        return static_cast<F&>(*this);
    };

    S& operator[](size_t ind) {
        return second_[ind];
    };

private:
    S second_;
};

// вообще ничего
template <typename F, typename S>
class CompressedPair<F, S, true, true> : private S, private F {
public:
    CompressedPair(const F& first, const S& second) : F(first), S(std::move(second)){};

    CompressedPair(const F&& first, const S&& second) : F(std::move(first)), S(std::move(second)){};

    CompressedPair(const F& first, S&& second) : S(std::move(second)), F(first){};

    CompressedPair(const F&& first, const S& second) : F(std::move(first)), S(std::move(second)){};

    const F& GetFirst() const {
        return static_cast<const F&>(*this);
    };

    F& GetFirst() {
        return static_cast<F&>(*this);
    };

    const S& GetSecond() const {
        return static_cast<const S&>(*this);
    };

    S& GetSecond() {
        return static_cast<S&>(*this);
    };
};

template <typename T>
class CompressedPair<T, T, true, true> {
public:
    CompressedPair(const T& first, const T& second) : first_(first), second_(second){};

    const T& GetFirst() const {
        return first_;
    };

    T& GetFirst() {
        return first_;
    };

    const T& GetSecond() const {
        return second_;
    };

    T& GetSecond() {
        return second_;
    };

private:
    T first_;
    T second_;
};
