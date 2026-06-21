#ifndef VAULT_CRYPTO_ENCRYPTION_ENGINE_HPP
#define VAULT_CRYPTO_ENCRYPTION_ENGINE_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace vault::crypto {

using ByteBuffer = std::vector<unsigned char>;

struct CryptoKey {
    ByteBuffer bytes;

    static CryptoKey fromPassword(const std::string& password) {
        return CryptoKey{ByteBuffer(password.begin(), password.end())};
    }

    bool empty() const {
        return bytes.empty();
    }
};

struct EncryptionContext {
    std::string fileId;
    std::size_t chunkIndex = 0;
    std::uint64_t offset = 0;
    std::uint64_t plainSize = 0;
    std::string storageKey;
};

class EncryptionEngine {
public:
    virtual ~EncryptionEngine() = default;

    virtual std::string algorithmId() const = 0;

    virtual bool encrypt(
        const ByteBuffer& plaintext,
        const CryptoKey& key,
        const EncryptionContext& context,
        ByteBuffer& ciphertext,
        std::string& error
    ) const = 0;

    virtual bool decrypt(
        const ByteBuffer& ciphertext,
        const CryptoKey& key,
        const EncryptionContext& context,
        ByteBuffer& plaintext,
        std::string& error
    ) const = 0;
};

} // namespace vault::crypto

#endif
