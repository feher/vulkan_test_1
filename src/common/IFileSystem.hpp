#pragma once

#include <filesystem>
#include <vector>

namespace VkTest1::Common
{

class IFileSystem
{
public:
    virtual ~IFileSystem() = default;

    virtual std::vector<std::byte> readFile(const std::filesystem::path& path) = 0;
};

} // namespace VkTest1::Common
