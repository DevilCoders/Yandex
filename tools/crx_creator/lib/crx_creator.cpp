#include "crx_creator.h"

#include <tools/crx_creator/lib/proto/crx3.pb.h>

#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/evp.h>

#include <util/generic/buffer.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/memory/blob.h>
#include <util/stream/buffer.h>

#include <array>
#include <cstdint>

static const std::array<uint8_t, 8> Crx3Magic = {
    0x43, 0x72, 0x32, 0x34,
    0x03, 0x00, 0x00, 0x00
};

struct BioFree {
    static void Destroy(BIO* bio) {
        BIO_free(bio);
    }
};

struct EvpMdCtxFree {
    static void Destroy(EVP_MD_CTX* ctx) {
        EVP_MD_CTX_free(ctx);
    }
};

struct EvpPkeyFree {
    static void Destroy(EVP_PKEY* key) {
        EVP_PKEY_free(key);
    }
};

constexpr unsigned char CrxSignatureContext[] = "CRX3 SignedData";
const size_t CrxIdLength = 16;

TBlob GetCrx3HeaderRSA(const TBlob& pem, IInputStream& archive) {
    // Open PEM file
    THolder<BIO, BioFree> pemData{BIO_new_mem_buf(pem.Data(), pem.Size())};
    Y_ENSURE(pemData, "Failed to allocate BIO");
    THolder<EVP_PKEY, EvpPkeyFree> privateKey{PEM_read_bio_PrivateKey(pemData.Get(), nullptr, nullptr, nullptr)};
    Y_ENSURE(privateKey, "File to read private key");

    CrxFile::CrxFileHeader header;
    // Generate public key
    int publicKeyLen = i2d_PUBKEY(privateKey.Get(), nullptr);
    Y_ENSURE(publicKeyLen > 0, "Failed to serialize public key");
    TVector<unsigned char> publicKey(publicKeyLen);
    uint8_t* data = publicKey.data();
    publicKeyLen = i2d_PUBKEY(privateKey.Get(), &data);
    Y_ENSURE(publicKeyLen > 0, "Failed to serialize public key");
    publicKey.resize(publicKeyLen);

    // Generate Crx Id.
    std::array<unsigned char, SHA256_DIGEST_LENGTH> digest;
    SHA256(publicKey.data(), publicKey.size(), digest.data());
    CrxFile::SignedData signedData;
    signedData.SetCrxId(digest.data(), CrxIdLength);
    TString signedDataStr = signedData.SerializeAsString();
    header.SetSignedHeaderData(signedDataStr.c_str(), signedDataStr.size());

    // Sign

    // Create signer
    THolder<EVP_MD_CTX, EvpMdCtxFree> ctx{EVP_MD_CTX_create()};
    Y_ENSURE(ctx, "Failed to initialize signature");
    Y_ENSURE(EVP_DigestSignInit(ctx.Get(), nullptr, EVP_sha256(), nullptr, privateKey.Get()), "Failed to initialize signature");

    // Hash data
    Y_ENSURE(EVP_DigestUpdate(ctx.Get(), CrxSignatureContext, std::size(CrxSignatureContext)), "Failed to hash data");

    size_t signedDataSize = signedDataStr.size();
    std::array<uint8_t, 4> signedDataSizeBuffer;
    for (size_t i = 0; i < 4; i++) {
        signedDataSizeBuffer[i] = signedDataSize & 255;
        signedDataSize >>= 8;
    }
    Y_ENSURE(EVP_DigestUpdate(ctx.Get(), signedDataSizeBuffer.data(), signedDataSizeBuffer.size()), "Failed to hash data");

    Y_ENSURE(EVP_DigestUpdate(ctx.Get(), signedDataStr.c_str(), signedDataStr.size()), "Failed to hash data");

    {
        const size_t bufferSize = 1 << 16;
        TArrayHolder<char> readBuffer(new char[bufferSize]);
        while (true) {
            size_t actualSize = archive.Read(readBuffer.Get(), bufferSize);
            if (actualSize == 0) {
                break;
            }
            Y_ENSURE(EVP_DigestUpdate(ctx.Get(), readBuffer.Get(), actualSize), "Failed to hash data");
        }
    }

    // Calculate signature
    size_t sigLen = 0;
    Y_ENSURE(EVP_DigestSignFinal(ctx.Get(), nullptr, &sigLen), "Failed to create signature");
    TVector<uint8_t> sig(sigLen);
    Y_ENSURE(EVP_DigestSignFinal(ctx.Get(), sig.data(), &sigLen), "Failed to create signature");
    sig.resize(sigLen);

    // Save signature
    auto& signature = *header.AddSha256WithRsa();
    signature.SetPublicKey(publicKey.data(), publicKey.size());
    signature.SetSignature(sig.data(), sig.size());

    size_t const defaultSize = 1024;
    TBuffer buffer(defaultSize);
    TBufferOutput output(buffer);
    output.Write(Crx3Magic.data(), Crx3Magic.size());

    size_t headerSize = header.ByteSizeLong();
    for (size_t i = 0; i < 4; i++) {
        output << (unsigned char) (headerSize & 255);
        headerSize >>= 8;
    }
    output << header.SerializeAsString();
    return TBlob::FromBuffer(buffer);
}
