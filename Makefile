CXX = g++
OPENSSL_PREFIX ?= /opt/homebrew/opt/openssl@3
OPENSSL_CXXFLAGS ?= -I$(OPENSSL_PREFIX)/include
OPENSSL_LDFLAGS ?= -L$(OPENSSL_PREFIX)/lib
CXXFLAGS = -std=c++17 -g -Wall -I. -Isrc/app $(OPENSSL_CXXFLAGS)
LDFLAGS = $(OPENSSL_LDFLAGS)
LDLIBS = -lcrypto

MAIN_TARGET = encrypt_decrypt
CRYPTION_TARGET = cryption
ARCH_TEST_TARGET = architecture_tests

APP_SRC = src/app/chunker/FileChunker.cpp \
          src/app/cli/CliApplication.cpp \
          src/app/crypto/Aes256GcmEncryptionEngine.cpp \
          src/app/crypto/EncryptedChunkFormat.cpp \
          src/app/fileHandling/ReadEnv.cpp \
          src/app/jobs/JobQueue.cpp \
          src/app/manifest/Manifest.cpp \
          src/app/metadata/InMemoryMetadataStore.cpp \
          src/app/storage/LocalFileStorageAdapter.cpp

MAIN_SRC = main.cpp \
           $(APP_SRC)

CRYPTION_SRC = src/app/encryptDecrypt/CryptionMain.cpp \
               src/app/encryptDecrypt/Cryption.cpp \
               $(APP_SRC)

ARCH_TEST_SRC = test/architecture_tests.cpp \
                $(APP_SRC)

MAIN_OBJ = $(MAIN_SRC:.cpp=.o)
CRYPTION_OBJ = $(CRYPTION_SRC:.cpp=.o)
ARCH_TEST_OBJ = $(ARCH_TEST_SRC:.cpp=.o)

all: $(MAIN_TARGET) $(CRYPTION_TARGET)

test: all $(ARCH_TEST_TARGET)
	./$(ARCH_TEST_TARGET)
	./test/run_roundtrip.sh

$(MAIN_TARGET): $(MAIN_OBJ)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@

$(CRYPTION_TARGET): $(CRYPTION_OBJ)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@

$(ARCH_TEST_TARGET): $(ARCH_TEST_OBJ)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o src/app/*/*.o test/*.o $(MAIN_TARGET) $(CRYPTION_TARGET) $(ARCH_TEST_TARGET)

.PHONY: clean all test
