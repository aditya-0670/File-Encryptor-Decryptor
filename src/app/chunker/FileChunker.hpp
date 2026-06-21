#ifndef VAULT_CHUNKER_FILE_CHUNKER_HPP
#define VAULT_CHUNKER_FILE_CHUNKER_HPP

#include "../manifest/Manifest.hpp"
#include "../core/Action.hpp"

#include <cstdint>
#include <filesystem>
#include <string>

namespace vault::chunking {

class FileChunker {
public:
    static constexpr std::uint64_t DefaultChunkSize = 1024 * 1024;

    explicit FileChunker(std::uint64_t chunkSize = DefaultChunkSize);

    bool buildManifestForDirectory(
        const std::filesystem::path& rootPath,
        Action action,
        manifest::VaultManifest& manifest,
        std::string& error
    ) const;

    bool buildManifestForFile(
        const std::filesystem::path& filePath,
        Action action,
        manifest::VaultManifest& manifest,
        std::string& error
    ) const;

    std::uint64_t chunkSize() const;

private:
    bool appendFile(
        const std::filesystem::path& rootPath,
        const std::filesystem::path& filePath,
        std::size_t fileIndex,
        Action action,
        manifest::VaultManifest& manifest,
        std::string& error
    ) const;

    bool appendPlaintextFile(
        const std::filesystem::path& rootPath,
        const std::filesystem::path& filePath,
        std::size_t fileIndex,
        manifest::VaultManifest& manifest,
        std::string& error
    ) const;

    bool appendEncryptedFile(
        const std::filesystem::path& rootPath,
        const std::filesystem::path& filePath,
        std::size_t fileIndex,
        manifest::VaultManifest& manifest,
        std::string& error
    ) const;

    std::uint64_t chunkSize_;
};

} // namespace vault::chunking

#endif
