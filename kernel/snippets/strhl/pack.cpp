#include "pack.h"
#include "zonedstring.h"
#include <kernel/snippets/strhl/zonedstring.pb.h>

#include <util/charset/wide.h>

void Pack(const TZonedString& zs, NProto::TZonedString& proto) {
    proto.SetString(WideToUTF8(zs.String));
    const wchar16* start = zs.String.data();
    for (const auto& it1 : zs.Zones) {
        NProto::TZone& zone = *proto.AddZones();
        zone.SetZoneId(it1.first);
        const auto& spans = it1.second.Spans;
        for (const auto& span : spans) {
            NProto::TSpan& protoSpan = *zone.AddSpans();
            protoSpan.SetOffset(span.Span.data() - start);
            protoSpan.SetLen(span.Span.size());
            for (const auto& it2 : span.Attrs) {
                NProto::TAttr& attr = *protoSpan.AddAttrs();
                attr.SetName(it2.first);
                attr.SetValue(WideToUTF8(it2.second));
            }
        }
    }
}

void Unpack(const NProto::TZonedString& proto, TZonedString& zs) {
    zs = TZonedString(UTF8ToWide(proto.GetString()));
    const wchar16* start = zs.String.data();
    for (size_t i = 0; i < proto.ZonesSize(); ++i) {
        const auto& protoZone = proto.GetZones(i);
        int zoneId = protoZone.GetZoneId();
        auto& zone = zs.GetOrCreateZone(zoneId, nullptr);
        for (size_t j = 0; j < protoZone.SpansSize(); ++j) {
            const auto& protoSpan = protoZone.GetSpans(j);
            zone.Spans.push_back(TZonedString::TSpan(TWtringBuf(start + protoSpan.GetOffset(), protoSpan.GetLen())));
            auto& span = zone.Spans.back();
            for (size_t k = 0; k < protoSpan.AttrsSize(); ++k) {
                const auto& protoAttr = protoSpan.GetAttrs(k);
                span.AddAttr(protoAttr.GetName(), UTF8ToWide(protoAttr.GetValue()));
            }
        }
    }
    zs.Normalize();
}
