#include <nginx/modules/strm_packager/src/common/encrypting_buffer.h>
#include <util/stream/format.h>

namespace NStrm::NPackager {
    TEncryptingBufferWriter::TEncryptingBufferWriter(TRequestWorker& worker, const TString& key, const TString& iv,
                                                     ui64 skipBytes)
        : Request(worker)
        , CipherCtx(NULL)
        , Finalized(true) // so that the destructor does not free preliminarily
    {
        Request.LogDebug() << "EncryptingBufferWriter(): skip = " << skipBytes;
        Request.LogDebug() << "  key = " << HexText(TStringBuf(key));
        Request.LogDebug() << "  iv  = " << HexText(TStringBuf(iv));
        Y_ENSURE(key.length() == CipherBlockSize);
        Y_ENSURE(iv.length() == CipherBlockSize);

        CipherCtx = EVP_CIPHER_CTX_new();
        Y_ENSURE(CipherCtx != NULL);
        Finalized = false;

        // shift blocks via iv
        TString shifted = ShiftIv(iv, skipBytes);

        int code = EVP_EncryptInit(CipherCtx, EVP_aes_128_ctr(),
                                   (const unsigned char*)key.c_str(),
                                   (const unsigned char*)shifted.c_str());
        Y_ENSURE(code == 1);

        // shift remaining bytest if required
        ui64 offset = skipBytes % CipherBlockSize;
        if (offset > 0) {
            TBuffer in(offset);
            TBuffer out(offset + CipherBlockSize);
            int outLen = 0;
            EVP_EncryptUpdate(CipherCtx, (unsigned char*)out.Data(), &outLen, (unsigned char*)in.Data(), in.size());
            Y_ENSURE((ui64)outLen == offset);
        }
    }

    void TEncryptingBufferWriter::Prepare(ui64 len) {
        Writer.Prepare(len);
    }

    TBuffer& TEncryptingBufferWriter::Buffer() {
        if (!Finalized) {
            TBuffer out(CipherBlockSize);
            int outLen = 0;
            int code = EVP_EncryptFinal(CipherCtx, (unsigned char*)out.Data(), &outLen);
            Y_ENSURE(code == 1);

            Writer.WorkIO(out.Data(), outLen);
            EVP_CIPHER_CTX_cleanup(CipherCtx);
            Finalized = true;
        }
        return Writer.Buffer();
    }

    TEncryptingBufferWriter::~TEncryptingBufferWriter() {
        if (!Finalized) {
            EVP_CIPHER_CTX_cleanup(CipherCtx);
        }
    }

    TString TEncryptingBufferWriter::ShiftIv(const TString& iv, ui64 skipBytes) {
        ui64 offset = skipBytes / CipherBlockSize;
        if (offset == 0) {
            return iv;
        }

        BigNum left = BN_bin2bn((const unsigned char*)iv.c_str(), iv.length(), NULL);
        BigNum right = BN_bin2bn((const unsigned char*)offset, sizeof(offset), NULL);

        BN_add(*left, *left, *right);

        // The sum may take more bytes than cipher block size so we copy it into
        // a temporary buffer and then extract only the required bytes
        TBuffer tmp(BN_num_bytes(*left));
        BN_bn2bin(*left, (unsigned char*)tmp.Data());

        TString result;
        result.resize(CipherBlockSize);
        std::move(tmp.Data() + tmp.size() - CipherBlockSize, tmp.Data() + tmp.size(), result.begin());
        return result;
    }

    BigNum::BigNum(BIGNUM* value)
        : Value(value)
    {
        Y_ENSURE(Value != NULL);
    }

    BigNum::~BigNum() {
        if (Value != NULL) {
            BN_free(Value);
        }
    }

    BIGNUM* BigNum::operator*() const {
        return Value;
    }
}
