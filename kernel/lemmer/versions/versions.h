#pragma once

#include <library/cpp/langs/langs.h>
#include <util/generic/string.h>
#include <util/stream/input.h>

namespace NLanguageVersioning {
    void InitializeHash(IInputStream& src, ELanguage langId);
    bool IsWordHashed(const TUtf16String& word, ELanguage langId);
}
