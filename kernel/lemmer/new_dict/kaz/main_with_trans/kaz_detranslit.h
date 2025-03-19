#pragma once

#include <dict/morphdict/translit/kaz_official/detranslit/lib/kaz_detranslit.h>

#include <kernel/lemmer/untranslit/detranslit.h>

class TKazDetransliterator : public IDetransliterator {
public:
    TUtf16String Convert(const TUtf16String& s) const override {
        return KazLatinToCyrillicOfficialLike(s, false);
    }
};

static TAutoPtr<IDetransliterator> GetKazDetransliterator() {
    return new TKazDetransliterator();
}
