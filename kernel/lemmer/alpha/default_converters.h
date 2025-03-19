#pragma once

#include "abc.h"
#include "diacritics.h"
#include <library/cpp/token/charfilter.h>
#include <util/charset/wide.h>
#include <util/generic/singleton.h>

struct TDefaultPreConverter: public NLemmer::NDetail::TTr {
    TDefaultPreConverter();
};

const wchar16 EMPTY_LIST[] = {0};

struct TEmptyConverter : NLemmer::NDetail::TTr {
    TEmptyConverter()
        : TTr(EMPTY_LIST, EMPTY_LIST, EMPTY_LIST)
    {
    }
};

/// Redefine the singleton values for specific classes
/** This is necessary for TAlphaStructConstructer class, so that no-effect
  * classes return nullptr instead of a class instance.
  */
template <class T>
const T* SingletonDefaulter() {
    return HugeSingleton<T>();
}

template <>
const TEmptyConverter* SingletonDefaulter<TEmptyConverter>() {
    return nullptr;
}

namespace {
    class TEmptyDiacriticsMap: public NLemmer::TDiacriticsMap {};
}

template <>
const TEmptyDiacriticsMap* SingletonDefaulter<TEmptyDiacriticsMap>() {
    return nullptr;
}

template <class TAlpha,
          class TPreConverter,
          class TDerenyxer,
          class TConverter,
          class TPreLower = TEmptyConverter,
          class TPreUpper = TEmptyConverter,
          class TPreTitle = TEmptyConverter,
          class TDiaMap = TEmptyDiacriticsMap>
struct TAlphaStructConstructer: public NLemmer::TAlphabetWordNormalizer {
    TAlphaStructConstructer()
        : TAlphabetWordNormalizer(
              SingletonDefaulter<TAlpha>(),
              SingletonDefaulter<TPreConverter>(),
              SingletonDefaulter<TDerenyxer>(),
              SingletonDefaulter<TConverter>(),
              SingletonDefaulter<TPreLower>(),
              SingletonDefaulter<TPreUpper>(),
              SingletonDefaulter<TPreTitle>(),
              SingletonDefaulter<TDiaMap>()) {
    }
};

namespace {
    struct TDefaultConverter: public NLemmer::TAlphabetWordNormalizer {
        TDefaultConverter()
            : TAlphabetWordNormalizer(
                  nullptr,
                  SingletonDefaulter<TDefaultPreConverter>(),
                  nullptr,
                  nullptr,
                  nullptr,
                  nullptr,
                  nullptr,
                  nullptr) {
        }
        bool AdvancedConvert(const wchar16* word,
                             size_t length,
                             wchar16* converted,
                             size_t bufLen,
                             NLemmer::TConvertRet& ret) const {
            ret.Length = NormalizeUnicode(word, length, converted, bufLen);
            return true;
        }
        bool AdvancedConvert(TUtf16String& s, NLemmer::TConvertRet& ret) const {
            s = NormalizeUnicode(s);
            ret.Length = s.length();
            return true;
        }
    };
}

// alphabet normalizers definitions (in alphabetical order)

namespace NLemmer {
    namespace NDetail {
        const NLemmer::TAlphabetWordNormalizer& AlphaDefault() {
            return *Singleton<TDefaultConverter>();
        }
    }
}
