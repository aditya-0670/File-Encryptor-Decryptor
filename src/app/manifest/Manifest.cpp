#include "Manifest.hpp"

#include <utility>

namespace vault::manifest {

void VaultManifest::setRootPath(std::string rootPath) {
    rootPath_ = std::move(rootPath);
}

void VaultManifest::setAlgorithmId(std::string algorithmId) {
    algorithmId_ = std::move(algorithmId);
}

void VaultManifest::addFile(FileManifest file) {
    files_.push_back(std::move(file));
}

const std::string& VaultManifest::rootPath() const {
    return rootPath_;
}

const std::string& VaultManifest::algorithmId() const {
    return algorithmId_;
}

const std::vector<FileManifest>& VaultManifest::files() const {
    return files_;
}

bool VaultManifest::empty() const {
    return files_.empty();
}

std::size_t VaultManifest::fileCount() const {
    return files_.size();
}

std::size_t VaultManifest::chunkCount() const {
    std::size_t total = 0;
    for (const FileManifest& file : files_) {
        total += file.chunks.size();
    }
    return total;
}

} // namespace vault::manifest
