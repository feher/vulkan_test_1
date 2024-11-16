#pragma once

#include <cassert>
#include <type_traits>

namespace VkTest1::Common
{

using Uint = unsigned int;

template<typename T>
requires(std::is_pointer_v<T>)
class NotNull
{
public:
    NotNull() = default;

    NotNull(T ptr) :
        m_ptr{ ptr }
    {
        assert(m_ptr != nullptr);
    }

    NotNull(const NotNull& other) noexcept :
        m_ptr{ other.m_ptr }
    {
        assert(m_ptr != nullptr);
    }

    NotNull& operator=(const NotNull& other)
    {
        m_ptr = other.m_ptr;
        assert(m_ptr != nullptr);
        return *this;
    }

    NotNull& operator=(T ptr)
    {
        m_ptr = ptr;
        assert(m_ptr != nullptr);
        return *this;
    }

    decltype(auto) operator*() const
    {
        assert(m_ptr != nullptr);
        auto& target = *m_ptr;
        return target;
    }

    T operator->() const
    {
        assert(m_ptr != nullptr);
        return m_ptr;
    }

    operator T() const
    {
        assert(m_ptr != nullptr);
        return m_ptr;
    }

private:
    T m_ptr;
};

namespace Flags
{

template<typename TFlags, typename TFlag>
constexpr bool isSet(const TFlags flags, const TFlag flag)
{
    return (flags & flag) != 0;
}

} // namespace Flags

template<typename T>
class Bitmask
{
public:
private:
};

} // namespace VkTest1::Common