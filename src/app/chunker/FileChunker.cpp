#include "FileChunker.hpp"

#include "../crypto/EncryptedChunkFormat.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <system_error>
#include <utility>
#include <vector>

namespace vault::chunking {
namespace fs = std::filesystem;

FileChunker::FileChunker(std::uint64_t chunkSize)
    : chunkSize_(chunkSize == 0 ? DefaultChunkSize : chunkSize) {}

bool FileChunker::buildManifestForDirectory(
    const fs::path& rootPath,
    Action action,
    manifest::VaultManifest& manifest,
    std::string& error
) const {
    std::error_code pathError;
    if (!fs::exists(rootPath, pathError) || !fs::is_directory(rootPath, pathError)) {
        error = "invalid directory path: " + rootPath.string();
        return false;
    }
    if (pathError) {
        error = "unable to inspect directory: " + pathError.message();
        return false;
    }

    std::vector<fs::path> files;
    fs::recursive_directory_iterator iterator(
        rootPath,
        fs::directory_options::skip_permission_denied,
        pathError
    );
    if (pathError) {
        error = "unable to scan directory: " + pathError.message();
        return false;
    }

    const fs::recursive_directory_iterator end;
    while (iterator != end) {
        const fs::directory_entry& entry = *iterator;
        std::error_code entryError;
        if (entry.is_regular_file(entryError)) {
            files.push_back(entry.path());
        }
        if (entryError) {
            error = "unable to inspect path " + entry.path().string() + ": " + entryError.message();
            return false;
        }

        iterator.increment(pathError);
        if (pathError) {
            error = "unable to continue directory scan: " + pathError.message();
            return false;
        }
    }

    std::sort(files.begin(), files.end());

    manifest = manifest::VaultManifest{};
    manifest.setRootPath(rootPath.string());

    for (std::size_t index = 0; index < files.size(); ++index) {
        if (!appendFile(rootPath, files[index], index, action, manifest, error)) {
            return false;
        }
    }

    return true;
}

bool FileChunker::buildManifestForFile(
    const fs::path& filePath,
    Action action,
    manifest::VaultManifest& manifest,
    std::string& error
) const {
    std::error_code pathError;
    if (!fs::exists(filePath, pathError) || !fs::is_regular_file(filePath, pathError)) {
        error = "invalid file path: " + filePath.string();
        return false;
    }
    if (pathError) {
        error = "unable to inspect file: " + pathError.message();
        return false;
    }

    manifest = manifest::VaultManifest{};
    manifest.setRootPath(filePath.parent_path().string());
    return appendFile(filePath.parent_path(), filePath, 0, action, manifest, error);
}

std::uint64_t FileChunker::chunkSize() const {
    return chunkSize_;
}

bool FileChunker::appendFile(
    const fs::path& rootPath,
    const fs::path& filePath,
    std::size_t fileIndex,
    Action action,
    manifest::VaultManifest& manifest,
    std::string& error
) const {
    if (action == Action::Encrypt) {
        return appendPlaintextFile(rootPath, filePath, fileIndex, manifest, error);
    }

    return appendEncryptedFile(rootPath, filePath, fileIndex, manifest, error);
}

bool FileChunker::appendPlaintextFile(
    const fs::path& rootPath,
    const fs::path& filePath,
    std::size_t fileIndex,
    manifest::VaultManifest& manifest,
    std::string& error
) const {
    std::error_code sizeError;
    const std::uint64_t fileSize = static_cast<std::uint64_t>(fs::file_size(filePath, sizeError));
    if (sizeError) {
        error = "unable to read file size for " + filePath.string() + ": " + sizeError.message();
        return false;
    }

    std::error_code relativeError;
    fs::path relativePath = fs::relative(filePath, rootPath, relativeError);
    if (relativeError || relativePath.empty()) {
        relativePath = filePath.filename();
    }

    manifest::FileManifest file;
    file.fileId = "file-" + std::to_string(fileIndex);
    file.sourcePath = filePath.string();
    file.relativePath = relativePath.string();
    file.plainSize = fileSize;

    const std::uint64_t chunkTotal = fileSize == 0
        ? 1
        : ((fileSize + chunkSize_ - 1) / chunkSize_);

    for (std::uint64_t chunkIndex = 0; chunkIndex < chunkTotal; ++chunkIndex) {
        const std::uint64_t offset = chunkIndex * chunkSize_;
        const std::uint64_t remaining = fileSize > offset ? fileSize - offset : 0;
        const std::uint64_t plainSize = std::min(chunkSize_, remaining);

        manifest::ChunkManifest chunk;
        chunk.index = static_cast<std::size_t>(chunkIndex);
        chunk.offset = offset;
        chunk.plainSize = plainSize;
        chunk.storedSize = plainSize;
        chunk.storageKey = file.sourcePath;
        chunk.chunkId = file.fileId + ":chunk-" + std::to_string(chunkIndex);
        file.chunks.push_back(std::move(chunk));
    }

    manifest.addFile(std::move(file));
    return true;
}

bool FileChunker::appendEncryptedFile(
    const fs::path& rootPath,
    const fs::path& filePath,
    std::size_t fileIndex,
    manifest::VaultManifest& manifest,
    std::string& error
) const {
    std::error_code sizeError;
    const std::uint64_t fileSize = static_cast<std::uint64_t>(fs::file_size(filePath, sizeError));
    if (sizeError) {
        error = "unable to read file size for " + filePath.string() + ": " + sizeError.message();
        return false;
    }

    std::error_code relativeError;
    fs::path relativePath = fs::relative(filePath, rootPath, relativeError);
    if (relativeError || relativePath.empty()) {
        relativePath = filePath.filename();
    }

    std::ifstream input(filePath, std::ios::binary);
    if (!input.is_open()) {
        error = "unable to open encrypted file for header scan: " + filePath.string();
        return false;
    }

    manifest::FileManifest file;
    file.fileId = "file-" + std::to_string(fileIndex);
    file.sourcePath = filePath.string();
    file.relativePath = relativePath.string();
    file.plainSize = 0;

    std::uint64_t offset = 0;
    std::size_t chunkIndex = 0;
    while (offset < fileSize) {
        input.clear();
        input.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
        if (!input.good()) {
            error = "unable to seek encrypted file during header scan: " + filePath.string();
            return false;
        }

        crypto::EncryptedChunkHeader header;
        std::string headerError;
        if (!crypto::readEncryptedChunkHeader(input, header, headerError)) {
            error = "invalid encrypted file " + filePath.string() + ": " + headerError;
            return false;
        }

        const std::uint64_t packetSize = header.packetSize();
        if (packetSize == 0 || offset + packetSize > fileSize) {
            error = "invalid encrypted file " + filePath.string() + ": chunk packet exceeds file size";
            return false;
        }

        manifest::ChunkManifest chunk;
        chunk.index = chunkIndex;
        chunk.offset = offset;
        chunk.plainSize = header.plainSize;
        chunk.storedSize = packetSize;
        chunk.storageKey = file.sourcePath;
        chunk.chunkId = file.fileId + ":chunk-" + std::to_string(chunkIndex);
        file.chunks.push_back(std::move(chunk));

        file.plainSize += header.plainSize;
        offset += packetSize;
        ++chunkIndex;
    }

    if (fileSize == 0 || file.chunks.empty()) {
        error = "invalid encrypted file " + filePath.string() + ": missing encrypted chunk header";
        return false;
    }

    manifest.addFile(std::move(file));
    return true;
}

} // namespace vault::chunking
