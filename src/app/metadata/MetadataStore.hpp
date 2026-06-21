#ifndef VAULT_METADATA_METADATA_STORE_HPP
#define VAULT_METADATA_METADATA_STORE_HPP

#include "../manifest/Manifest.hpp"

#include <optional>
#include <string>

namespace vault::metadata {

class MetadataStore {
public:
    virtual ~MetadataStore() = default;

    virtual bool saveManifest(
        const manifest::VaultManifest& manifest,
        std::string& error
    ) = 0;

    virtual std::optional<manifest::VaultManifest> loadLatest(
        std::string& error
    ) const = 0;
};

} // namespace vault::metadata

#endif
