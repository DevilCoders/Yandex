#pragma once

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/memory/blob.h>
#include <util/generic/strbuf.h>

#include "common.h"

/** \file codecs.h
 *
 * Omnidex data "codecs" i.e. encoders/decoders are located here
 *
 * Each codec consists of 3 parts:
 * 1. Stat collector - algorithm to train compression model (table)
 * 2. Compressor - algorithm to compress data using specified compression model
 * 3. Decompressor - algorithm to decompress data using specified compression model
 *
 * every codec has a name which could be specified in omnidex json scheme. see examples
 *
 */

namespace NOmni {
    class IStatCollector {
    public:
        IStatCollector();
        void AddStat(TStringBuf data);
        TBlob BuildTable();
        virtual ~IStatCollector() {
        }

    protected:
        virtual TBlob DoBuildTable() = 0;
        virtual void DoAddStat(TStringBuf data) = 0;

    private:
        bool Done;
    };

    class ICompressor {
    public:
        virtual void Compress(TStringBuf data, TVector<char>* dst) = 0;
        virtual ~ICompressor() {
        }
    };

    class IDecompressor {
    public:
        virtual void Decompress(TStringBuf data, TVector<char>* dst) = 0;
        virtual ~IDecompressor() {
        }
    };

    IStatCollector* CreateCollectorByType(ECodecType codecType);
    ICompressor* CreateCompressorByType(ECodecType codecType, TBlob table);
    IDecompressor* CreateDecompressorByType(ECodecType codecType, TBlob table);

}
