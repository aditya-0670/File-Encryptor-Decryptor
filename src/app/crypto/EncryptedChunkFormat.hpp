#ifndef VAULT_CRYPTO_ENCRYPTED_CHUNK_FORMAT_HPP
#define VAULT_CRYPTO_ENCRYPTED_CHUNK_FORMAT_HPP

#include "EncryptionEngine.hpp"

#include <cstddef>
#include <cstdint>
#include <istream>
#include <string>

namespace vault::crypto {

struct EncryptedChunkHeader {
    std::uint8_t version = 1;
    std::uint8_t algorithm = 1;
    std::uint8_t kdf = 1;
    std::uint8_t flags = 0;
    std::uint32_t kdfIterations = 100000;
    std::uint16_t saltSize = 16;
    std::uint16_t nonceSize = 12;
    std::uint16_t tagSize = 16;
    std::uint64_t plainSize = 0;
    std::uint64_t cipherSize = 0;
    ByteBuffer salt;
    ByteBuffer nonce;

    std::size_t serializedSize() const;
    std::uint64_t packetSize() const;
};

std::size_t encryptedChunkFixedHeaderSize();

ByteBuffer serializeEncryptedChunkHeader(const EncryptedChunkHeader& header);

bool parseEncryptedChunkPacket(
    const ByteBuffer& packet,
    EncryptedChunkHeader& header,
    ByteBuffer& headerBytes,
    ByteBuffer& ciphertext,
    ByteBuffer& tag,
    std::string& error
);

bool readEncryptedChunkHeader(
    std::istream& input,
    EncryptedChunkHeader& header,
    std::string& error
);

bool validateEncryptedChunkHeader(
    const EncryptedChunkHeader& header,
    std::string& error
);

} // namespace vault::crypto

#endif
