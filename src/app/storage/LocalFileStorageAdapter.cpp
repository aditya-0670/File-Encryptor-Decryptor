#include "LocalFileStorageAdapter.hpp"

#include <filesystem>
#include <fstream>
#include <ios>
#include <system_error>

namespace vault::storage {
namespace fs = std::filesystem;

std::string LocalFileStorageAdapter::adapterId() const {
    return "local-file-staged-replace";
}

bool LocalFileStorageAdapter::validateWritable(
    const manifest::VaultManifest& manifest,
    std::string& error
) const {
    for (const manifest::FileManifest& file : manifest.files()) {
        std::fstream stream(file.sourcePath, std::ios::in | std::ios::out | std::ios::binary);
        if (!stream.is_open()) {
            error = "unable to open file for update: " + file.sourcePath;
            return false;
        }
    }

    return true;
}

bool LocalFileStorageAdapter::prepareWrite(
    const manifest::VaultManifest& manifest,
    std::string& error
) {
    for (const manifest::FileManifest& file : manifest.files()) {
        const std::string tempPath = tempPathFor(file);
        std::error_code removeError;
        fs::remove(tempPath, removeError);
        if (removeError) {
            error = "unable to remove stale temp file: " + tempPath + ": " + removeError.message();
            discardWrite(manifest);
            return false;
        }

        std::ofstream output(tempPath, std::ios::binary | std::ios::trunc);
        if (!output.is_open()) {
            error = "unable to create temp output file: " + tempPath;
            discardWrite(manifest);
            return false;
        }
    }

    return true;
}

bool LocalFileStorageAdapter::readChunk(
    const manifest::FileManifest& file,
    const manifest::ChunkManifest& chunk,
    crypto::ByteBuffer& output,
    std::string& error
) const {
    std::ifstream stream(file.sourcePath, std::ios::binary);
    if (!stream.is_open()) {
        error = "unable to open file for reading: " + file.sourcePath;
        return false;
    }

    output.assign(static_cast<std::size_t>(chunk.storedSize), 0);
    if (chunk.storedSize == 0) {
        return true;
    }

    stream.seekg(static_cast<std::streamoff>(chunk.offset), std::ios::beg);
    if (!stream.good()) {
        error = "unable to seek file for reading: " + file.sourcePath;
        return false;
    }

    stream.read(
        reinterpret_cast<char*>(output.data()),
        static_cast<std::streamsize>(output.size())
    );
    if (stream.gcount() != static_cast<std::streamsize>(output.size())) {
        error = "unable to read complete chunk from: " + file.sourcePath;
        return false;
    }

    return true;
}

bool LocalFileStorageAdapter::writeChunk(
    const manifest::FileManifest& file,
    const manifest::ChunkManifest&,
    const crypto::ByteBuffer& input,
    std::string& error
) {
    const std::string tempPath = tempPathFor(file);
    std::ofstream stream(tempPath, std::ios::binary | std::ios::app);
    if (!stream.is_open()) {
        error = "unable to open temp output file for writing: " + tempPath;
        return false;
    }

    if (input.empty()) {
        return true;
    }

    stream.write(
        reinterpret_cast<const char*>(input.data()),
        static_cast<std::streamsize>(input.size())
    );
    if (!stream.good()) {
        error = "unable to write chunk to temp output file: " + tempPath;
        return false;
    }

    return true;
}

bool LocalFileStorageAdapter::commitWrite(
    const manifest::VaultManifest& manifest,
    std::string& error
) {
    for (const manifest::FileManifest& file : manifest.files()) {
        const std::string tempPath = tempPathFor(file);
        std::error_code renameError;
        fs::rename(tempPath, file.sourcePath, renameError);
        if (renameError) {
            error = "unable to replace file with staged output: " + file.sourcePath + ": " + renameError.message();
            discardWrite(manifest);
            return false;
        }
    }

    return true;
}

void LocalFileStorageAdapter::discardWrite(
    const manifest::VaultManifest& manifest
) {
    for (const manifest::FileManifest& file : manifest.files()) {
        std::error_code removeError;
        fs::remove(tempPathFor(file), removeError);
    }
}

std::string LocalFileStorageAdapter::tempPathFor(const manifest::FileManifest& file) const {
    return file.sourcePath + ".sdfv.tmp";
}

} // namespace vault::storage
