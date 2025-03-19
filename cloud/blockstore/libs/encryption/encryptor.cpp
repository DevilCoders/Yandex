#include "encryptor.h"

#include <cloud/storage/core/libs/common/error.h>
#include <cloud/storage/core/libs/keyring/keyring.h>

#include <contrib/libs/openssl/include/openssl/err.h>
#include <contrib/libs/openssl/include/openssl/evp.h>

#include <openssl/sha.h>

#include <util/generic/utility.h>
#include <util/string/builder.h>
#include <util/system/file.h>
#include <util/system/sanitizers.h>

namespace NCloud::NBlockStore {

namespace {

////////////////////////////////////////////////////////////////////////////////

TString ComputeSHA384Hash(const TString& encryptionKey, ui32 encryptionMode)
{
    SHA512_CTX ctx;
    SHA384_Init(&ctx);
    SHA384_Update(&ctx, &encryptionMode, sizeof(encryptionMode));
    ui32 keySize = encryptionKey.size();
    SHA384_Update(&ctx, &keySize, sizeof(keySize));
    SHA384_Update(&ctx, encryptionKey.data(), encryptionKey.size());

    TString hash;
    hash.resize(SHA384_DIGEST_LENGTH);
    SHA384_Final(reinterpret_cast<unsigned char*>(hash.Detach()), &ctx);
    return hash;
}

////////////////////////////////////////////////////////////////////////////////

class TAesXtsEncryptor
    : public IEncryptor
{
    using TEvpCipherCtxPtr =
        std::unique_ptr<EVP_CIPHER_CTX, decltype(&::EVP_CIPHER_CTX_free)>;

private:
    unsigned char Key[EVP_MAX_KEY_LENGTH] = {0};

public:
    TAesXtsEncryptor(const TString& key)
    {
        Y_VERIFY_DEBUG(EVP_MAX_KEY_LENGTH == key.size());
        memcpy(Key, key.Data(), Min<ui32>(key.size(), EVP_MAX_KEY_LENGTH));
    }

    ~TAesXtsEncryptor()
    {
        SecureZero(Key, EVP_MAX_KEY_LENGTH);
    }

    bool Encrypt(
        const TBlockDataRef& srcRef,
        const TBlockDataRef& dstRef,
        ui64 blockIndex) override
    {
        if (srcRef.Size() != dstRef.Size()) {
            return false;
        }

        if (srcRef.Data() == nullptr) {
            return false;
        }

        auto src = reinterpret_cast<const unsigned char*>(srcRef.Data());
        auto dst = reinterpret_cast<unsigned char*>(const_cast<char*>(dstRef.Data()));
        unsigned char iv[EVP_MAX_IV_LENGTH] = {0};
        memcpy(iv, &blockIndex, sizeof(blockIndex));

        auto ctx = TEvpCipherCtxPtr(EVP_CIPHER_CTX_new(), ::EVP_CIPHER_CTX_free);

        auto res = EVP_EncryptInit_ex(ctx.get(), EVP_aes_256_xts(), NULL, Key, iv);
        if (res != 1) {
            return false;
        }

        int len = 0;
        res = EVP_EncryptUpdate(ctx.get(), dst, &len, src, srcRef.Size());
        if (res != 1) {
            return false;
        }

        size_t totalLen = len;
        res = EVP_EncryptFinal_ex(ctx.get(), dst + len, &len);
        if (res != 1) {
            return false;
        }

        totalLen += len;
        if (totalLen != srcRef.Size()) {
            return false;
        }

        NSan::Unpoison(dstRef.Data(), dstRef.Size());
        return true;
    }

    bool Decrypt(
        const TBlockDataRef& srcRef,
        const TBlockDataRef& dstRef,
        ui64 blockIndex) override
    {
        if (srcRef.Size() != dstRef.Size()) {
            return false;
        }

        if (srcRef.Data() == nullptr) {
            memset(const_cast<char*>(dstRef.Data()), 0, srcRef.Size());
            return true;
        }

        auto src = reinterpret_cast<const unsigned char*>(srcRef.Data());
        auto dst = reinterpret_cast<unsigned char*>(const_cast<char*>(dstRef.Data()));
        unsigned char iv[EVP_MAX_IV_LENGTH] = {0};
        memcpy(iv, &blockIndex, sizeof(blockIndex));

        auto ctx = TEvpCipherCtxPtr(EVP_CIPHER_CTX_new(), ::EVP_CIPHER_CTX_free);

        auto res = EVP_DecryptInit_ex(ctx.get(), EVP_aes_256_xts(), NULL, Key, iv);
        if (res != 1) {
            return false;
        }

        int len = 0;
        res = EVP_DecryptUpdate(ctx.get(), dst, &len, src, srcRef.Size());
        if (res != 1) {
            return false;
        }

        size_t totalLen = len;
        res = EVP_DecryptFinal_ex(ctx.get(), dst + len, &len);
        if (res != 1) {
            return false;
        }

        totalLen += len;
        if (totalLen != srcRef.Size()) {
            return false;
        }

        NSan::Unpoison(dstRef.Data(), dstRef.Size());
        return true;
    }
};

////////////////////////////////////////////////////////////////////////////////

class TCaesarEncryptor
    : public IEncryptor
{
private:
    const size_t Shift;

public:
    TCaesarEncryptor(size_t shift)
        : Shift(shift)
    {}

    bool Encrypt(
        const TBlockDataRef& srcRef,
        const TBlockDataRef& dstRef,
        ui64 blockIndex) override
    {
        if (srcRef.Size() != dstRef.Size()) {
            return false;
        }

        if (srcRef.Data() == nullptr) {
            return false;
        }

        const char* src = srcRef.Data();
        char* dst = const_cast<char*>(dstRef.Data());
        for (size_t i = 0; i < dstRef.Size(); ++i) {
            dst[i] = src[i] + Shift + blockIndex;
        }

        return true;
    }

    bool Decrypt(
        const TBlockDataRef& srcRef,
        const TBlockDataRef& dstRef,
        ui64 blockIndex) override
    {
        if (srcRef.Size() != dstRef.Size()) {
            return false;
        }

        if (srcRef.Data() == nullptr) {
            memset(const_cast<char*>(dstRef.Data()), 0, srcRef.Size());
            return true;
        }

        const char* src = srcRef.Data();
        char* dst = const_cast<char*>(dstRef.Data());
        for (size_t i = 0; i < dstRef.Size(); ++i) {
            dst[i] = src[i] - Shift - blockIndex;
        }

        return true;
    }
};

////////////////////////////////////////////////////////////////////////////////

class TEncryptionKey
{
private:
    TString Key;

public:
    TEncryptionKey(TString key = "")
        : Key(std::move(key))
    {}

    ~TEncryptionKey()
    {
        SecureZero(Key.begin(), Key.Size());
    }

    const TString& Get() const
    {
        return Key;
    }
};

////////////////////////////////////////////////////////////////////////////////

TEncryptionKey ReadKeyFromKMS(NProto::TKmsSpec kmsSpec, ui32 expectedLength)
{
    Y_UNUSED(kmsSpec);
    Y_UNUSED(expectedLength);

    // TODO:
    ythrow TServiceError(E_NOT_IMPLEMENTED)
        << "Getting DEK from KMS is not implemented yet";
}

TEncryptionKey ReadKeyFromKeyring(ui32 keyringId, ui32 expectedLength)
{
    auto keyring = TKeyring::Create(keyringId);

    if (keyring.GetValueSize() != expectedLength) {
        ythrow TServiceError(E_ARGUMENT)
            << "Key from keyring " << keyringId
            << " should has size " << expectedLength;
    }

    return TEncryptionKey(keyring.GetValue());
}

TEncryptionKey ReadKeyFromFile(TString filePath, ui32 expectedLength)
{
    TFile file(filePath, EOpenModeFlag::OpenExisting | EOpenModeFlag::RdOnly);

    if (file.GetLength() != expectedLength) {
        ythrow TServiceError(E_ARGUMENT)
            << "Key file " << filePath.Quote()
            << " size " << file.GetLength() << " != " << expectedLength;
    }

    TString key = TString::TUninitialized(expectedLength);
    auto size = file.Read(key.begin(), expectedLength);
    if (size != expectedLength) {
        ythrow TServiceError(E_ARGUMENT)
            << "Read " << size << " bytes from key file "
            << filePath.Quote() << ", expected " << expectedLength;
    }

    return TEncryptionKey(std::move(key));
}

TEncryptionKey GetEncryptionKey(NProto::TKeyPath keyPath, ui32 expectedLength)
{
    if (keyPath.HasKmsSpec()) {
        return ReadKeyFromKMS(keyPath.GetKmsSpec(), expectedLength);
    } else if (keyPath.HasKeyringId()) {
        return ReadKeyFromKeyring(keyPath.GetKeyringId(), expectedLength);
    } else if (keyPath.HasFilePath()) {
        return ReadKeyFromFile(keyPath.GetFilePath(), expectedLength);
    } else if (keyPath.HasDEK()) {
        return TEncryptionKey(keyPath.GetDEK());
    } else {
        ythrow TServiceError(E_ARGUMENT)
            << "KeyPath should contain path to encryption key";
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

TResultOrError<IEncryptorPtr> CreateAesXtsEncryptor(
    const NProto::TKeyPath& encryptionKeyPath)
{
    return SafeExecute<TResultOrError<IEncryptorPtr>>([&] {
        auto key = GetEncryptionKey(encryptionKeyPath, EVP_MAX_KEY_LENGTH);
        IEncryptorPtr encryptor = std::make_shared<TAesXtsEncryptor>(key.Get());
        return encryptor;
    });
}

IEncryptorPtr CreateTestCaesarEncryptor(size_t shift)
{
    return std::make_shared<TCaesarEncryptor>(shift);
}

TResultOrError<TString> ComputeEncryptionKeyHash(
    const NProto::TEncryptionKey& spec)
{
    switch (spec.GetMode())
    {
        case NProto::NO_ENCRYPTION:
            return TString();

        case NProto::ENCRYPTION_AES_XTS:
        case NProto::ENCRYPTION_TEST:
            return SafeExecute<TResultOrError<TString>>([&] {
                auto key = GetEncryptionKey(spec.GetKeyPath(), EVP_MAX_KEY_LENGTH);
                return ComputeSHA384Hash(key.Get(), spec.GetMode());
            });

        default:
            return TErrorResponse(
                E_ARGUMENT,
                TStringBuilder()
                    << "Unknown encryption mode: "
                    << static_cast<int>(spec.GetMode()));
    }
}

}   // namespace NCloud::NBlockStore
