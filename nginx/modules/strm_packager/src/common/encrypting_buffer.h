#pragma once

extern "C" {
#include <ngx_event_openssl.h>
}

#include <nginx/modules/strm_packager/src/base/workers.h>
#include <strm/media/transcoder/mp4muxer/io.h>

namespace NStrm::NPackager {
    class TEncryptingBufferWriter: public NMP4Muxer::NIODetails::TWriterBase {
    public:
        TEncryptingBufferWriter(TRequestWorker& worker, const TString& key, const TString& iv, ui64 skipBytes);
        ~TEncryptingBufferWriter();

        template <size_t len, bool prepared = false>
        inline void WorkIO(void const* buf) {
            WorkIO<prepared>(buf, len);
        }

        template <bool prepared = false>
        inline void WorkIO(void const* buf, ui64 len) {
            Y_ENSURE(Finalized == false);

            Out.Reserve(len + CipherBlockSize);
            unsigned outLen = 0;
            int code = EVP_EncryptUpdate(CipherCtx, (unsigned char*)Out.Data(), (int*)&outLen,
                                         (const unsigned char*)buf, len);
            Y_ENSURE(code == 1);
            Y_ENSURE(outLen == len); // valid for CTR only
            Writer.WorkIO<prepared>(Out.Data(), outLen);
        }

        void Prepare(ui64 len);

        TBuffer& Buffer();

        static const ui64 CipherBlockSize = 16;

    private:
        static TString ShiftIv(const TString& iv, ui64 skipBytes);

        TRequestWorker& Request;
        NMP4Muxer::TBufferWriter Writer;
        EVP_CIPHER_CTX* CipherCtx;
        TBuffer Out;
        bool Finalized;
    };

    class BigNum {
    public:
        BigNum(BIGNUM* value);
        ~BigNum();

        BIGNUM* operator*() const;

    private:
        BIGNUM* Value;
    };
}
