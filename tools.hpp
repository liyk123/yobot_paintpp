#pragma once
#include <algorithm>

template <typename T, auto Destructor>
struct GenericDeleter
{
    void operator()(T* ptr) const noexcept
    {
        if (ptr) Destructor(ptr);
    }
};

template<size_t N>
struct FixedString
{
    char data[N]{};

    constexpr FixedString(const char(&str)[N])
    {
        std::copy_n(str, N, data);
    }

    constexpr FixedString() = default;

    constexpr size_t size() const { return N - 1; }

    template<size_t M>
    constexpr auto operator+(FixedString<M> other) const
    {
        FixedString<N + M - 1> result;
        std::copy_n(data, N - 1, result.data);
        std::copy_n(other.data, M, result.data + N - 1);
        return result;
    }
};