#include "compressor.h"
#include <kernel/multipart_archive/config/config.h>

#include <library/cpp/streams/lz/lz.h>
#include <library/cpp/streams/lzma/lzma.h>
#include <library/cpp/logger/global/global.h>

namespace {

    template<class T>
    inline static T IntCeil(T a, T b) {
        return a / b + (a % b ? 1 : 0);
    }

    inline static ui16 GetLz4BufferSize(size_t uncompressedDataSize) {
        if (!uncompressedDataSize)
            return 0;
        size_t count = IntCeil<size_t>(uncompressedDataSize, Max<ui16>());
        return IntCeil(uncompressedDataSize, count);
    }

}

namespace NRTYArchive {

    class TCompressorConfigChecker : public TMultipartConfig::IChecker {
    public:
        void Check(const TMultipartConfig& config) const override {
            if (config.Compression != IArchivePart::COMPRESSED)
                return;
            switch (config.CompressionParams.Algorithm) {
            case IArchivePart::TConstructContext::TCompressionParams::LZ4:
            break;
            case IArchivePart::TConstructContext::TCompressionParams::LZMA:
                if (config.CompressionParams.Level != config.CompressionParams.DEFAULT_LEVEL && config.CompressionParams.Level > 9)
                    ythrow yexception() << "CompressionLevel must be less than 9, your value is " << config.CompressionParams.Level;
            break;
            default:;
            }
        }
        static TMultipartConfig::TCheckerFactory::TRegistrator<TCompressorConfigChecker> Registartor;
    };

    TMultipartConfig::TCheckerFactory::TRegistrator<TCompressorConfigChecker> TCompressorConfigChecker::Registartor("TCompressorConfigChecker");

    TCompressor::TCompressor(const IArchivePart::TConstructContext& ctx)
        : Config(ctx.Compression)
    {}

    IOutputStream* TCompressor::CreateCompressStream(IOutputStream& slave, size_t dataSize) const {
        switch (Config.Algorithm) {
        case IArchivePart::TConstructContext::TCompressionParams::LZ4:
            return new TLz4Compress(&slave, GetLz4BufferSize(dataSize));
        case IArchivePart::TConstructContext::TCompressionParams::LZMA:
            if (Config.Level == Config.DEFAULT_LEVEL)
                return new TLzmaCompress(&slave);
            return new TLzmaCompress(&slave, Config.Level);
        default:;
        }
        FAIL_LOG("Invalid compression algorithm");
    }

    IInputStream* TCompressor::CreateDecompressStream(IInputStream& slave) const {
        switch (Config.Algorithm) {
        case IArchivePart::TConstructContext::TCompressionParams::LZ4:
            return new TLz4Decompress(&slave);
        case IArchivePart::TConstructContext::TCompressionParams::LZMA:
            return new TLzmaDecompress(&slave);
        default:;
        }
        FAIL_LOG("Invalid compression algorithm");
    }

}
