#include "JobQueue.hpp"

#include <utility>

namespace vault::jobs {

void JobQueue::enqueue(const VaultJob& job) {
    jobs_.push(job);
}

void JobQueue::enqueueManifest(Action action, const manifest::VaultManifest& manifest) {
    for (const manifest::FileManifest& file : manifest.files()) {
        for (const manifest::ChunkManifest& chunk : file.chunks) {
            enqueue(VaultJob{action, file, chunk});
        }
    }
}

bool JobQueue::executeAll(
    const crypto::EncryptionEngine& encryptionEngine,
    const crypto::CryptoKey& key,
    storage::StorageAdapter& storageAdapter,
    metadata::MetadataStore& metadataStore,
    const manifest::VaultManifest& manifest,
    std::string& error
) {
    if (!metadataStore.saveManifest(manifest, error)) {
        return false;
    }

    if (!storageAdapter.prepareWrite(manifest, error)) {
        return false;
    }

    while (!jobs_.empty()) {
        VaultJob job = std::move(jobs_.front());
        jobs_.pop();

        if (!executeJob(job, encryptionEngine, key, storageAdapter, error)) {
            storageAdapter.discardWrite(manifest);
            return false;
        }
    }

    if (!storageAdapter.commitWrite(manifest, error)) {
        storageAdapter.discardWrite(manifest);
        return false;
    }

    return true;
}

bool JobQueue::empty() const {
    return jobs_.empty();
}

std::size_t JobQueue::size() const {
    return jobs_.size();
}

bool JobQueue::executeJob(
    const VaultJob& job,
    const crypto::EncryptionEngine& encryptionEngine,
    const crypto::CryptoKey& key,
    storage::StorageAdapter& storageAdapter,
    std::string& error
) const {
    crypto::ByteBuffer input;
    if (!storageAdapter.readChunk(job.file, job.chunk, input, error)) {
        return false;
    }

    crypto::EncryptionContext context;
    context.fileId = job.file.fileId;
    context.chunkIndex = job.chunk.index;
    context.offset = job.chunk.offset;
    context.plainSize = job.chunk.plainSize;
    context.storageKey = job.chunk.storageKey;

    crypto::ByteBuffer output;
    const bool transformed = job.action == Action::Encrypt
        ? encryptionEngine.encrypt(input, key, context, output, error)
        : encryptionEngine.decrypt(input, key, context, output, error);

    if (!transformed) {
        return false;
    }

    return storageAdapter.writeChunk(job.file, job.chunk, output, error);
}

} // namespace vault::jobs
