#ifndef VAULT_STORAGE_STORAGE_ADAPTER_HPP
#define VAULT_STORAGE_STORAGE_ADAPTER_HPP

#include "../crypto/EncryptionEngine.hpp"
#include "../manifest/Manifest.hpp"

#include <string>

namespace vault::storage {

class StorageAdapter {
public:
    virtual ~StorageAdapter() = default;

    virtual std::string adapterId() const = 0;

    virtual bool validateWritable(
        const manifest::VaultManifest& manifest,
        std::string& error
    ) const = 0;

    virtual bool prepareWrite(
        const manifest::VaultManifest& manifest,
        std::string& error
    ) = 0;

    virtual bool readChunk(
        const manifest::FileManifest& file,
        const manifest::ChunkManifest& chunk,
        crypto::ByteBuffer& output,
        std::string& error
    ) const = 0;

    virtual bool writeChunk(
        const manifest::FileManifest& file,
        const manifest::ChunkManifest& chunk,
        const crypto::ByteBuffer& input,
        std::string& error
    ) = 0;

    virtual bool commitWrite(
        const manifest::VaultManifest& manifest,
        std::string& error
    ) = 0;

    virtual void discardWrite(
        const manifest::VaultManifest& manifest
    ) = 0;
};

} // namespace vault::storage

#endif
