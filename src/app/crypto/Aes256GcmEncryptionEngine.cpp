#include "Aes256GcmEncryptionEngine.hpp"

#include "EncryptedChunkFormat.hpp"

#include <openssl/evp.h>
#include <openssl/rand.h>

#include <memory>

namespace vault::crypto {
namespace {

constexpr std::uint32_t KdfIterations = 100000;
constexpr std::size_t DerivedKeySize = 32;
constexpr std::size_t SaltSize = 16;
constexpr std::size_t NonceSize = 12;
constexpr std::size_t TagSize = 16;

using EvpCipherContext = std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)>;

EvpCipherContext makeCipherContext() {
    return EvpCipherContext(EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
}

bool randomBytes(ByteBuffer& output, std::size_t size, std::string& error) {
    output.assign(size, 0);
    if (RAND_bytes(output.data(), static_cast<int>(output.size())) != 1) {
        error = "unable to generate secure random bytes";
        return false;
    }
    return true;
}

bool deriveKey(
    const CryptoKey& password,
    const ByteBuffer& salt,
    std::uint32_t iterations,
    ByteBuffer& derivedKey,
    std::string& error
) {
    if (password.empty()) {
        error = "password must not be empty";
        return false;
    }
    if (iterations == 0) {
        error = "KDF iterations must be greater than zero";
        return false;
    }

    derivedKey.assign(DerivedKeySize, 0);
    const int ok = PKCS5_PBKDF2_HMAC(
        reinterpret_cast<const char*>(password.bytes.data()),
        static_cast<int>(password.bytes.size()),
        salt.data(),
        static_cast<int>(salt.size()),
        static_cast<int>(iterations),
        EVP_sha256(),
        static_cast<int>(derivedKey.size()),
        derivedKey.data()
    );
    if (ok != 1) {
        error = "unable to derive encryption key from password";
        return false;
    }

    return true;
}

bool gcmEncrypt(
    const ByteBuffer& plaintext,
    const ByteBuffer& derivedKey,
    const ByteBuffer& nonce,
    const ByteBuffer& aad,
    ByteBuffer& ciphertext,
    ByteBuffer& tag,
    std::string& error
) {
    EvpCipherContext context = makeCipherContext();
    if (!context) {
        error = "unable to allocate AES-GCM context";
        return false;
    }

    if (EVP_EncryptInit_ex(context.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
        error = "unable to initialize AES-GCM encryption";
        return false;
    }
    if (EVP_CIPHER_CTX_ctrl(context.get(), EVP_CTRL_GCM_SET_IVLEN, static_cast<int>(nonce.size()), nullptr) != 1) {
        error = "unable to configure AES-GCM nonce size";
        return false;
    }
    if (EVP_EncryptInit_ex(context.get(), nullptr, nullptr, derivedKey.data(), nonce.data()) != 1) {
        error = "unable to set AES-GCM encryption key";
        return false;
    }

    int outLen = 0;
    if (!aad.empty() && EVP_EncryptUpdate(
        context.get(),
        nullptr,
        &outLen,
        aad.data(),
        static_cast<int>(aad.size())
    ) != 1) {
        error = "unable to authenticate AES-GCM header";
        return false;
    }

    ciphertext.assign(plaintext.size(), 0);
    if (!plaintext.empty() && EVP_EncryptUpdate(
        context.get(),
        ciphertext.data(),
        &outLen,
        plaintext.data(),
        static_cast<int>(plaintext.size())
    ) != 1) {
        error = "unable to encrypt AES-GCM chunk";
        return false;
    }

    int finalLen = 0;
    if (EVP_EncryptFinal_ex(context.get(), ciphertext.data() + outLen, &finalLen) != 1) {
        error = "unable to finalize AES-GCM encryption";
        return false;
    }
    ciphertext.resize(static_cast<std::size_t>(outLen + finalLen));

    tag.assign(TagSize, 0);
    if (EVP_CIPHER_CTX_ctrl(context.get(), EVP_CTRL_GCM_GET_TAG, static_cast<int>(tag.size()), tag.data()) != 1) {
        error = "unable to read AES-GCM authentication tag";
        return false;
    }

    return true;
}

bool gcmDecrypt(
    const ByteBuffer& ciphertext,
    const ByteBuffer& derivedKey,
    const ByteBuffer& nonce,
    const ByteBuffer& aad,
    const ByteBuffer& tag,
    ByteBuffer& plaintext,
    std::string& error
) {
    EvpCipherContext context = makeCipherContext();
    if (!context) {
        error = "unable to allocate AES-GCM context";
        return false;
    }

    if (EVP_DecryptInit_ex(context.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
        error = "unable to initialize AES-GCM decryption";
        return false;
    }
    if (EVP_CIPHER_CTX_ctrl(context.get(), EVP_CTRL_GCM_SET_IVLEN, static_cast<int>(nonce.size()), nullptr) != 1) {
        error = "unable to configure AES-GCM nonce size";
        return false;
    }
    if (EVP_DecryptInit_ex(context.get(), nullptr, nullptr, derivedKey.data(), nonce.data()) != 1) {
        error = "unable to set AES-GCM decryption key";
        return false;
    }

    int outLen = 0;
    if (!aad.empty() && EVP_DecryptUpdate(
        context.get(),
        nullptr,
        &outLen,
        aad.data(),
        static_cast<int>(aad.size())
    ) != 1) {
        error = "unable to authenticate AES-GCM header";
        return false;
    }

    plaintext.assign(ciphertext.size(), 0);
    if (!ciphertext.empty() && EVP_DecryptUpdate(
        context.get(),
        plaintext.data(),
        &outLen,
        ciphertext.data(),
        static_cast<int>(ciphertext.size())
    ) != 1) {
        error = "unable to decrypt AES-GCM chunk";
        return false;
    }

    if (EVP_CIPHER_CTX_ctrl(context.get(), EVP_CTRL_GCM_SET_TAG, static_cast<int>(tag.size()), const_cast<unsigned char*>(tag.data())) != 1) {
        error = "unable to set AES-GCM authentication tag";
        return false;
    }

    int finalLen = 0;
    if (EVP_DecryptFinal_ex(context.get(), plaintext.data() + outLen, &finalLen) != 1) {
        error = "authentication failed: ciphertext, header, password, or tag is invalid";
        plaintext.clear();
        return false;
    }
    plaintext.resize(static_cast<std::size_t>(outLen + finalLen));

    return true;
}

} // namespace

std::string Aes256GcmEncryptionEngine::algorithmId() const {
    return "aes-256-gcm-pbkdf2-sha256-v1";
}

bool Aes256GcmEncryptionEngine::encrypt(
    const ByteBuffer& plaintext,
    const CryptoKey& key,
    const EncryptionContext&,
    ByteBuffer& ciphertext,
    std::string& error
) const {
    EncryptedChunkHeader header;
    header.kdfIterations = KdfIterations;
    header.plainSize = static_cast<std::uint64_t>(plaintext.size());
    header.cipherSize = static_cast<std::uint64_t>(plaintext.size());

    if (!randomBytes(header.salt, SaltSize, error)) {
        return false;
    }
    if (!randomBytes(header.nonce, NonceSize, error)) {
        return false;
    }
    header.saltSize = static_cast<std::uint16_t>(header.salt.size());
    header.nonceSize = static_cast<std::uint16_t>(header.nonce.size());
    header.tagSize = TagSize;

    ByteBuffer derivedKey;
    if (!deriveKey(key, header.salt, header.kdfIterations, derivedKey, error)) {
        return false;
    }

    ByteBuffer headerBytes = serializeEncryptedChunkHeader(header);
    ByteBuffer encryptedPayload;
    ByteBuffer tag;
    if (!gcmEncrypt(plaintext, derivedKey, header.nonce, headerBytes, encryptedPayload, tag, error)) {
        return false;
    }

    ciphertext.clear();
    ciphertext.reserve(headerBytes.size() + encryptedPayload.size() + tag.size());
    ciphertext.insert(ciphertext.end(), headerBytes.begin(), headerBytes.end());
    ciphertext.insert(ciphertext.end(), encryptedPayload.begin(), encryptedPayload.end());
    ciphertext.insert(ciphertext.end(), tag.begin(), tag.end());

    return true;
}

bool Aes256GcmEncryptionEngine::decrypt(
    const ByteBuffer& ciphertext,
    const CryptoKey& key,
    const EncryptionContext&,
    ByteBuffer& plaintext,
    std::string& error
) const {
    EncryptedChunkHeader header;
    ByteBuffer headerBytes;
    ByteBuffer encryptedPayload;
    ByteBuffer tag;
    if (!parseEncryptedChunkPacket(ciphertext, header, headerBytes, encryptedPayload, tag, error)) {
        plaintext.clear();
        return false;
    }

    ByteBuffer derivedKey;
    if (!deriveKey(key, header.salt, header.kdfIterations, derivedKey, error)) {
        plaintext.clear();
        return false;
    }

    return gcmDecrypt(encryptedPayload, derivedKey, header.nonce, headerBytes, tag, plaintext, error);
}

} // namespace vault::crypto
