#include "wtypes.h"

#include <library/cpp/containers/dictionary/dictionary.h>
#include <util/generic/singleton.h>

namespace {

template <typename ENameId, ENameId UnkId, typename TDerived>
class TOntoNamesCollection: public TEnum2String<ENameId, TStringBuf, NNameIdDictionary::TSolidId2Str<ENameId, TStringBuf>, NNameIdDictionary::TCaseInsensitiveStr2Id<ENameId> > {
public:
    static TString ToString(ENameId id) {
        return TString{Default<TDerived>().Id2Name(id)};
    }

    static bool TryFromString(const TStringBuf& name, ENameId& id) {
        const ENameId* tmpId = Default<TDerived>().FindName(name);
        if (!tmpId)
            return false;
        id = *tmpId;
        return true;
    }

    static ENameId FromString(const TStringBuf& name) {
        ENameId id;
        return TryFromString(name, id) ? id : UnkId;
    }
};


class TOntoCatsCollection: public TOntoNamesCollection<EOntoCatsType, ONTO_UNKNOWN, TOntoCatsCollection> {
public:
    TOntoCatsCollection() {
        const std::pair<const char*, EOntoCatsType> names[] = {
            std::make_pair("unknown",     ONTO_UNKNOWN),
            std::make_pair("org",         ONTO_ORG),
            std::make_pair("geo",         ONTO_GEO),
            std::make_pair("hum",         ONTO_HUM),
            std::make_pair("soft",        ONTO_SOFT),
            std::make_pair("text",        ONTO_TEXT),
            std::make_pair("device",      ONTO_DEVICE),
            std::make_pair("music",       ONTO_MUSIC),
            std::make_pair("site",        ONTO_SITE),
            std::make_pair("film",        ONTO_FILM),
            std::make_pair("band",        ONTO_BAND),
            std::make_pair("auto",        ONTO_AUTO),
            std::make_pair("date",        ONTO_DATE),
            std::make_pair("disease",     ONTO_DISEASE),
            std::make_pair("event",       ONTO_EVENT),
            std::make_pair("drugs",       ONTO_DRUGS),
            std::make_pair("food",        ONTO_FOOD),
            std::make_pair("picture",     ONTO_PICTURE),
            std::make_pair("auto_parts",  ONTO_AUTO_PARTS),
            std::make_pair("clothes",     ONTO_CLOTHES),
            std::make_pair("furniture",   ONTO_FURNITURE),
            std::make_pair("toys",        ONTO_TOYS),
            std::make_pair("anim",        ONTO_ANIM),
            std::make_pair("accom",       ONTO_ACCOM),
            std::make_pair("trans",       ONTO_TRANS),
            std::make_pair("intent",      ONTO_INTENT),
            std::make_pair("intent_bit",  ONTO_VOTING_CONT),
            std::make_pair("beauty",      ONTO_BEAUTY)
        };

        static_assert(Y_ARRAY_SIZE(names) == ONTO_CATS_END, "expect Y_ARRAY_SIZE(names) == ONTO_CATS_END");
        Init(names, Y_ARRAY_SIZE(names));
    }
};

class TOntoIntsCollection: public TOntoNamesCollection<EOntoIntsType, ONTO_INTS_UNKNOWN, TOntoIntsCollection> {
public:
    TOntoIntsCollection() {
        const std::pair<const char*, EOntoIntsType> names[] = {
            std::make_pair("unknown", ONTO_INTS_UNKNOWN),
            std::make_pair("auto",    ONTO_INTS_AUTO),
            std::make_pair("auto_parts", ONTO_INTS_AUTO_PARTS),
            std::make_pair("device",  ONTO_INTS_DEVICE),
            std::make_pair("live",    ONTO_INTS_LIVE),
            std::make_pair("music",   ONTO_INTS_MUSIC),
            std::make_pair("play",    ONTO_INTS_PLAY),
            std::make_pair("text",    ONTO_INTS_TEXT),
            std::make_pair("sport",   ONTO_INTS_SPORT),
            std::make_pair("film",    ONTO_INTS_FILM),
            std::make_pair("imported",ONTO_INTS_IMPORTED),
            std::make_pair("job",     ONTO_INTS_JOB),
            std::make_pair("clothes",     ONTO_INTS_CLOTHES)
        };

        static_assert(Y_ARRAY_SIZE(names) == ONTO_INTS_END, "expect Y_ARRAY_SIZE(names) == ONTO_INTS_END");
        Init(names, Y_ARRAY_SIZE(names));
    }
};

class TSubcatCollection: public TEnum2String<TSubcat, TString, NNameIdDictionary::TSparseId2Str<TSubcat, TString>, NNameIdDictionary::TCaseInsensitiveStr2Id<TSubcat> > {
public:
    TSubcatCollection() {
        const std::pair<const char*, TSubcat> names[] = {
            std::make_pair("real_estate", TSubcat(ONTO_ACCOM, ONTO_INTS_LIVE)),
            std::make_pair("sportevent",  TSubcat(ONTO_EVENT, ONTO_INTS_SPORT)),
            std::make_pair("actor",       TSubcat(ONTO_HUM, ONTO_INTS_FILM)),
            std::make_pair("occupation",  TSubcat(ONTO_HUM, ONTO_INTS_JOB)),
            std::make_pair("musician",    TSubcat(ONTO_HUM, ONTO_INTS_MUSIC)),
            std::make_pair("sportsman",   TSubcat(ONTO_HUM, ONTO_INTS_SPORT)),
            std::make_pair("writer",      TSubcat(ONTO_HUM, ONTO_INTS_TEXT)),
            std::make_pair("carmaker",    TSubcat(ONTO_ORG, ONTO_INTS_AUTO)),
            std::make_pair("devicemaker", TSubcat(ONTO_ORG, ONTO_INTS_DEVICE)),
            std::make_pair("tvchannel",   TSubcat(ONTO_ORG, ONTO_INTS_FILM)),
            std::make_pair("hotel",       TSubcat(ONTO_ORG, ONTO_INTS_LIVE)),
            std::make_pair("sportclub",   TSubcat(ONTO_ORG, ONTO_INTS_SPORT)),
            std::make_pair("periodics",   TSubcat(ONTO_ORG, ONTO_INTS_TEXT)),
            std::make_pair("computergame",TSubcat(ONTO_SOFT, ONTO_INTS_PLAY))
        };

        Init(names, Y_ARRAY_SIZE(names));
    }
};

}   // namespace


// EOntoCatsType

TString ToString(EOntoCatsType id) {
    return TOntoCatsCollection::ToString(id);
}

template <>
EOntoCatsType FromStringImpl<>(const char* name, size_t size) {
    return TOntoCatsCollection::FromString(TStringBuf(name, size));
}


// EOntoIntsType

TString ToString(EOntoIntsType id) {
    return TOntoIntsCollection::ToString(id);
}

template <>
EOntoIntsType FromStringImpl<>(const char* name, size_t size) {
    return TOntoIntsCollection::FromString(TStringBuf(name, size));
}


// TSubcat

TString ToString(EOntoCatsType cat, EOntoIntsType intent) {
    const TString* name = Default<TSubcatCollection>().FindId(TSubcat(cat, intent));
    return name ? *name : TString();
}

