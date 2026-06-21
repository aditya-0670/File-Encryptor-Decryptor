#include "CliApplication.hpp"

#include "../chunker/FileChunker.hpp"
#include "../crypto/Aes256GcmEncryptionEngine.hpp"
#include "../fileHandling/ReadEnv.hpp"
#include "../jobs/JobQueue.hpp"
#include "../metadata/InMemoryMetadataStore.hpp"
#include "../storage/LocalFileStorageAdapter.hpp"

#include <iostream>

namespace vault::cli {
namespace fs = std::filesystem;

int CliApplication::run(int argc, char* argv[]) {
    CliOptions options;
    if (!parseOptions(argc, argv, options)) {
        return argc == 2 && (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help") ? 0 : 1;
    }

    chunking::FileChunker chunker;
    manifest::VaultManifest manifest;
    std::string error;
    if (!chunker.buildManifestForDirectory(options.targetPath, options.action, manifest, error)) {
        std::cerr << "Manifest error: " << error << std::endl;
        return 1;
    }

    crypto::Aes256GcmEncryptionEngine encryptionEngine;
    manifest.setAlgorithmId(encryptionEngine.algorithmId());

    if (manifest.empty()) {
        std::cout << "No regular files found. Nothing to do." << std::endl;
        return 0;
    }

    std::string keyError;
    std::optional<std::string> password = ReadEnv::readPassword(keyError);
    if (!password.has_value()) {
        std::cerr << "Password error: " << keyError << std::endl;
        return 1;
    }

    storage::LocalFileStorageAdapter storageAdapter;
    if (!storageAdapter.validateWritable(manifest, error)) {
        std::cerr << "Storage error: " << error << std::endl;
        std::cerr << "Aborting before modifying any files." << std::endl;
        return 1;
    }

    metadata::InMemoryMetadataStore metadataStore;
    jobs::JobQueue jobQueue;
    jobQueue.enqueueManifest(options.action, manifest);

    const crypto::CryptoKey key = crypto::CryptoKey::fromPassword(password.value());
    if (!jobQueue.executeAll(encryptionEngine, key, storageAdapter, metadataStore, manifest, error)) {
        std::cerr << "Job error: " << error << std::endl;
        return 1;
    }

    std::cout << "Processed " << manifest.fileCount()
              << " file(s) across " << manifest.chunkCount()
              << " chunk(s) using " << encryptionEngine.algorithmId() << "."
              << std::endl;

    return 0;
}

bool CliApplication::parseOptions(int argc, char* argv[], CliOptions& options) const {
    std::string actionInput;

    if (argc == 1) {
        return readInteractiveInput(options);
    }

    if (argc == 2 && (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help")) {
        printUsage(argv[0]);
        return false;
    }

    if (argc != 3) {
        printUsage(argv[0]);
        return false;
    }

    options.targetPath = argv[1];
    actionInput = argv[2];

    std::optional<Action> action = parseAction(actionInput);
    if (!action.has_value()) {
        std::cerr << "Invalid action: " << actionInput << std::endl;
        std::cerr << "Expected 'encrypt' or 'decrypt'." << std::endl;
        return false;
    }

    options.action = action.value();
    return true;
}

bool CliApplication::readInteractiveInput(CliOptions& options) const {
    std::string directory;
    std::string actionInput;

    std::cout << "Enter the directory path: ";
    if (!std::getline(std::cin, directory)) {
        std::cerr << "Failed to read directory path." << std::endl;
        return false;
    }

    std::cout << "Enter the action (encrypt/decrypt): ";
    if (!std::getline(std::cin, actionInput)) {
        std::cerr << "Failed to read action." << std::endl;
        return false;
    }

    std::optional<Action> action = parseAction(actionInput);
    if (!action.has_value()) {
        std::cerr << "Invalid action: " << actionInput << std::endl;
        std::cerr << "Expected 'encrypt' or 'decrypt'." << std::endl;
        return false;
    }

    options.targetPath = fs::path(directory);
    options.action = action.value();
    return true;
}

void CliApplication::printUsage(const char* programName) const {
    std::cerr << "Usage: " << programName << " <directory> <encrypt|decrypt>" << std::endl;
    std::cerr << "Or run without arguments for interactive prompts." << std::endl;
}

} // namespace vault::cli
