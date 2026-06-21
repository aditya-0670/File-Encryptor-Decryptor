#ifndef VAULT_METADATA_IN_MEMORY_METADATA_STORE_HPP
#define VAULT_METADATA_IN_MEMORY_METADATA_STORE_HPP

#include "MetadataStore.hpp"

namespace vault::metadata {

class InMemoryMetadataStore : public MetadataStore {
public:
    bool saveManifest(
        const manifest::VaultManifest& manifest,
        std::string& error
    ) override;

    std::optional<manifest::VaultManifest> loadLatest(
        std::string& error
    ) const override;

private:
    std::optional<manifest::VaultManifest> latestManifest_;
};

} // namespace vault::metadata

#endif
