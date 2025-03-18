#include "compress.h"

#include <library/cpp/blockcodecs/codecs.h>

#include <util/generic/singleton.h>

using namespace NBlockCodecs;

namespace {
    struct TCoder {
        inline TCoder()
            : F(Codec("lz4-fast-safe"))
            , S(Codec("lz4-hc-safe"))
        {
        }

        inline TString Compress(const TStringBuf& s) const {
            return S->Encode(F->Encode(s));
        }

        inline TString Decompress(const TStringBuf& s) const {
            return F->Decode(S->Decode(s));
        }

        static inline const TCoder& Instance() {
            return *Singleton<TCoder>();
        }

        const ICodec* F;
        const ICodec* S;
    };
}

TString NXmlRPC::CompressXml(const TStringBuf& s) {
    return TCoder::Instance().Compress(s);
}

TString NXmlRPC::DecompressXml(const TStringBuf& s) {
    return TCoder::Instance().Decompress(s);
}
