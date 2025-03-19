#pragma once

#include "word_input_symbol.h"

#include <kernel/remorph/tokenizer/multitoken_split.h>
#include <kernel/remorph/tokenizer/callback.h>

#include <library/cpp/langmask/langmask.h>

#include <util/generic/ptr.h>
#include <util/string/vector.h>

namespace NText {

TWordSymbols CreateWordSymbols(const NToken::TSentenceInfo& sentInfo, const TLangMask& lang, bool withMorphology = true, bool useDisamb = false);

TWordSymbols CreateWordSymbols(const TUtf16String& text, const TLangMask& lang, NToken::EMultitokenSplit ms = NToken::MS_MINIMAL, bool withMorphology = true, bool useDisamb = false);

} // NText
