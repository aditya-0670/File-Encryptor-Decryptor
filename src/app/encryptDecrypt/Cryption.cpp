#include "Cryption.hpp"
#include "../chunker/FileChunker.hpp"
#include "../core/Action.hpp"
#include "../crypto/Aes256GcmEncryptionEngine.hpp"
#include "../fileHandling/ReadEnv.hpp"
#include "../jobs/JobQueue.hpp"
#include "../metadata/InMemoryMetadataStore.hpp"
#include "../storage/LocalFileStorageAdapter.hpp"

#include <iostream>
#include <optional>
#include <string>

namespace {

struct LegacyTask {
    std::string filePath;
    vault::Action action;
};

std::optional<LegacyTask> parseLegacyTask(const std::string& taskData, std::string& error) {
    const std::size_t delimiter = taskData.rfind(',');
    if (delimiter == std::string::npos) {
        error = "invalid task data format";
        return std::nullopt;
    }

    const std::string filePath = taskData.substr(0, delimiter);
    const std::string actionText = taskData.substr(delimiter + 1);
    if (vault::trim(filePath).empty()) {
        error = "task data is missing a file path";
        return std::nullopt;
    }

    std::optional<vault::Action> action = vault::parseAction(actionText);
    if (!action.has_value()) {
        error = "invalid task action: " + actionText;
        return std::nullopt;
    }

    return LegacyTask{filePath, action.value()};
}

bool processFile(const std::string& filePath, vault::Action action, const std::string& password, std::string& error) {
    vault::chunking::FileChunker chunker;
    vault::manifest::VaultManifest manifest;
    if (!chunker.buildManifestForFile(filePath, action, manifest, error)) {
        return false;
    }

    vault::crypto::Aes256GcmEncryptionEngine encryptionEngine;
    manifest.setAlgorithmId(encryptionEngine.algorithmId());

    vault::storage::LocalFileStorageAdapter storageAdapter;
    if (!storageAdapter.validateWritable(manifest, error)) {
        return false;
    }

    vault::metadata::InMemoryMetadataStore metadataStore;
    vault::jobs::JobQueue jobQueue;
    jobQueue.enqueueManifest(action, manifest);

    const vault::crypto::CryptoKey key = vault::crypto::CryptoKey::fromPassword(password);
    return jobQueue.executeAll(encryptionEngine, key, storageAdapter, metadataStore, manifest, error);
}

} // namespace

int executeCryption(const std::string& taskData) {
    std::string passwordError;
    std::optional<std::string> password = ReadEnv::readPassword(passwordError);
    if (!password.has_value()) {
        std::cerr << "Password error: " << passwordError << std::endl;
        return 1;
    }

    return executeCryption(taskData, password.value());
}

int executeCryption(const std::string& taskData, const std::string& password) {
    if (password.empty()) {
        std::cerr << "Invalid password: password must not be empty" << std::endl;
        return 1;
    }

    std::string error;
    std::optional<LegacyTask> task = parseLegacyTask(taskData, error);
    if (!task.has_value()) {
        std::cerr << "Task error: " << error << std::endl;
        return 1;
    }

    if (!processFile(task->filePath, task->action, password, error)) {
        std::cerr << "Task error: " << error << std::endl;
        return 1;
    }

    return 0;
}
