#pragma once

#include <util/generic/buffer.h>
#include <util/stream/mem.h>

#include <library/cpp/codecs/codecs.h>

#include <kernel/tarc/repack/codecs/flatbuffers/repack_info.fbs.h>

namespace NRepack {
    class TCodec {
    public:
        struct DictsData {
            TString mkupModel;
            TString zoneModel;
            TString textModel;
            TString sentModel;
        };

        TCodec(const DictsData& data);

        TBuffer EncodeMarkup(TStringBuf data) {
            return Encode(data, MarkupCodec_);
        }
        TBuffer EncodeWeightZones(TStringBuf data) {
            return Encode(data, WeightZonesCodec_);
        }
        TBuffer EncodeSentences(TStringBuf data) {
            return Encode(data, SentencesCodec_);
        }
        TBuffer EncodeBlocks(TStringBuf data) {
            return Encode(data, BlocksCodec_);
        }

        TBuffer DecodeMarkup(TStringBuf data) {
            return Decode(data, MarkupCodec_);
        }
        TBuffer DecodeWeightZones(TStringBuf data) {
            return Decode(data, WeightZonesCodec_);
        }
        TBuffer DecodeSentences(TStringBuf data) {
            return Decode(data, SentencesCodec_);
        }
        TBuffer DecodeBlocks(TStringBuf data) {
            return Decode(data, BlocksCodec_);
        }
    
    private:
        TBuffer Encode(TStringBuf data, NCodecs::TCodecConstPtr codec) {
            TBuffer out;
            Y_ENSURE(codec->Encode(data, out) == 0);
            return out;
        }

        TBuffer Decode(TStringBuf data, NCodecs::TCodecConstPtr codec) {
            TBuffer out;
            codec->Decode(data, out);
            return out;
        }

        NCodecs::TCodecConstPtr MarkupCodec_;
        NCodecs::TCodecConstPtr WeightZonesCodec_;
        NCodecs::TCodecConstPtr SentencesCodec_;
        NCodecs::TCodecConstPtr BlocksCodec_;
    };

    struct TCodecWithInfo {
        TCodecWithInfo(TStringBuf ver);
        TCodecWithInfo(const TCodec::DictsData& data);

        static TCodecWithInfo GetLatest();
        
        NFbs::TCodecInfoT info;
        TCodec& codec;
    };
}
