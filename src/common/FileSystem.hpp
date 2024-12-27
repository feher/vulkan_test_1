#pragma once

#include <common/IFileSystem.hpp>

namespace VkTest1::Common
{

class FileSystem : public IFileSystem
{
public:
    std::vector<std::byte> readFile(const std::filesystem::path& path) override;
};

} // namespace VkTest1::Common
