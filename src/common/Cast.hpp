#pragma once

namespace VkTest1::Common
{

template<typename TTo, typename TFrom>
TTo NarrowCast(TFrom from)
{
    return static_cast<TTo>(from);
}

} // namespace VkTest1::Common
