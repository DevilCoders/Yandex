#include "offroad_codecs.h"

namespace NRepack {

void TOffroadWeightZonesCodec::EncodeOneZone(const TArchiveZone& zone, IOutputStream* output, const TTable& tables) {
    TBufferOutput out;
    TWriter writer(tables.Get(), &out);

    for (const TArchiveZoneSpan& hit : zone.Spans) {
        writer.WriteHit(hit);
    }
    writer.Finish();

    ::Save(output, out.Buffer());
}

void TOffroadWeightZonesCodec::DecodeOneZone(IInputStream& in, TArchiveZone& zone, const TTable& tables) {
    TBuffer buf;
    ::Load(&in, buf);
    TReader reader(tables.Get(), TArrayRef<const char>(buf.Data(), buf.Size()));

    while (true) {
        TArchiveZoneSpan hit;
        if (reader.ReadHit(&hit)) {
            zone.Spans.push_back(hit);
        } else {
            break;
        }
    }
}

void TOffroadWeightZonesCodec::SampleOneZone(const TArchiveZone& zone, TSampler& sampler) {
    for (const auto& span : zone.Spans) {
        sampler.WriteHit(span);
    }
    sampler.FinishBlock();
}

ui8 TOffroadWeightZonesCodec::Encode(TStringBuf in, TBuffer& out) const {
    TArchiveWeightZones zones;

    TMemoryInput input(in.Data(), in.Size());
    zones.Load(&input);

    TBufferOutput output(out);
    EncodeOneZone(zones.LowZone,  &output, LowZoneTable_);
    EncodeOneZone(zones.HighZone, &output, HighZoneTable_);
    EncodeOneZone(zones.BestZone, &output, BestZoneTable_);

    return 0;
}

void TOffroadWeightZonesCodec::Decode(TStringBuf in, TBuffer& out) const {
    TArchiveWeightZones zones{};

    TMemoryInput input(in.Data(), in.Size());
    DecodeOneZone(input, zones.LowZone,  LowZoneTable_);
    DecodeOneZone(input, zones.HighZone, HighZoneTable_);
    DecodeOneZone(input, zones.BestZone, BestZoneTable_);

    TBufferOutput output(out);
    zones.Save(&output);
}

void TOffroadWeightZonesCodec::DoLearn(NCodecs::ISequenceReader& in) {
    TArchiveWeightZones zones;
    TSampler lowZoneSampler, highZoneSampler, bestZoneSampler;

    TStringBuf data;
    while (in.NextRegion(data)) {
        if (!data) {
            continue;
        }

        TMemoryInput input(data.Data(), data.Size());
        zones.Load(&input);

        SampleOneZone(zones.LowZone, lowZoneSampler);
        SampleOneZone(zones.HighZone, highZoneSampler);
        SampleOneZone(zones.BestZone, bestZoneSampler);
    }

    LowZoneModel_ = lowZoneSampler.Finish();
    HighZoneModel_ = highZoneSampler.Finish();
    BestZoneModel_ = bestZoneSampler.Finish();

    LowZoneTable_ = NOffroad::NewMultiTable(LowZoneModel_);
    HighZoneTable_ = NOffroad::NewMultiTable(HighZoneModel_);
    BestZoneTable_ = NOffroad::NewMultiTable(BestZoneModel_);
}

void TOffroadWeightZonesCodec::Save(IOutputStream* out) const {
    LowZoneModel_.Save(out);
    HighZoneModel_.Save(out);
    BestZoneModel_.Save(out);
}

void TOffroadWeightZonesCodec::Load(IInputStream* in) {
    LowZoneModel_.Load(in);
    HighZoneModel_.Load(in);
    BestZoneModel_.Load(in);

    LowZoneTable_ = NOffroad::NewMultiTable(LowZoneModel_);
    HighZoneTable_ = NOffroad::NewMultiTable(HighZoneModel_);
    BestZoneTable_ = NOffroad::NewMultiTable(BestZoneModel_);
}

}
