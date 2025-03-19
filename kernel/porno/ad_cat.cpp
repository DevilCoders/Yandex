#include "ad_cat.h"

#include <kernel/porno/proto/ad_cat.pb.h>

#include <library/cpp/protobuf/util/is_equal.h>

#include <util/generic/algorithm.h>
#include <util/generic/hash.h>
#include <util/generic/strbuf.h>
#include <util/string/cast.h>

namespace {
    using TSetterPtr = void (TAdCatProto::*)(bool);
    using TGetterPtr = bool (TAdCatProto::*)(void) const;

    struct TCategoryDescriptor {
        EAdCat Enum;
        char Letter;
        TSetterPtr Setter;
        TGetterPtr Getter;
    };

    static const TCategoryDescriptor CATEGORY_DESCRIPTORS[] = {
        {AC_EXPLICIT, 'P', &TAdCatProto::SetExplicit, &TAdCatProto::GetExplicit},
        {AC_PERVERSION, 'I', &TAdCatProto::SetPerversion, &TAdCatProto::GetPerversion},
        {AC_SENSITIVE, 'S', &TAdCatProto::SetSensitive, &TAdCatProto::GetSensitive},
        {AC_GREY, 'G', &TAdCatProto::SetGrey, &TAdCatProto::GetGrey},
        {AC_GRUESOME, 'R', &TAdCatProto::SetGruesome, &TAdCatProto::GetGruesome},
        {AC_CHILD, 'C', &TAdCatProto::SetChild, &TAdCatProto::GetChild},
        {AC_ORDINARY, 'O', &TAdCatProto::SetOrdinary, &TAdCatProto::GetOrdinary},
        {AC_FIX_LIST, 'F', &TAdCatProto::SetFixlist, &TAdCatProto::GetFixlist},
    };
    static constexpr size_t CATEGORY_DESCRIPTORS_SIZE = Y_ARRAY_SIZE(CATEGORY_DESCRIPTORS);

    static_assert(EAdCat_ARRAYSIZE == CATEGORY_DESCRIPTORS_SIZE, "");

    TSetterPtr GetSetter(const EAdCat category) noexcept {
        for (const auto& d : CATEGORY_DESCRIPTORS) {
            if (category == d.Enum) {
                return d.Setter;
            }
        }
        return nullptr;
    }

    TSetterPtr GetSetter(const char letter) noexcept {
        for (const auto& d : CATEGORY_DESCRIPTORS) {
            if (letter == d.Letter) {
                return d.Setter;
            }
        }
        return nullptr;
    }

    auto ConstructDefaultValue() {
        TAdCatProto adCat;
        adCat.SetOrdinary(true);
        return adCat;
    }
}

bool SetAdCatCategory(const EAdCat category, TAdCatProto& adCat) {
    if (const auto setter = GetSetter(category)) {
        (adCat.*setter)(true);
        return true;
    }
    return false;
}

bool HasDefaultValue(const TAdCatProto& adCat) {
    static const auto defaultAdCat = ConstructDefaultValue();
    return NProtoBuf::IsEqual(adCat, defaultAdCat);
}

TString AdCatToString(const TAdCatProto& p) {
    char res[CATEGORY_DESCRIPTORS_SIZE];
    char* cur = res;
    for (const auto& d : CATEGORY_DESCRIPTORS) {
        if ((p.*d.Getter)()) {
            *cur++ = d.Letter;
        }
    }
    Sort(res, cur);
    return {res, cur};
}

bool TryAdCatFromString(const TStringBuf categories, TAdCatProto& p) {
    p.Clear();
    for (const auto c : categories) {
        if (const auto setter = GetSetter(c)) {
            (p.*setter)(true);
        } else {
            return false;
        }
    }
    return true;
}

TAdCatProto AdCatFromString(const TStringBuf categories) {
    TAdCatProto r;
    if (!TryAdCatFromString(categories, r)) {
        ythrow TFromStringException{};
    }
    return r;
}
