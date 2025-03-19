#pragma once

#include <util/charset/wide.h>
#include <kernel/lemmer/dictlib/grammar_index.h>
////////////////////////////////////////////////////////////////////////////////////////////////////
void PhraseToDeclension(TUtf16String phrase, EGrammar declension, TVector<TUtf16String> *res);
bool IsMatchGramPattern(const TUtf16String& phrase, const TGramBitSet& pattern);
