#include "InMemoryMetadataStore.hpp"

namespace vault::metadata {

bool InMemoryMetadataStore::saveManifest(
    const manifest::VaultManifest& manifest,
    std::string&
) {
    latestManifest_ = manifest;
    return true;
}

std::optional<manifest::VaultManifest> InMemoryMetadataStore::loadLatest(
    std::string& error
) const {
    if (!latestManifest_.has_value()) {
        error = "no manifest has been saved";
        return std::nullopt;
    }

    return latestManifest_;
}

} // namespace vault::metadata
