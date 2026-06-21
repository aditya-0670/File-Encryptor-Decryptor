#include "src/app/chunker/FileChunker.hpp"
#include "src/app/core/Action.hpp"
#include "src/app/crypto/Aes256GcmEncryptionEngine.hpp"
#include "src/app/jobs/JobQueue.hpp"
#include "src/app/metadata/InMemoryMetadataStore.hpp"
#include "src/app/storage/LocalFileStorageAdapter.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <string>

namespace fs = std::filesystem;

namespace {

void writeTextFile(const fs::path& path, const std::string& content) {
    std::ofstream output(path, std::ios::binary);
    output << content;
}

std::string readTextFile(const fs::path& path) {
    std::ifstream input(path, std::ios::binary);
    return std::string(
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>()
    );
}

} // namespace

int main() {
    assert(vault::parseAction("encrypt").value() == vault::Action::Encrypt);
    assert(vault::parseAction("DeCrYpT").value() == vault::Action::Decrypt);
    assert(!vault::parseAction("rotate").has_value());

    vault::crypto::Aes256GcmEncryptionEngine engine;
    vault::crypto::CryptoKey key = vault::crypto::CryptoKey::fromPassword("correct horse battery staple");
    vault::crypto::EncryptionContext context;
    vault::crypto::ByteBuffer plaintext{'A', 'B', 'C'};
    vault::crypto::ByteBuffer ciphertext;
    vault::crypto::ByteBuffer decrypted;
    std::string error;

    assert(engine.encrypt(plaintext, key, context, ciphertext, error));
    assert(ciphertext != plaintext);
    assert(ciphertext.size() > plaintext.size());
    assert(ciphertext[0] == 'S');
    assert(ciphertext[1] == 'D');
    assert(ciphertext[2] == 'F');
    assert(ciphertext[3] == 'V');
    assert(engine.decrypt(ciphertext, key, context, decrypted, error));
    assert(decrypted == plaintext);

    vault::crypto::ByteBuffer tamperedPacket = ciphertext;
    tamperedPacket.back() ^= 0x01;
    assert(!engine.decrypt(tamperedPacket, key, context, decrypted, error));
    assert(decrypted.empty());

    const fs::path workdir = fs::current_path() / "test" / "tmp_architecture_cpp";
    const fs::path inputDir = workdir / "input";
    const fs::path sampleFile = inputDir / "sample.txt";

    fs::remove_all(workdir);
    fs::create_directories(inputDir);
    writeTextFile(sampleFile, "abcdefghij");
    const std::string original = readTextFile(sampleFile);

    vault::chunking::FileChunker chunker(4);
    vault::manifest::VaultManifest encryptManifest;
    assert(chunker.buildManifestForDirectory(inputDir, vault::Action::Encrypt, encryptManifest, error));
    encryptManifest.setAlgorithmId(engine.algorithmId());

    assert(encryptManifest.fileCount() == 1);
    assert(encryptManifest.chunkCount() == 3);
    assert(encryptManifest.files().front().chunks[0].offset == 0);
    assert(encryptManifest.files().front().chunks[1].offset == 4);
    assert(encryptManifest.files().front().chunks[2].plainSize == 2);

    vault::storage::LocalFileStorageAdapter storage;
    assert(storage.validateWritable(encryptManifest, error));

    vault::metadata::InMemoryMetadataStore metadata;
    vault::jobs::JobQueue encryptQueue;
    encryptQueue.enqueueManifest(vault::Action::Encrypt, encryptManifest);
    assert(encryptQueue.size() == 3);
    assert(encryptQueue.executeAll(engine, key, storage, metadata, encryptManifest, error));
    const std::string encrypted = readTextFile(sampleFile);
    assert(encrypted != original);
    assert(encrypted.size() > original.size());
    assert(encrypted.rfind("SDFV", 0) == 0);

    std::optional<vault::manifest::VaultManifest> latest = metadata.loadLatest(error);
    assert(latest.has_value());
    assert(latest->algorithmId() == "aes-256-gcm-pbkdf2-sha256-v1");

    vault::manifest::VaultManifest decryptManifest;
    assert(chunker.buildManifestForDirectory(inputDir, vault::Action::Decrypt, decryptManifest, error));
    decryptManifest.setAlgorithmId(engine.algorithmId());
    assert(decryptManifest.chunkCount() == 3);
    assert(decryptManifest.files().front().chunks[0].plainSize == 4);

    vault::jobs::JobQueue decryptQueue;
    decryptQueue.enqueueManifest(vault::Action::Decrypt, decryptManifest);
    assert(decryptQueue.executeAll(engine, key, storage, metadata, decryptManifest, error));
    assert(readTextFile(sampleFile) == original);

    assert(chunker.buildManifestForDirectory(inputDir, vault::Action::Encrypt, encryptManifest, error));
    encryptManifest.setAlgorithmId(engine.algorithmId());
    vault::jobs::JobQueue tamperEncryptQueue;
    tamperEncryptQueue.enqueueManifest(vault::Action::Encrypt, encryptManifest);
    assert(tamperEncryptQueue.executeAll(engine, key, storage, metadata, encryptManifest, error));

    std::string tamperedFile = readTextFile(sampleFile);
    tamperedFile.back() ^= 0x01;
    writeTextFile(sampleFile, tamperedFile);
    assert(chunker.buildManifestForDirectory(inputDir, vault::Action::Decrypt, decryptManifest, error));
    decryptManifest.setAlgorithmId(engine.algorithmId());

    vault::jobs::JobQueue tamperDecryptQueue;
    tamperDecryptQueue.enqueueManifest(vault::Action::Decrypt, decryptManifest);
    assert(!tamperDecryptQueue.executeAll(engine, key, storage, metadata, decryptManifest, error));
    assert(error.find("authentication failed") != std::string::npos);
    assert(readTextFile(sampleFile) == tamperedFile);
    assert(!fs::exists(sampleFile.string() + ".sdfv.tmp"));

    fs::remove_all(workdir);
    std::cout << "Architecture tests passed." << std::endl;
    return 0;
}
