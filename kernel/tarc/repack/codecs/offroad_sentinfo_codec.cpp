#include "offroad_codecs.h"

namespace NRepack {

TConstArrayRef<TArchiveTextBlockSentInfo> TOffroadSentInfoCodec::GetInfos(TStringBuf in) {
    return {reinterpret_cast<const TArchiveTextBlockSentInfo*>(in.Data()), in.Size() / sizeof(TArchiveTextBlockSentInfo)};
}

ui8 TOffroadSentInfoCodec::Encode(TStringBuf in, TBuffer& out) const {
    TBufferOutput output(out);
    TWriter writer(Table_.Get(), &output);

    for (const TArchiveTextBlockSentInfo& info : GetInfos(in)) {
        writer.WriteHit(info);
    }
    writer.Finish();

    return 0;
}

void TOffroadSentInfoCodec::Decode(TStringBuf in, TBuffer& out) const {
    TReader reader(Table_.Get(), TArrayRef<const char>(in.Data(), in.Size()));

    TBufferOutput output(out);
    while (true) {
        TArchiveTextBlockSentInfo hit;
        if (reader.ReadHit(&hit)) {
            output.Write(&hit, sizeof(hit));
        } else {
            break;
        }
    }
}

void TOffroadSentInfoCodec::DoLearn(NCodecs::ISequenceReader& in) {
    TSampler sampler;

    TStringBuf data;
    while (in.NextRegion(data)) {
        if (!data) {
            continue;
        }

        for (const TArchiveTextBlockSentInfo& info : GetInfos(data)) {
            sampler.WriteHit(info);
        }
        sampler.FinishBlock();
    }

    Model_ = sampler.Finish();
    Table_ = NOffroad::NewMultiTable(Model_);
}

void TOffroadSentInfoCodec::Save(IOutputStream* out) const {
    Model_.Save(out);
}

void TOffroadSentInfoCodec::Load(IInputStream* in) {
    Model_.Load(in);
    Table_ = NOffroad::NewMultiTable(Model_);
}

}
