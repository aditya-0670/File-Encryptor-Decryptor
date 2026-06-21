#ifndef VAULT_CRYPTO_AES_256_GCM_ENCRYPTION_ENGINE_HPP
#define VAULT_CRYPTO_AES_256_GCM_ENCRYPTION_ENGINE_HPP

#include "EncryptionEngine.hpp"

namespace vault::crypto {

class Aes256GcmEncryptionEngine : public EncryptionEngine {
public:
    std::string algorithmId() const override;

    bool encrypt(
        const ByteBuffer& plaintext,
        const CryptoKey& key,
        const EncryptionContext& context,
        ByteBuffer& ciphertext,
        std::string& error
    ) const override;

    bool decrypt(
        const ByteBuffer& ciphertext,
        const CryptoKey& key,
        const EncryptionContext& context,
        ByteBuffer& plaintext,
        std::string& error
    ) const override;
};

} // namespace vault::crypto

#endif
