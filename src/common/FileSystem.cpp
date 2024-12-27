#include "FileSystem.hpp"

#include "Errors.hpp"

#include <fstream>

namespace VkTest1::Common
{

std::vector<std::byte> FileSystem::readFile(const std::filesystem::path& path)
{
    std::ifstream fileStream{ path, std::ios::binary };
    if (!fileStream.is_open())
    {
        throw IoError{ "Cannot open file." };
    }

    fileStream.seekg(0, std::ios::end);
    const auto fileSize{ fileStream.tellg() };
    fileStream.seekg(0);

    std::vector<std::byte> contents(fileSize);
    fileStream.read(reinterpret_cast<char*>(contents.data()), fileSize);
    if (!fileStream.good())
    {
        throw IoError{ "Cannot read file." };
    }

    return contents;
}

} // namespace VkTest1::Common
