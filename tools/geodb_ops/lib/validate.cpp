#include "validate.h"

#include <library/cpp/logger/global/global.h>
#include <library/cpp/langs/langs.h>
#include <library/cpp/timezone_conversion/convert.h>

#include <kernel/geodb/geodb.h>

#include <util/charset/utf8.h>
#include <util/generic/yexception.h>
#include <util/string/escape.h>
#include <util/string/type.h>
#include <util/system/compiler.h>
#include <util/system/types.h>

static bool IsValidMapping(const NGeoDB::TGeoKeeper& geodb) try {
    INFO_LOG << "Validating mapping" << '\n';
    bool valid = true;
    for (const auto& e : geodb) {
        if (e.first != e.second.GetId()) {
            valid &= false;
            ERROR_LOG << "Invalid mapping: " << e.first << "->" << e.second.GetId() << '\n';
        }
    }
    INFO_LOG << "Done validating mapping" << '\n';
    return valid;
} catch (std::exception&) {
    ERROR_LOG << "Exception occured in `IsValidMapping`: " << CurrentExceptionMessage() << '\n';
    return false;
}

static bool IsValidLanguageID(const ui32 id) {
    return id < LANG_MAX;
}

static bool IsValidNames(const NGeoDB::TGeoKeeper& geodb) try {
    INFO_LOG << "Validating names" << '\n';
    bool valid = true;
    for (const auto& e : geodb) {
        for (const auto& n : e.second.GetNames()) {
            if (!IsValidLanguageID(n.GetLang())) {
                valid &= false;
                ERROR_LOG << "invalid `Lang` for id=" << e.first << '\n';
            }

            if (!IsUtf(n.GetNominative())) {
                valid &= false;
                ERROR_LOG << "`Nominative` is not a valid UTF-8 string for id=" << e.first << '\n';
            }

            if (!IsUtf(n.GetGenitive())) {
                valid &= false;
                ERROR_LOG << "`Genitive` is not a valid UTF-8 string for id=" << e.first << '\n';
            }

            if (!IsUtf(n.GetDative())) {
                valid &= false;
                ERROR_LOG << "`Dative` is not a valid UTF-8 string for id=" << e.first << '\n';
            }

            if (!IsUtf(n.GetLocative())) {
                valid &= false;
                ERROR_LOG << "`Locative` is not a valid UTF-8 string for id=" << e.first << '\n';
            }

            if (!IsUtf(n.GetPreposition())) {
                valid &= false;
                ERROR_LOG << "`Preposition` is not a valid UTF-8 string for id=" << e.first << '\n';
            }

            if (!IsUtf(n.GetDirectional())) {
                valid &= false;
                ERROR_LOG << "`Directional` is not a valid UTF-8 string for id=" << e.first << '\n';
            }
        }

        if (!IsUtf(e.second.GetNativeName())) {
            valid &= false;
            ERROR_LOG << "`NativeName` is not a valid UTF-8 string for id=" << e.first << '\n';
        }

        if (!IsUtf(e.second.GetShortName())) {
            valid &= false;
            ERROR_LOG << "`ShortName` is not a valid UTF-8 string for id=" << e.first << '\n';
        }

        for (const auto& name : e.second.GetSynonymNames()) {
            if (!IsUtf(name)) {
                valid &= false;
                ERROR_LOG << "one of `SynonymNames` is not a valid UTF-8 string for id=" << e.first << '\n';
            }
        }
    }

    INFO_LOG << "Done validating names" << '\n';
    return valid;
} catch (std::exception&) {
    ERROR_LOG << "Exception occured in `IsValidNames`: " << CurrentExceptionMessage() << '\n';
    return false;
}

static bool IsValidTimezone(const NGeoDB::TGeoKeeper& geodb) try {
    INFO_LOG << "Validating timezones" << '\n';
    bool valid = true;
    for (const auto& e : geodb) {
        if (!e.second.HasTimeZone()) {
            continue;
        }

        const auto& tzName = e.second.GetTimeZone();
        if (!tzName) {
            NOTICE_LOG << "`TimeZone` for id=" << e.first << " is empty" << '\n';
            continue;
        }

        try {
            const auto tz = NDatetime::GetTimeZone(tzName);
            Y_FAKE_READ(tz);
        } catch (std::exception&) {
            valid &= false;
            ERROR_LOG << "Unknown `TimeZone` for id=" << e.first
                    << ", TimeZone=\"" << EscapeC(e.second.GetTimeZone()) << '"' << '\n';
        }
    }
    INFO_LOG << "Done validating `TimeZone`" << '\n';
    return valid;
} catch (std::exception&) {
    ERROR_LOG << "Exception occured in `IsValidTimezone`: " << CurrentExceptionMessage() << '\n';
    return false;
}

static bool IsValidPhoneNumber(const NGeoDB::TGeoKeeper& geodb) try {
    INFO_LOG << "Validating `PhoneCode`" << '\n';
    bool valid = true;
    for (const auto& e : geodb) {
        if (e.second.PhoneCodeSize() == 0) {
            continue;
        }

        for (const auto& c : e.second.GetPhoneCode()) {
            if (!IsNumber(c)) {
                valid &= false;
                ERROR_LOG << "phone code \"" << c << "\""
                        << " for region " << e.first << " contains non-numbers" << Endl;
            }
        }
    }
    INFO_LOG << "Done validating `PhoneCode`" << '\n';
    return valid;
} catch (std::exception&) {
    ERROR_LOG << "Exception occured in: \"" << Y_FUNC_SIGNATURE << '"' << CurrentExceptionMessage() << '\n';
    return false;
}

bool NGeoDBOps::IsValid(const ::NGeoDB::TGeoKeeper& geodb) {
    using TValidator = bool (*)(const ::NGeoDB::TGeoKeeper&);
    static constexpr TValidator validators[] = {
        // order matters!
        IsValidMapping,
        IsValidNames,
        IsValidTimezone,
        IsValidPhoneNumber,
    };

    for (const auto validator : validators) {
        if (!validator(geodb)) {
            return false;
        }
    }

    return true;
}
