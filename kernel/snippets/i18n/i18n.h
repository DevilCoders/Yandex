#pragma once

#include <util/generic/strbuf.h>
#include <util/charset/wide.h>
#include <library/cpp/langs/langs.h>

namespace NSnippets {

TUtf16String Localize(const TStringBuf& key, const ELanguage lang);

} // namespace NSnippets
