#ifndef VAULT_STORAGE_LOCAL_FILE_STORAGE_ADAPTER_HPP
#define VAULT_STORAGE_LOCAL_FILE_STORAGE_ADAPTER_HPP

#include "StorageAdapter.hpp"

namespace vault::storage {

class LocalFileStorageAdapter : public StorageAdapter {
public:
    std::string adapterId() const override;

    bool validateWritable(
        const manifest::VaultManifest& manifest,
        std::string& error
    ) const override;

    bool prepareWrite(
        const manifest::VaultManifest& manifest,
        std::string& error
    ) override;

    bool readChunk(
        const manifest::FileManifest& file,
        const manifest::ChunkManifest& chunk,
        crypto::ByteBuffer& output,
        std::string& error
    ) const override;

    bool writeChunk(
        const manifest::FileManifest& file,
        const manifest::ChunkManifest& chunk,
        const crypto::ByteBuffer& input,
        std::string& error
    ) override;

    bool commitWrite(
        const manifest::VaultManifest& manifest,
        std::string& error
    ) override;

    void discardWrite(
        const manifest::VaultManifest& manifest
    ) override;

private:
    std::string tempPathFor(const manifest::FileManifest& file) const;
};

} // namespace vault::storage

#endif
