#pragma once

/// @file compression.h - Utils for using YT compression and erasure coding.

#include <mapreduce/yt/interface/client.h>

#include <util/generic/string.h>

namespace NJupiter {
    /**
     * Compression codec description https://wiki.yandex-team.ru/yt/userdoc/compression/
     * Performance tests https://wiki.yandex-team.ru/yt/internal/compression/
     * Code generation enum to string https://wiki.yandex-team.ru/PoiskovajaPlatforma/Build/WritingCmakefiles/#generate-enum
     */
    enum class ECompressionCodec {
        NONE  /* "none" */,

        SNAPPY  /* "snappy" */,

        ZLIB_1  /* "zlib_1" */,
        ZLIB_2  /* "zlib_2" */,
        ZLIB_3  /* "zlib_3" */,
        ZLIB_4  /* "zlib_4" */,
        ZLIB_5  /* "zlib_5" */,
        ZLIB_6  /* "zlib_6" */,
        ZLIB_7  /* "zlib_7" */,
        ZLIB_8  /* "zlib_8" */,
        ZLIB_9  /* "zlib_9" */,

        LZ4  /* "lz4" */,
        LZ4_HIGH_COMPRESSION  /* "lz4_high_compression" */,

        QUICK_LZ  /* "quick_lz" */,

        ZSTD_1  /* "zstd_1" */,
        ZSTD_2  /* "zstd_2" */,
        ZSTD_3  /* "zstd_3" */,
        ZSTD_4  /* "zstd_4" */,
        ZSTD_5  /* "zstd_5" */,
        ZSTD_6  /* "zstd_6" */,
        ZSTD_7  /* "zstd_7" */,
        ZSTD_8  /* "zstd_8" */,
        ZSTD_9  /* "zstd_9" */,
        ZSTD_10  /* "zstd_10" */,
        ZSTD_11  /* "zstd_11" */,
        ZSTD_12  /* "zstd_12" */,
        ZSTD_13  /* "zstd_13" */,
        ZSTD_14  /* "zstd_14" */,
        ZSTD_15  /* "zstd_15" */,
        ZSTD_16  /* "zstd_16" */,
        ZSTD_17  /* "zstd_17" */,
        ZSTD_18  /* "zstd_18" */,
        ZSTD_19  /* "zstd_19" */,
        ZSTD_20  /* "zstd_20" */,
        ZSTD_21  /* "zstd_21" */,

        BROTLI_1  /* "brotli_1" */,
        BROTLI_2  /* "brotli_2" */,
        BROTLI_3  /* "brotli_3" */,
        BROTLI_4  /* "brotli_4" */,
        BROTLI_5  /* "brotli_5" */,
        BROTLI_6  /* "brotli_6" */,
        BROTLI_7  /* "brotli_7" */,
        BROTLI_8  /* "brotli_8" */,
        BROTLI_9  /* "brotli_9" */,
        BROTLI_10  /* "brotli_10" */,
        BROTLI_11  /* "brotli_11" */,

        GZIP /* "gzip_best_compression" */,
    };

    enum class EErasureCodec {
        NONE  /* "none" */,

        REED_SOLOMON_6_3  /* "reed_solomon_6_3" */,

        LRC_12_2_2  /* "lrc_12_2_2" */,
        ISA_LRC_12_2_2  /* "isa_lrc_12_2_2" */,
    };

    /** Returns string representation of the codec */
    TString GetCompressionCodecName(ECompressionCodec codec);
    /** Returns string representation of the codec */
    TString GetErasureCodecName(EErasureCodec codec);

    /** Returns compression stat attribute of a table */
    NYT::TNode::TMapType GetCompressionStats(NYT::IClientBasePtr client, const TString& path);
    /** Returns compression codec setting for the table */
    ECompressionCodec GetCompressionCodecAttr(NYT::IClientBasePtr client, const TString& path);
    /** Returns the ACTUAL codec currently used for the table data. Fails if different chunks use different compression */
    ECompressionCodec GetCompressionCodec(NYT::IClientBasePtr client, const TString& path);

    /** Returns erasure codec stat attribute of a table */
    NYT::TNode::TMapType GetErasureStats(NYT::IClientBasePtr client, const TString& path);
    /** Returns the erasure codec setting for the table*/
    EErasureCodec GetErasureCodecAttr(NYT::IClientBasePtr client, const TString& path);
    /** Returns the ACTUAL erasure codec currently used for the table data. Fails if different chunks use different codecs */
    EErasureCodec GetErasureCodec(const NYT::TNode& node);
    /** Returns the ACTUAL erasure codec currently used for the table data. Fails if different chunks use different codecs */
    EErasureCodec GetErasureCodec(NYT::IClientBasePtr client, const TString& path);

    /** Sets compression codec for the table. Note, that it will only apply the next time data is written to the table. */
    void SetCompressionCodec(NYT::IClientBasePtr client, const TString& path, ECompressionCodec codec);
    /** Sets erasure codec for the table. Note, that it will only apply the next time data is written to the table. */
    void SetErasureCodec(NYT::IClientBasePtr client, const TString& path, EErasureCodec codec);

}
