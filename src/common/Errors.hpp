#pragma once

#include <stdexcept>

namespace VkTest1::Common
{

class IoError : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

class WindowError : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

class RendererError : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

} // namespace VkTest1::Common
