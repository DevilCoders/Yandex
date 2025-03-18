#include "tel_schemes.h"

#include <util/string/split.h>
#include <util/string/strip.h>
#include <util/stream/mem.h>
#include <util/generic/strbuf.h>
#include <util/generic/yexception.h>
#include <util/string/cast.h>
////
//  TAreaScheme
////

TAreaScheme::TAreaScheme()
    : CountryOffset(0)
    , CountryLength(0)
    , AreaOffset(0)
    , AreaLength(0)
    , Weak(false)
{
}

TAreaScheme::TAreaScheme(ui8 countryOffset, ui8 countryLength, ui8 areaOffset, ui8 areaLength, bool weak)
    : CountryOffset(countryOffset)
    , CountryLength(countryLength)
    , AreaOffset(areaOffset)
    , AreaLength(areaLength)
    , Weak(weak)
{
}

////
//  TPhoneSchemes
////

TPhoneSchemes::TPhoneSchemes() {
    InitDefaultSchemes();
}

TPhoneSchemes::TPhoneSchemes(const THashMap<TString, TAreaScheme>& schemes) {
    Set(schemes);
}

bool TPhoneSchemes::IsGood(const TStringBuf& pattern) const {
    return Schemes.Find(pattern);
}

bool TPhoneSchemes::TryGetScheme(const TStringBuf& pattern, TAreaScheme& scheme) const {
    return Schemes.Find(pattern, &scheme);
}

static std::pair<TString, TAreaScheme> ParseScheme(const TStringBuf& line) {
    static const size_t minFields = 5;

    TVector<TStringBuf> fields;
    StringSplitter(line).Split('\t').AddTo(&fields);

    if (fields.size() < minFields)
        ythrow yexception() << "TPhoneSchemes: fields number is " << fields.size() << Endl;

    return std::make_pair(ToString(fields[0]),
                          TAreaScheme(
                              FromString<int>(fields[1]),
                              FromString<int>(fields[2]),
                              FromString<int>(fields[3]),
                              FromString<int>(fields[4]),
                              fields.size() > 5 && FromString<int>(fields[5]) != 0));
}

void TPhoneSchemes::ReadSchemes(IInputStream& in) {
    THashMap<TString, TAreaScheme> schemes;

    TString line;
    while (in.ReadLine(line)) {
        if (!StripInPlace(line).empty()) {
            schemes.insert(ParseScheme(line));
        }
    }

    Set(schemes);
}

void TPhoneSchemes::InitDefaultSchemes() {
// Include a list of statically defined schemes
// static const TString defaultSchemes = "";
#include "default_schemes.inc"

    constexpr TStringBuf buf = defaultSchemes;
    TMemoryInput in(buf.data(), buf.size());
    ReadSchemes(in);
}

void TPhoneSchemes::Set(const THashMap<TString, TAreaScheme>& schemes) {
    TSchemeTrie::TBuilder builder;
    for (const auto& scheme : schemes) {
        builder.Add(scheme.first, scheme.second);
    }
    TBufferOutput out;
    CompactTrieMinimize(out, builder);
    Schemes.Init(TBlob::FromBuffer(out.Buffer()));
}
