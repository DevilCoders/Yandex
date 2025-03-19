#include "factory.h"

#include <library/cpp/blockcodecs/core/codecs.h>
#include <library/cpp/streams/lz/lz.h>
#include <util/generic/yexception.h>
#include <util/stream/str.h>
#include <util/stream/buffer.h>
#include <util/stream/zlib.h>
#include <util/stream/mem.h>
#include <util/ysaveload.h>

static const char DEFAULT_PROTOBUF_MARK = 'p';
static const char COMPRESSION_MARK = 'z';

static inline IOutputStream* PrependDefaultMark(IOutputStream* out) {
    ::Save<ui8>(out, DEFAULT_PROTOBUF_MARK);
    return out;
}

static inline IOutputStream* PrependCompressionMark(IOutputStream* out, TCompressorFactory::EFormat format) {
    Y_ASSERT(static_cast<size_t>(format) <= Max<ui8>());
    ::Save<ui8>(out, COMPRESSION_MARK);
    ::Save<ui8>(out, static_cast<ui8>(format));
    return out;
}

namespace {
class TCopyOutputStream : public IOutputStream {
public:
    TCopyOutputStream(IOutputStream* stream)
        : Slave(stream)
    { }
protected:
    void DoWrite(const void* buf, size_t len) override {
        Slave->Write(buf, len);
    }
private:
    IOutputStream* Slave;
};

class TCopyInputStream : public IInputStream {
public:
    TCopyInputStream(IInputStream* stream)
        : Slave(stream)
    { }
protected:
    size_t DoRead(void* buf, size_t len) override {
        return Slave->Read(buf, len);
    }
private:
    IInputStream* Slave;
};
} // namespace

template <typename TCompr>
static inline IOutputStream* MakeLzCompressor(IOutputStream* out, TCompressorFactory::EFormat format) {
    return new TCompr(PrependCompressionMark(out, format));
}

static const NBlockCodecs::ICodec& GetBlockCodec(TCompressorFactory::EFormat format) {
    return *NBlockCodecs::Codec(TStringBuf(ToString(format)).Skip(TStringBuf("BC_").size()));
}

void TCompressorFactory::Compress(IOutputStream* out, TStringBuf data, EFormat format) {
    if (BC_BEGIN <= format) {
        PrependCompressionMark(out, format)->Write(GetBlockCodec(format).Encode(data));
    } else {
        MakeCompressor(out, format)->Write(data);
    }
}

TString TCompressorFactory::Decompress(IInputStream* in) {
    TString result;
    Decompress(in, result);
    return result;
}

void TCompressorFactory::Decompress(IInputStream* in, TString& out) {
    EFormat format = ReadCompressionMark(in);
    if (BC_BEGIN <= format) {
        GetBlockCodec(format).Decode(in->ReadAll(), out);
    } else {
        TStringOutput streamOut(out);
        MakeDecompressor(in, format)->ReadAll(streamOut);
    }
}

TString TCompressorFactory::CompressToString(TStringBuf data, EFormat fmt) {
    TString result;
    result.reserve(data.size());
    {
        TStringOutput sout(result);
        Compress(&sout, data, fmt);
    }
    return result;
}

TString TCompressorFactory::DecompressFromString(TStringBuf data) {
    TString result;
    DecompressFromString(data, result);
    return result;
}

void TCompressorFactory::DecompressFromString(TStringBuf data, TString& out) {
    TMemoryInput minp(data.data(), data.size());
    return Decompress(&minp, out);
}

THolder<IOutputStream> TCompressorFactory::MakeCompressor(IOutputStream* out, EFormat format) {
    switch (format) {
        case ZLIB_DEFAULT:      return MakeHolder<TZLibCompress>(PrependDefaultMark(out), ZLib::ZLib);
        case ZLIB_MINIMAL:      return MakeHolder<TZLibCompress>(PrependDefaultMark(out), ZLib::ZLib, 1);
        case ZLIB_UNCOMPRESSED: return MakeHolder<TZLibCompress>(PrependDefaultMark(out), ZLib::ZLib, 0);

        case GZIP_DEFAULT:      return MakeHolder<TZLibCompress>(PrependCompressionMark(out, format), ZLib::GZip);

        case LZ_LZ4:            return THolder<IOutputStream>(MakeLzCompressor<TLz4Compress>(out, format));
        case LZ_SNAPPY:         return THolder<IOutputStream>(MakeLzCompressor<TSnappyCompress>(out, format));
        case LZ_MINILZO:        return THolder<IOutputStream>(MakeLzCompressor<TLzoCompress>(out, format));
        case LZ_FASTLZ:         return THolder<IOutputStream>(MakeLzCompressor<TLzfCompress>(out, format));
        case LZ_QUICKLZ:        return THolder<IOutputStream>(MakeLzCompressor<TLzqCompress>(out, format));

        case NO_COMPRESSION:    return MakeHolder<TCopyOutputStream>(PrependCompressionMark(out, format));

        default: break;
    }
    ythrow yexception() << "Unknown compression method (" << size_t(format) << ")";
}

THolder<IInputStream> TCompressorFactory::MakeDecompressor(IInputStream* in, TCompressorFactory::EFormat format) {
    // selecting a proper decompressor is much easier
    switch (format) {
        case ZLIB_DEFAULT:
        case ZLIB_MINIMAL:
        case ZLIB_UNCOMPRESSED:
        case GZIP_DEFAULT:
            return MakeHolder<TZLibDecompress>(in);

        case LZ_LZ4:
        case LZ_SNAPPY:
        case LZ_MINILZO:
        case LZ_FASTLZ:
        case LZ_QUICKLZ:
            return OpenLzDecompressor(in);

        case NO_COMPRESSION:
            return MakeHolder<TCopyInputStream>(in);

        default: break;
    }

    ythrow yexception() << "Unknown compression method (" << size_t(format) << ")";
}

TCompressorFactory::EFormat TCompressorFactory::ReadCompressionMark(IInputStream* in) {
    ui8 mark = 0;
    ::Load<ui8>(in, mark);

    if (mark == DEFAULT_PROTOBUF_MARK) {
        return ZLIB_DEFAULT;
    } else if (mark == COMPRESSION_MARK) {
        ui8 fmt = 0;
        ::Load<ui8>(in, fmt);
        return static_cast<EFormat>(fmt);
    } else {
        ythrow yexception() << "Unsupported compression format.";
    }
}
