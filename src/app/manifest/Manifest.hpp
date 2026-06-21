#ifndef VAULT_MANIFEST_MANIFEST_HPP
#define VAULT_MANIFEST_MANIFEST_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace vault::manifest {

struct ChunkManifest {
    std::string chunkId;
    std::size_t index = 0;
    std::uint64_t offset = 0;
    std::uint64_t plainSize = 0;
    std::uint64_t storedSize = 0;
    std::string storageKey;
};

struct FileManifest {
    std::string fileId;
    std::string sourcePath;
    std::string relativePath;
    std::uint64_t plainSize = 0;
    std::vector<ChunkManifest> chunks;
};

class VaultManifest {
public:
    void setRootPath(std::string rootPath);
    void setAlgorithmId(std::string algorithmId);
    void addFile(FileManifest file);

    const std::string& rootPath() const;
    const std::string& algorithmId() const;
    const std::vector<FileManifest>& files() const;

    bool empty() const;
    std::size_t fileCount() const;
    std::size_t chunkCount() const;

private:
    std::string rootPath_;
    std::string algorithmId_;
    std::vector<FileManifest> files_;
};

} // namespace vault::manifest

#endif
