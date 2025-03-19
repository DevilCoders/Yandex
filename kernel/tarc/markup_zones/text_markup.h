#pragma once

#include <kernel/tarc/iface/tarcface.h>

#include <library/cpp/charset/doccodes.h>
#include <library/cpp/charset/wide.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <util/charset/wide.h>
#include <util/generic/vector.h>
#include <util/system/yassert.h>
#include <util/system/defaults.h>

struct TTextZoneSpan {
    ui64 CharBeg; // Character offset of the zone start in the full doc text (including title)
    ui64 CharEnd; // Character offset of the zone end in the full doc text (including title)
    ui64 ByteBeg; // Byte offset of the zone start in the utf-8 encoded text
    ui64 ByteEnd; // Byte offset of the zone end in the utf-8 encoded text

    TTextZoneSpan()
        : CharBeg(0)
        , CharEnd(0)
        , ByteBeg(0)
        , ByteEnd(0)
    {
    }
};

struct TTextZone {
    ui32 ZoneId;
    TVector<TTextZoneSpan> Spans;
};

struct TTextMarkupZones {
    // low-weighted zone has AZ_COUNT id;
    // high-weighted zone has AZ_COUNT+1 id;
    // best-weighted zone has AZ_COUNT+2 id;
    TVector<TTextZone> Zones;

    TTextMarkupZones() {
        Zones.resize(AZ_COUNT + 3);
        for (size_t i = 0; i < Zones.size(); i++) {
            Zones[i].ZoneId = (ui32)i;
        }
    }

    inline const TTextZone& GetZone(ui32 id) const {
        Y_ASSERT(id < AZ_COUNT);
        return Zones[id];
    }

    inline TTextZone& GetZone(ui32 id) {
        Y_ASSERT(id < AZ_COUNT);
        return Zones[id];
    }

};

void GetTextMarkupZones(const ui8* data, TTextMarkupZones& zones, bool wZone = false);


inline TString UnescapeAttribute(const char* from, size_t len) {
    return Base64Decode(TStringBuf(from, len));
}

inline TString UnescapeAttribute(const TString& from) {
    return UnescapeAttribute(from.c_str(), from.size());
}

inline TString GetAttribute(const TUtf16String& attrs, const char* attr) {
    //\t<name>\t<value>\t<name>\t<value>
    //^marker  ^value  ^marker  ^value

    TUtf16String attrmarker;
    attrmarker.append('\t').append(ASCIIToWide(attr)).append('\t');

    size_t markerPos = attrs.find(attrmarker);

    if (TUtf16String::npos == markerPos)
        return TString();

    markerPos += attrmarker.size();

    size_t attrEnd = attrs.find('\t', markerPos);
    size_t n = TUtf16String::npos;

    if (TUtf16String::npos != attrEnd)
        n = attrEnd - markerPos;

    return UnescapeAttribute(WideToChar(attrs.substr(markerPos, n), CODES_YANDEX));
}

struct TArchiveSent
{
    ui16 Number;
    ui16 Flag;
    TUtf16String Text;
    TUtf16String OnlyText;
    TUtf16String OnlyAttrs;
public:
    TString GetSentAttribute(const char*attr) const {
        if (!(Flag & SENT_HAS_ATTRS))
            return TString();

        return GetAttribute(OnlyAttrs, attr);
    }
};

void UnpackMarkupZones(const void* markupInfo, size_t markupInfoLen, TArchiveMarkupZones* mZonesOut);

void GetSentencesByNumbers(const ui8* data, const TVector<int>& sentNumbers, TVector<TArchiveSent>* outSents, TArchiveWeightZones* wZones, bool needExtended = false);
void GetArchiveMarkupZones(const ui8* data, TArchiveMarkupZones* mZones);

void GetAllText(const ui8* data, class TBuffer* out);

