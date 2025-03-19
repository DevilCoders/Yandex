#include <contrib/libs/openssl/include/openssl/err.h>
#include <contrib/libs/openssl/include/openssl/evp.h>
#include <contrib/libs/openssl/include/openssl/rand.h>
#include <ydb/core/blobstorage/crypto/chacha_vec.h>
#include <ydb/core/blobstorage/crypto/poly1305_vec.h>
#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/logger/filter.h>
#include <library/cpp/logger/global/global.h>

#include <util/generic/scope.h>
#include <util/string/split.h>
#include <util/system/thread.h>

#ifdef _linux_
#include <sched.h>
#endif

struct TWorkerResult {
    int EncryptMbPerSec;
    int DecryptMbPerSec;
};

enum class EAlgorithm {
    // OpenSSL implementations
    AES_128_CTR,
    AES_256_CTR,
    AES_128_GCM,
    AES_256_GCM,
    CHACHA20_POLY1305,
    // Kikimr implementations
    CHACHA8,
    CHACHA8_POLY1305,
};

using uchar = unsigned char;

// NOTE: None of the following code is a proper use of OpenSSL API!

TString GetOpenSSLError() {
    BIO* bio = BIO_new(BIO_s_mem());
    Y_DEFER {
        BIO_free(bio);
    };
    ERR_print_errors(bio);
    char* buf = nullptr;
    size_t len = BIO_get_mem_data(bio, &buf);
    return TString(buf, len);
}

void InitKeyAndNonce(EAlgorithm algorithm, std::vector<uchar>* key, std::vector<uchar>* nonce) {
    switch (algorithm) {
        case EAlgorithm::AES_128_CTR:
            key->resize(16);
            nonce->resize(16);
            break;
        case EAlgorithm::AES_128_GCM:
            key->resize(16);
            nonce->resize(12);
            break;
        case EAlgorithm::AES_256_CTR:
            key->resize(32);
            nonce->resize(16);
            break;
        case EAlgorithm::AES_256_GCM:
            key->resize(32);
            nonce->resize(12);
            break;
        case EAlgorithm::CHACHA20_POLY1305:
            key->resize(32);
            nonce->resize(12);
            break;
        case EAlgorithm::CHACHA8:
        case EAlgorithm::CHACHA8_POLY1305:
            key->resize(32);
            nonce->resize(8);
            break;
    }
    if (RAND_bytes(key->data(), key->size()) != 1) {
        throw yexception() << "Failed RAND_bytes: " << GetOpenSSLError();
    }
    if (RAND_bytes(nonce->data(), nonce->size()) != 1) {
        throw yexception() << "Failed RAND_bytes: " << GetOpenSSLError();
    }
}

size_t TagSizeForAlgorithm(EAlgorithm algorithm) {
    switch (algorithm) {
        case EAlgorithm::AES_128_CTR:
        case EAlgorithm::AES_256_CTR:
        case EAlgorithm::CHACHA8:
            return 0;
        case EAlgorithm::AES_128_GCM:
        case EAlgorithm::AES_256_GCM:
        case EAlgorithm::CHACHA20_POLY1305:
        case EAlgorithm::CHACHA8_POLY1305:
            return 16;
    }
}

void SetThisThreadCpuNum(int cpuNum) {
#ifdef _linux_
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(cpuNum, &mask);
    Y_ENSURE(sched_setaffinity(0, sizeof(mask), &mask) == 0);
#else
    Y_UNUSED(cpuNum);
#endif
}

class OpenSSLCipher {
public:
    OpenSSLCipher(EAlgorithm algorithm)
        : Algorithm(algorithm) {
        EncryptCtx = EVP_CIPHER_CTX_new();
        DecryptCtx = EVP_CIPHER_CTX_new();
    }

    ~OpenSSLCipher() {
        EVP_CIPHER_CTX_free(EncryptCtx);
        EVP_CIPHER_CTX_free(DecryptCtx);
    }

    EAlgorithm GetAlgorithm() const {
        return Algorithm;
    }

    void SetKey(std::vector<uchar> const& key) {
        Key = key;
    }

    void SetNonce(std::vector<uchar> const& nonce) {
        Nonce = nonce;
    }

    void EncryptStart() {
        if (EVP_CIPHER_CTX_reset(EncryptCtx) != 1) {
            throw yexception() << "Failed EVP_CIPHER_CTX_reset: " << GetOpenSSLError();
        }
        EVP_EncryptInit_ex(EncryptCtx, EVPCipherForAlgorithm(Algorithm), nullptr, Key.data(), Nonce.data());
        ValidateCtxInit(EncryptCtx);
    }

    void EncryptUpdate(uchar* ciphertext, uchar const* plaintext, size_t size) {
        int outl = 0;
        if (EVP_EncryptUpdate(EncryptCtx, ciphertext, &outl, plaintext, size) != 1) {
            throw yexception() << "Failed EVP_EncryptUpdate: " << GetOpenSSLError();
        }
        if (outl != (int)size) {
            throw yexception() << "Mismatch after EVP_EncryptUpdate: " << outl << " vs " << size;
        }
    }

    void EncryptFinish(uchar* ciphertext, size_t remainingSize) {
        uchar buf[128];
        int outl = 0;
        if (EVP_EncryptFinal_ex(EncryptCtx, buf, &outl) != 1) {
            throw yexception() << "Failed EVP_DecryptFinal_ex: " << GetOpenSSLError();
        }
        if ((size_t)outl != 0) {
            throw yexception() << "EVP_DecryptFinal_ex tried to write " << outl
                               << " bytes, should be zero (wrong block size)";
        }
        size_t tagSize = TagSizeForAlgorithm(Algorithm);
        if (tagSize != remainingSize) {
            throw yexception() << "Should have " << tagSize << " remaining ciphertext, got " << remainingSize;
        }
        if (tagSize != 0) {
            if (EVP_CIPHER_CTX_ctrl(EncryptCtx, EVP_CTRL_GCM_GET_TAG, tagSize, ciphertext + outl) != 1) {
                throw yexception() << "Failed EVP_CIPHER_CTX_ctrl(EVP_CTRL_GCM_GET_TAG): "
                                   << GetOpenSSLError();
            }
        }
    }

    void DecryptStart() {
        if (EVP_CIPHER_CTX_reset(DecryptCtx) != 1) {
            throw yexception() << "Failed EVP_CIPHER_CTX_reset: " << GetOpenSSLError();
        }
        EVP_DecryptInit_ex(DecryptCtx, EVPCipherForAlgorithm(Algorithm), nullptr, Key.data(), Nonce.data());
        ValidateCtxInit(DecryptCtx);
    }

    void DecryptUpdate(uchar* plaintext, uchar const* ciphertext, size_t size) {
        int outl = 0;
        if (EVP_DecryptUpdate(DecryptCtx, plaintext, &outl, ciphertext, size) != 1) {
            throw yexception() << "Failed EVP_DecryptUpdate: " << GetOpenSSLError();
        }
        if (outl != (int)size) {
            throw yexception() << "Mismatch after EVP_DecryptUpdate: " << outl << " vs " << size;
        }
    }

    void DecryptFinish(uchar const* ciphertext, size_t remainingSize) {
        size_t tagSize = TagSizeForAlgorithm(Algorithm);
        if (remainingSize != tagSize) {
            throw yexception() << "Should have " << tagSize << " remaining ciphertext, got " << remainingSize;
        }
        if (tagSize > 0) {
            if (EVP_CIPHER_CTX_ctrl(DecryptCtx, EVP_CTRL_GCM_SET_TAG, tagSize, (void*)ciphertext) != 1) {
                throw yexception() << "Failed EVP_CIPHER_CTX_ctrl(EVP_CTRL_GCM_SET_TAG): "
                                   << GetOpenSSLError();
            }
        }

        uchar buf[128];
        int outl = 0;
        if (EVP_DecryptFinal_ex(DecryptCtx, buf, &outl) != 1) {
            throw yexception() << "Failed EVP_DecryptFinal_ex: " << GetOpenSSLError();
        }
        if (outl != 0) {
            throw yexception() << outl << " remaining plaintext (wrong block size)";
        }
    }

private:
    EAlgorithm Algorithm;
    EVP_CIPHER_CTX* EncryptCtx;
    EVP_CIPHER_CTX* DecryptCtx;
    std::vector<uchar> Key;
    std::vector<uchar> Nonce;

    static EVP_CIPHER const* EVPCipherForAlgorithm(EAlgorithm algorithm) {
        switch (algorithm) {
            case EAlgorithm::AES_128_CTR:
                return EVP_aes_128_ctr();
            case EAlgorithm::AES_256_CTR:
                return EVP_aes_256_ctr();
            case EAlgorithm::AES_128_GCM:
                return EVP_aes_128_gcm();
            case EAlgorithm::AES_256_GCM:
                return EVP_aes_256_gcm();
            case EAlgorithm::CHACHA20_POLY1305:
                return EVP_chacha20_poly1305();
            case EAlgorithm::CHACHA8:
            case EAlgorithm::CHACHA8_POLY1305:
                Y_ENSURE(false);
        }
    }

    void ValidateCtxInit(EVP_CIPHER_CTX* ctx) const {
        if (EVP_CIPHER_CTX_key_length(ctx) != (int)Key.size()) {
            throw yexception() << "Key sizes mismatch: " << EVP_CIPHER_CTX_key_length(ctx)
                               << " vs " << Key.size();
        }
        if (EVP_CIPHER_CTX_iv_length(ctx) != (int)Nonce.size()) {
            throw yexception() << "Nonce sizes mismatch: " << EVP_CIPHER_CTX_iv_length(ctx)
                               << " vs " << Nonce.size();
        }
    }
};

class ChaCha8Cipher {
public:
    ChaCha8Cipher(bool withPoly1305)
        : WithPoly1305(withPoly1305) {
    }

    EAlgorithm GetAlgorithm() const {
        if (WithPoly1305) {
            return EAlgorithm::CHACHA8_POLY1305;
        } else {
            return EAlgorithm::CHACHA8;
        }
    }

    void SetKey(std::vector<uchar> const& key) {
        Y_ENSURE(ChaChaVec::KEY_SIZE == key.size());
        Key = key;
    }

    void SetNonce(std::vector<uchar> const& nonce) {
        Y_ENSURE(sizeof(ChaChaVec::NonceType) == nonce.size());
        Nonce = nonce;
    }

    void EncryptStart() {
        ReInit();
    }

    void EncryptUpdate(uchar* ciphertext, uchar const* plaintext, size_t size) {
        ChaChaVec.Encipher(plaintext, ciphertext, size);
        if (WithPoly1305) {
            Poly1305Vec.Update(ciphertext, size);
        }
    }

    void EncryptFinish(uchar* ciphertext, size_t remainingSize) {
        size_t tagSize = TagSize();
        if (remainingSize != tagSize) {
            throw yexception() << "Should have " << tagSize << " remaining ciphertext, got " << remainingSize;
        }
        if (WithPoly1305) {
            Poly1305Vec.Finish(ciphertext);
        }
    }

    void DecryptStart() {
        ReInit();
    }

    void DecryptUpdate(uchar* plaintext, uchar const* ciphertext, size_t size) {
        if (WithPoly1305) {
            Poly1305Vec.Update(ciphertext, size);
        }
        ChaChaVec.Decipher(ciphertext, plaintext, size);
    }

    void DecryptFinish(uchar const* ciphertext, size_t remainingSize) {
        size_t tagSize = TagSize();
        if (remainingSize != tagSize) {
            throw yexception() << "Should have " << tagSize << " remaining ciphertext, got " << remainingSize;
        }
        if (WithPoly1305) {
            uchar buf[16];
            Poly1305Vec.Finish(buf);
            if (memcmp(buf, ciphertext, sizeof(buf)) != 0) {
                throw yexception() << "Mismatched authentication tag";
            }
        }
    }

private:
    bool WithPoly1305;
    ChaChaVec ChaChaVec;
    Poly1305Vec Poly1305Vec;
    std::vector<uchar> Key;
    std::vector<uchar> Nonce;

    void ReInit() {
        ChaChaVec.SetKey(Key.data(), Key.size());
        ChaChaVec.SetIV(Nonce.data());
        if (WithPoly1305) {
            // Use the first ciphertext stream block to generate the Poly1305 key. Encipher/Decipher are
            // equivalent for ChaChaVec, so this is safe for bothe Encrypt and Decrypt.
            uchar zeroBlock[Poly1305Vec::KEY_SIZE];
            memset(zeroBlock, 0, Poly1305Vec::KEY_SIZE);
            uchar polyKey[Poly1305Vec::KEY_SIZE];
            ChaChaVec.Encipher(zeroBlock, polyKey, Poly1305Vec::KEY_SIZE);
            Poly1305Vec.SetKey(polyKey, Poly1305Vec::KEY_SIZE);
        }
    }

    size_t TagSize() const {
        return WithPoly1305 ? 16 : 0;
    }
};

template <typename Cipher>
void DoEncryptDecrypt(Cipher& cipher, int threadNum, size_t blockSize, size_t numBlocks, TDuration waitFor,
                      TWorkerResult* result) {
    std::vector<uchar> key;
    std::vector<uchar> nonce;
    InitKeyAndNonce(cipher.GetAlgorithm(), &key, &nonce);
    cipher.SetKey(key);
    cipher.SetNonce(nonce);

    size_t tagSize = TagSizeForAlgorithm(cipher.GetAlgorithm());
    std::vector<uchar> plaintext;
    plaintext.resize(blockSize * numBlocks);
    std::vector<uchar> ciphertext;
    ciphertext.resize(blockSize * numBlocks + tagSize);

    if (RAND_bytes(plaintext.data(), plaintext.size()) != 1) {
        throw yexception() << "Failed RAND_bytes: " << GetOpenSSLError();
    }
    char const kMagicFirst = 0x13;
    char const kMagicLast = 0x37;
    plaintext[0] = kMagicFirst;
    plaintext[plaintext.size() - 1] = kMagicLast;

    INFO_LOG << "Thread #" << threadNum << " doing encrypts\n";
    // TInstant is not monotonic, but we do not care.
    TInstant startTime = TInstant::Now();
    for (uint64_t numIters = 0;; numIters++) {
        if (numIters % 128 == 0) {
            TDuration delta = TInstant::Now() - startTime;
            if (delta >= waitFor) {
                result->EncryptMbPerSec = numIters * numBlocks * blockSize * 1000 / (delta.MilliSeconds() * 1024 * 1024);
                break;
            }
        }

        cipher.EncryptStart();
        for (size_t b = 0; b < numBlocks; b++) {
            cipher.EncryptUpdate(ciphertext.data() + b * blockSize, plaintext.data() + b * blockSize,
                                 blockSize);
        }
        cipher.EncryptFinish(ciphertext.data() + numBlocks * blockSize, tagSize);
    }

    INFO_LOG << "Thread #" << threadNum << " doing decrypts\n";
    startTime = TInstant::Now();
    for (uint64_t numIters = 0;; numIters++) {
        if (numIters % 128 == 0) {
            TDuration delta = TInstant::Now() - startTime;
            if (delta >= waitFor) {
                result->DecryptMbPerSec = numIters * numBlocks * blockSize * 1000 / (delta.MilliSeconds() * 1024 * 1024);
                break;
            }
        }

        cipher.DecryptStart();
        for (size_t b = 0; b < numBlocks; b++) {
            cipher.DecryptUpdate(plaintext.data() + b * blockSize, ciphertext.data() + b * blockSize,
                                 blockSize);
        }
        cipher.DecryptFinish(ciphertext.data() + numBlocks * blockSize, tagSize);
        Y_ENSURE(plaintext[0] == kMagicFirst);
        Y_ENSURE(plaintext[plaintext.size() - 1] == kMagicLast);
    }
}

void WorkerMain(int threadNum, int cpuNum, EAlgorithm algorithm, size_t blockSize, size_t numBlocks,
                int waitForSec, TWorkerResult* result) {
    if (cpuNum != -1) {
        INFO_LOG << "Assigning thread #" << threadNum << " to CPU #" << cpuNum << "\n";
        SetThisThreadCpuNum(cpuNum);
    }
    TDuration waitFor = TDuration::Seconds(waitForSec);

    if (algorithm == EAlgorithm::CHACHA8 || algorithm == EAlgorithm::CHACHA8_POLY1305) {
        ChaCha8Cipher cipher(algorithm == EAlgorithm::CHACHA8_POLY1305);
        DoEncryptDecrypt(cipher, threadNum, blockSize, numBlocks, waitFor, result);
    } else {
        OpenSSLCipher cipher(algorithm);
        DoEncryptDecrypt(cipher, threadNum, blockSize, numBlocks, waitFor, result);
    }
}

EAlgorithm AlgorithmFromString(TString const& str) {
    if (str == "aes-128-ctr") {
        return EAlgorithm::AES_128_CTR;
    } else if (str == "aes-256-ctr") {
        return EAlgorithm::AES_256_CTR;
    } else if (str == "aes-128-gcm") {
        return EAlgorithm::AES_128_GCM;
    } else if (str == "aes-256-gcm") {
        return EAlgorithm::AES_256_GCM;
    } else if (str == "chacha20-poly1305") {
        return EAlgorithm::CHACHA20_POLY1305;
    } else if (str == "chacha8") {
        return EAlgorithm::CHACHA8;
    } else if (str == "chacha8-poly1305") {
        return EAlgorithm::CHACHA8_POLY1305;
    } else {
        throw yexception() << "Unknown algorithm " << str;
    }
}

std::vector<int> ParseCpus(TString const& str) {
    std::vector<int> ret;
    for (auto const& it : StringSplitter(str).Split(',')) {
        if (!it.Token().empty()) {
            ret.push_back(FromString<int>(it.Token()));
        }
    }
    return ret;
}

int main(int argc, char** argv) {
    DoInitGlobalLog("console", TLOG_DEBUG, false, false);
    ERR_load_crypto_strings();

    NLastGetopt::TOpts opts;
    opts.AddLongOption("algorithm", "algorithm to use, one of aes-128-ctr, aes-256-ctr,"
                                    " aes-128-gcm, aes-256-gcm, chacha20-poly1305, chacha8")
        .DefaultValue("aes-128-ctr");
    opts.AddLongOption("block-size", "size of block to encrypt")
        .DefaultValue("8192");
    opts.AddLongOption("num-blocks", "number of blocks to encrypt in one iteration, affects the working set size")
        .DefaultValue("4");
    opts.AddLongOption("threads", "number of threads doing encryption/decryption")
        .DefaultValue("1");
    opts.AddLongOption("cpus", "list of comma-separate CPU numbers for each (e.g. 0,5,14)")
        .DefaultValue("");
    opts.AddLongOption("wait-for", "wait for seconds before stopping iteration")
        .DefaultValue("3");
    opts.AddHelpOption();
    opts.SetFreeArgsNum(0);

    int numThreads = 0;
    TString algorithmStr;
    EAlgorithm algorithm;
    size_t blockSize = 0;
    size_t numBlocks = 0;
    std::vector<int> threadCpus;
    int waitForSec = 0;
    try {
        NLastGetopt::TOptsParseResult result(&opts, argc, argv);
        numThreads = FromString(result.Get("threads"));
        algorithmStr = result.Get("algorithm");
        algorithm = AlgorithmFromString(algorithmStr);
        blockSize = FromString(result.Get("block-size"));
        numBlocks = FromString(result.Get("num-blocks"));
        threadCpus = ParseCpus(result.Get("cpus"));
        // Fill the remaining CPUs with -1.
        threadCpus.resize(numThreads, -1);
        waitForSec = FromString(result.Get("wait-for"));
    } catch (const yexception& ex) {
        DEBUG_LOG << ex.what();
        opts.PrintUsage("benchcrypto");
        return 1;
    }

    INFO_LOG << "Starting " << numThreads << " workers for algorithm " << algorithmStr
             << " with block size " << blockSize << " and working set " << blockSize * numBlocks * 2
             << "\n";
    TVector<std::unique_ptr<TThread>> workers;
    TVector<TWorkerResult> results;
    results.resize(numThreads);
    for (int i = 0; i < numThreads; i++) {
        workers.emplace_back(new TThread([=, &results]() {
            WorkerMain(i, threadCpus[i], algorithm, blockSize, numBlocks, waitForSec, &results[i]);
        }));
    }
    for (auto& worker : workers) {
        worker->Start();
    }
    for (auto& worker : workers) {
        worker->Join();
    }

    int minEncryptMbPerSec = results[0].EncryptMbPerSec;
    int maxEncryptMbPerSec = results[0].EncryptMbPerSec;
    int minDecryptMbPerSec = results[0].DecryptMbPerSec;
    int maxDecryptMbPerSec = results[0].DecryptMbPerSec;
    int64_t avgEncryptMbPerSec = results[0].EncryptMbPerSec;
    int64_t avgDecryptMbPerSec = results[0].DecryptMbPerSec;
    for (int i = 1; i < numThreads; i++) {
        minEncryptMbPerSec = Min(minEncryptMbPerSec, results[i].EncryptMbPerSec);
        maxEncryptMbPerSec = Max(maxEncryptMbPerSec, results[i].EncryptMbPerSec);
        minDecryptMbPerSec = Min(minDecryptMbPerSec, results[i].DecryptMbPerSec);
        maxDecryptMbPerSec = Max(maxDecryptMbPerSec, results[i].DecryptMbPerSec);
        avgEncryptMbPerSec += results[i].EncryptMbPerSec;
        avgDecryptMbPerSec += results[i].DecryptMbPerSec;
    }
    avgEncryptMbPerSec /= numThreads;
    avgDecryptMbPerSec /= numThreads;
    INFO_LOG << "Encrypt: " << minEncryptMbPerSec << "-" << maxEncryptMbPerSec
             << "(" << avgEncryptMbPerSec << ")"
             << "Mb/sec, decrypt: " << minDecryptMbPerSec << "-" << maxDecryptMbPerSec
             << "(" << avgDecryptMbPerSec << ")"
             << "Mb/sec\n";
}
