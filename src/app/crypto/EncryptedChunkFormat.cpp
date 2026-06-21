#include "EncryptedChunkFormat.hpp"

#include <array>
#include <limits>

namespace vault::crypto {
namespace {

constexpr std::array<unsigned char, 4> Magic{{'S', 'D', 'F', 'V'}};
constexpr std::uint8_t Version = 1;
constexpr std::uint8_t AlgorithmAes256Gcm = 1;
constexpr std::uint8_t KdfPbkdf2Sha256 = 1;
constexpr std::uint16_t SaltSize = 16;
constexpr std::uint16_t NonceSize = 12;
constexpr std::uint16_t TagSize = 16;
constexpr std::size_t FixedHeaderSize = 34;

void appendUint16(ByteBuffer& output, std::uint16_t value) {
    output.push_back(static_cast<unsigned char>((value >> 8) & 0xff));
    output.push_back(static_cast<unsigned char>(value & 0xff));
}

void appendUint32(ByteBuffer& output, std::uint32_t value) {
    output.push_back(static_cast<unsigned char>((value >> 24) & 0xff));
    output.push_back(static_cast<unsigned char>((value >> 16) & 0xff));
    output.push_back(static_cast<unsigned char>((value >> 8) & 0xff));
    output.push_back(static_cast<unsigned char>(value & 0xff));
}

void appendUint64(ByteBuffer& output, std::uint64_t value) {
    for (int shift = 56; shift >= 0; shift -= 8) {
        output.push_back(static_cast<unsigned char>((value >> shift) & 0xff));
    }
}

std::uint16_t readUint16(const ByteBuffer& input, std::size_t offset) {
    return static_cast<std::uint16_t>((input[offset] << 8) | input[offset + 1]);
}

std::uint32_t readUint32(const ByteBuffer& input, std::size_t offset) {
    return (static_cast<std::uint32_t>(input[offset]) << 24)
        | (static_cast<std::uint32_t>(input[offset + 1]) << 16)
        | (static_cast<std::uint32_t>(input[offset + 2]) << 8)
        | static_cast<std::uint32_t>(input[offset + 3]);
}

std::uint64_t readUint64(const ByteBuffer& input, std::size_t offset) {
    std::uint64_t value = 0;
    for (std::size_t index = 0; index < 8; ++index) {
        value = (value << 8) | input[offset + index];
    }
    return value;
}

bool parseFixedHeader(const ByteBuffer& fixed, EncryptedChunkHeader& header, std::string& error) {
    if (fixed.size() != FixedHeaderSize) {
        error = "invalid encrypted chunk header size";
        return false;
    }

    for (std::size_t index = 0; index < Magic.size(); ++index) {
        if (fixed[index] != Magic[index]) {
            error = "invalid encrypted chunk header magic";
            return false;
        }
    }

    header.version = fixed[4];
    header.algorithm = fixed[5];
    header.kdf = fixed[6];
    header.flags = fixed[7];
    header.kdfIterations = readUint32(fixed, 8);
    header.saltSize = readUint16(fixed, 12);
    header.nonceSize = readUint16(fixed, 14);
    header.tagSize = readUint16(fixed, 16);
    header.plainSize = readUint64(fixed, 18);
    header.cipherSize = readUint64(fixed, 26);

    return validateEncryptedChunkHeader(header, error);
}

} // namespace

std::size_t EncryptedChunkHeader::serializedSize() const {
    return FixedHeaderSize + salt.size() + nonce.size();
}

std::uint64_t EncryptedChunkHeader::packetSize() const {
    return static_cast<std::uint64_t>(serializedSize()) + cipherSize + tagSize;
}

std::size_t encryptedChunkFixedHeaderSize() {
    return FixedHeaderSize;
}

ByteBuffer serializeEncryptedChunkHeader(const EncryptedChunkHeader& header) {
    ByteBuffer output;
    output.reserve(header.serializedSize());

    output.insert(output.end(), Magic.begin(), Magic.end());
    output.push_back(header.version);
    output.push_back(header.algorithm);
    output.push_back(header.kdf);
    output.push_back(header.flags);
    appendUint32(output, header.kdfIterations);
    appendUint16(output, header.saltSize);
    appendUint16(output, header.nonceSize);
    appendUint16(output, header.tagSize);
    appendUint64(output, header.plainSize);
    appendUint64(output, header.cipherSize);
    output.insert(output.end(), header.salt.begin(), header.salt.end());
    output.insert(output.end(), header.nonce.begin(), header.nonce.end());

    return output;
}

bool parseEncryptedChunkPacket(
    const ByteBuffer& packet,
    EncryptedChunkHeader& header,
    ByteBuffer& headerBytes,
    ByteBuffer& ciphertext,
    ByteBuffer& tag,
    std::string& error
) {
    if (packet.size() < FixedHeaderSize) {
        error = "encrypted chunk is too small to contain a header";
        return false;
    }

    ByteBuffer fixed(packet.begin(), packet.begin() + FixedHeaderSize);
    if (!parseFixedHeader(fixed, header, error)) {
        return false;
    }

    const std::uint64_t headerSize = FixedHeaderSize + header.saltSize + header.nonceSize;
    const std::uint64_t packetSize = headerSize + header.cipherSize + header.tagSize;
    if (packetSize > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        error = "encrypted chunk packet is too large for this platform";
        return false;
    }
    if (packet.size() != static_cast<std::size_t>(packetSize)) {
        error = "encrypted chunk size does not match its header";
        return false;
    }

    const std::size_t saltOffset = FixedHeaderSize;
    const std::size_t nonceOffset = saltOffset + header.saltSize;
    const std::size_t cipherOffset = nonceOffset + header.nonceSize;
    const std::size_t tagOffset = cipherOffset + static_cast<std::size_t>(header.cipherSize);

    header.salt.assign(packet.begin() + saltOffset, packet.begin() + nonceOffset);
    header.nonce.assign(packet.begin() + nonceOffset, packet.begin() + cipherOffset);
    headerBytes.assign(packet.begin(), packet.begin() + cipherOffset);
    ciphertext.assign(packet.begin() + cipherOffset, packet.begin() + tagOffset);
    tag.assign(packet.begin() + tagOffset, packet.end());

    return true;
}

bool readEncryptedChunkHeader(
    std::istream& input,
    EncryptedChunkHeader& header,
    std::string& error
) {
    ByteBuffer fixed(FixedHeaderSize);
    input.read(reinterpret_cast<char*>(fixed.data()), static_cast<std::streamsize>(fixed.size()));
    if (input.gcount() != static_cast<std::streamsize>(fixed.size())) {
        error = "unable to read complete encrypted chunk header";
        return false;
    }

    if (!parseFixedHeader(fixed, header, error)) {
        return false;
    }

    header.salt.assign(header.saltSize, 0);
    input.read(reinterpret_cast<char*>(header.salt.data()), static_cast<std::streamsize>(header.salt.size()));
    if (input.gcount() != static_cast<std::streamsize>(header.salt.size())) {
        error = "unable to read encrypted chunk salt";
        return false;
    }

    header.nonce.assign(header.nonceSize, 0);
    input.read(reinterpret_cast<char*>(header.nonce.data()), static_cast<std::streamsize>(header.nonce.size()));
    if (input.gcount() != static_cast<std::streamsize>(header.nonce.size())) {
        error = "unable to read encrypted chunk nonce";
        return false;
    }

    return true;
}

bool validateEncryptedChunkHeader(
    const EncryptedChunkHeader& header,
    std::string& error
) {
    if (header.version != Version) {
        error = "unsupported encrypted chunk version";
        return false;
    }
    if (header.algorithm != AlgorithmAes256Gcm) {
        error = "unsupported encrypted chunk algorithm";
        return false;
    }
    if (header.kdf != KdfPbkdf2Sha256) {
        error = "unsupported encrypted chunk KDF";
        return false;
    }
    if (header.flags != 0) {
        error = "unsupported encrypted chunk flags";
        return false;
    }
    if (header.kdfIterations == 0) {
        error = "invalid encrypted chunk KDF iteration count";
        return false;
    }
    if (header.saltSize != SaltSize) {
        error = "invalid encrypted chunk salt size";
        return false;
    }
    if (header.nonceSize != NonceSize) {
        error = "invalid encrypted chunk nonce size";
        return false;
    }
    if (header.tagSize != TagSize) {
        error = "invalid encrypted chunk tag size";
        return false;
    }
    if (header.cipherSize != header.plainSize) {
        error = "AES-GCM chunk ciphertext size must match plaintext size";
        return false;
    }

    return true;
}

} // namespace vault::crypto
