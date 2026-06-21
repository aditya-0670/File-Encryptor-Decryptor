# Secure Distributed File Vault

Phase 3 replaces the toy encryption with real authenticated encryption while keeping the Phase 2 module boundaries.

The app is still a local C++17 command-line program that encrypts or decrypts files in place. Writes are staged into temporary files and originals are replaced only after all chunks finish successfully, so decrypt failures do not replace encrypted input with partial plaintext.

## Cryptography

- Algorithm: AES-256-GCM via OpenSSL EVP.
- KDF: PBKDF2-HMAC-SHA256.
- KDF iterations: 100,000 per chunk.
- Salt: random 16 bytes per encrypted chunk.
- Nonce: random 12 bytes per encrypted chunk.
- Tag: 16-byte GCM authentication tag per chunk.
- Header authentication: encrypted chunk headers are supplied to GCM as AAD.

Decryption verifies the authentication tag before writing staged plaintext back over the source file. If ciphertext, header, password, nonce, salt, or tag data is wrong, decryption fails cleanly and the original encrypted file remains in place.

## Encrypted Chunk Format

Each encrypted chunk is stored as:

```text
fixed header | salt | nonce | ciphertext | auth tag
```

Fixed header fields:

```text
magic          4 bytes   "SDFV"
version        1 byte    1
algorithm      1 byte    1 = AES-256-GCM
kdf            1 byte    1 = PBKDF2-HMAC-SHA256
flags          1 byte    0
iterations     4 bytes   big-endian uint32
salt size      2 bytes   big-endian uint16
nonce size     2 bytes   big-endian uint16
tag size       2 bytes   big-endian uint16
plain size     8 bytes   big-endian uint64
cipher size    8 bytes   big-endian uint64
```

The chunker uses these headers during decrypt to discover encrypted packet boundaries before submitting jobs.

## Architecture

```text
main.cpp
`-- cli/
    `-- CliApplication
        |-- chunker/FileChunker
        |-- crypto/EncryptionEngine
        |   `-- Aes256GcmEncryptionEngine
        |-- manifest/VaultManifest
        |-- metadata/MetadataStore
        |   `-- InMemoryMetadataStore
        |-- storage/StorageAdapter
        |   `-- LocalFileStorageAdapter
        `-- jobs/JobQueue
```

## Modules

- `src/app/cli`: argument parsing, interactive prompts, validation orchestration, and application wiring.
- `src/app/core`: shared action parsing and common value helpers.
- `src/app/crypto`: encryption interface, AES-256-GCM implementation, and encrypted chunk header parsing.
- `src/app/chunker`: deterministic file discovery and chunk manifest creation for plaintext and encrypted files.
- `src/app/manifest`: file and chunk manifest model.
- `src/app/metadata`: manifest persistence boundary, currently in-memory.
- `src/app/storage`: staged local file reads/writes.
- `src/app/jobs`: queued chunk processing over storage and encryption interfaces.
- `src/app/fileHandling`: `.env` password loading.
- `src/app/encryptDecrypt`: compatibility wrapper for the legacy `cryption` helper target.

## Build

OpenSSL 3 is required. The default Makefile path is Homebrew's Apple Silicon location:

```bash
make
```

Override the OpenSSL prefix if needed:

```bash
make OPENSSL_PREFIX=/path/to/openssl
```

This builds:

- `encrypt_decrypt`: main CLI program.
- `cryption`: legacy task-level helper, now backed by the same AES-GCM flow.

Clean generated build files with:

```bash
make clean
```

## Configuration

Create a `.env` file in the directory where you run the executable:

```text
VAULT_PASSWORD=correct horse battery staple
```

`PASSWORD=...` is also accepted. `KEY=...` and a raw first-line value are accepted only as compatibility aliases; the value is treated as password text, not as a numeric encryption key.

## Usage

Command-line mode:

```bash
./encrypt_decrypt ./test encrypt
./encrypt_decrypt ./test decrypt
```

Interactive mode:

```bash
./encrypt_decrypt
```

The program modifies files in place after staging successful output. Use copies of important files until persistent metadata and recovery workflows are added.

## Dashboard

A static frontend dashboard with realistic sample vault telemetry is available at:

```text
dashboard/index.html
```

It shows backup overview, protected files, encrypted size, chunk count, dedupe ratio, job queue status, failed jobs, AI security findings, restore history, and throughput metrics. It is currently sample-data only and not wired to the C++ vault runtime.

## Tests

```bash
make test
```

The test target runs:

- `architecture_tests`: C++ checks for action parsing, AES-GCM round trip, header discovery, chunking, metadata, storage, job queue execution, and tamper failure.
- `test/run_roundtrip.sh`: CLI validation and encrypt/decrypt round-trip checks.

Temporary test data is written under `test/tmp*` and ignored by git.

## Phase 3 Scope

Completed in this phase:

- Replaced byte-shift encryption with AES-256-GCM.
- Added password-based key derivation.
- Added random salt, random nonce, and GCM tag handling.
- Added encrypted chunk packet headers.
- Added authenticated decrypt failure behavior.
- Added tests proving tampered ciphertext fails to decrypt and leaves the encrypted file in place.

Not included yet:

- Persistent manifest storage.
- Key rotation or password change workflows.
- Remote or distributed storage.
- Frontend or deployment.
