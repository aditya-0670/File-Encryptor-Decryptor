#ifndef VAULT_JOBS_JOB_QUEUE_HPP
#define VAULT_JOBS_JOB_QUEUE_HPP

#include "../core/Action.hpp"
#include "../crypto/EncryptionEngine.hpp"
#include "../manifest/Manifest.hpp"
#include "../metadata/MetadataStore.hpp"
#include "../storage/StorageAdapter.hpp"

#include <queue>

namespace vault::jobs {

struct VaultJob {
    Action action;
    manifest::FileManifest file;
    manifest::ChunkManifest chunk;
};

class JobQueue {
public:
    void enqueue(const VaultJob& job);
    void enqueueManifest(Action action, const manifest::VaultManifest& manifest);

    bool executeAll(
        const crypto::EncryptionEngine& encryptionEngine,
        const crypto::CryptoKey& key,
        storage::StorageAdapter& storageAdapter,
        metadata::MetadataStore& metadataStore,
        const manifest::VaultManifest& manifest,
        std::string& error
    );

    bool empty() const;
    std::size_t size() const;

private:
    bool executeJob(
        const VaultJob& job,
        const crypto::EncryptionEngine& encryptionEngine,
        const crypto::CryptoKey& key,
        storage::StorageAdapter& storageAdapter,
        std::string& error
    ) const;

    std::queue<VaultJob> jobs_;
};

} // namespace vault::jobs

#endif
