#pragma once

#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/stream/input.h>
#include <util/stream/output.h>

class TCompressorFactory {
public:
    enum EFormat {
        ZLIB_DEFAULT = 0        /* "zlib" */,
        ZLIB_MINIMAL = 1        /* "zlib1" */,
        ZLIB_UNCOMPRESSED = 2   /* "zlib0" */,

        GZIP_DEFAULT = 10       /* "gzip" */,

        LZ_LZ4 = 20             /* "lz4" */,
        LZ_SNAPPY = 21          /* "snappy" */,
        LZ_MINILZO = 22         /* "minilz" */,
        LZ_FASTLZ = 23          /* "fastlz" */,
        LZ_QUICKLZ = 24         /* "quicklz" */,

        NO_COMPRESSION = 30     /* "uncompressed" */,

        BC_BEGIN = 100,
        BC_ZSTD_08_1 = 101      /* "BC_zstd08_1" */,

        MAX_COMPRESSION = 255      // all values should not exceed byte.
    };

    static void Compress(IOutputStream* out, TStringBuf data, EFormat format);
    static TString Decompress(IInputStream* in);
    static void Decompress(IInputStream* in, TString& out);

    static TString CompressToString(TStringBuf data, EFormat);
    static TString DecompressFromString(TStringBuf data);
    static void DecompressFromString(TStringBuf data, TString& out);

private:
    static THolder<IOutputStream> MakeCompressor(IOutputStream* out, EFormat format);
    static THolder<IInputStream> MakeDecompressor(IInputStream* in, EFormat format);
    static EFormat ReadCompressionMark(IInputStream* in);
};

const TString& ToString(TCompressorFactory::EFormat x);
bool FromString(const TString& name, TCompressorFactory::EFormat& ret);
