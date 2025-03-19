#pragma once

#include <util/charset/wide.h>
#include <util/generic/hash.h>
#include <util/generic/vector.h>

struct THiliteMark;

struct TZonedString {
    enum EZoneName {
        ZONE_FIO,
        ZONE_MATCH,
        ZONE_MATCHED_PHONE,
        ZONE_PARABEG,
        ZONE_EXTSNIP,
        ZONE_TRASH,
        ZONE_SENTENCE,
        ZONE_MENU,
        ZONE_QUEST,
        ZONE_PHONES,
        ZONE_LINK,
        ZONE_TABLE_CELL,
        ZONE_UNKNOWN,
    };

    class TSpan {
    public:
        TWtringBuf Span;
        THashMap<TString, TUtf16String> Attrs;
    public:
        TSpan()
        {
        }

        explicit TSpan(const TWtringBuf &wtrBuf)
            : Span(wtrBuf)
        {
        }

        void AddAttr(const TString &key, const TUtf16String &value) {
            if (!key.empty() && !value.empty()) {
                Attrs[key] = value;
            }
        }

        const wchar16* operator~() const {
            return Span.data();
        }

        size_t operator+() const {
            return Span.size();
        }
    };

    typedef TVector<TSpan> TSpans;
    struct TZone {
        TSpans Spans;
        const THiliteMark* Mark;

        TZone()
          : Spans()
          , Mark()
        {
        }

        TZone(const TSpans& spans, const THiliteMark* mark)
          : Spans(spans)
          , Mark(mark)
        {
        }
    };

    typedef THashMap<int, TZone> TZones;

    TUtf16String String;
    TZones Zones;

    TZonedString()
      : String()
      , Zones()
    {
    }

    TZonedString(const TUtf16String& s)
      : String(s)
      , Zones()
    {
    }

    TZonedString(const TZonedString& s)
      : String(s.String)
      , Zones(s.Zones)
    {
        ShiftZonesSpans(String.data() - s.String.data());
    }

    TZonedString& operator=(const TZonedString& s) {
        if (this != &s) {
            String = s.String;
            Zones = s.Zones;
            ShiftZonesSpans(String.data() - s.String.data());
        }
        return *this;
    }

    void Normalize();

    TZonedString Substr(size_t ofs, size_t len, const TWtringBuf& addPrefix, const TWtringBuf& addSuffix) const;

    TZone& GetOrCreateZone(int zoneId, const THiliteMark* mark) {
        auto it = Zones.find(zoneId);
        if (it != Zones.end())
            return it->second;
        Zones.insert(std::make_pair(zoneId, TZone(TSpans(), mark)));
        return Zones[zoneId];
    }

    void ShiftZonesSpans(ptrdiff_t shift) {
        for (TZonedString::TZones::iterator it = Zones.begin(); it != Zones.end(); ++it) {
            for (TZonedString::TSpans::iterator jt = it->second.Spans.begin(); jt != it->second.Spans.end(); ++jt) {
                jt->Span = TWtringBuf(~*jt + shift, ~*jt + shift + +*jt);
            }
        }
    }

private:
    void Normalize(size_t ofs, size_t len);
};

inline bool LsBuf(const TZonedString::TSpan& a, const TZonedString::TSpan& b) {
    if (a.Span.data() != b.Span.data()) {
        return a.Span.data() < b.Span.data();
    }
    return a.Span.size() < b.Span.size();
}
