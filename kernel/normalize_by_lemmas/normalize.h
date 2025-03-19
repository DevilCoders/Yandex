#pragma once

#include <kernel/gazetteer/gazetteer.h>
#include <kernel/remorph/matcher/remorph_matcher.h>

struct TNormalizeByLemmasInfo {
    const THolder<const TGazetteer> SpecialWordsGzt;
    const NReMorph::TMatcherPtr MatcherPtr = nullptr;

    TNormalizeByLemmasInfo(const TString& specialWordsGztFileName, const TString& regexpFileName);
};

bool DoesContainPhone(const TStringBuf& query);
TString NormalizePhones(const TStringBuf& query);
TUtf16String NormalizeByLemmas(const TUtf16String& req,
                               const THolder<const TGazetteer>& specialWordsGazetteer,
                               const NReMorph::TMatcherPtr matcherPtr = nullptr,
                               NReMorph::TMatchResults* matchResults = nullptr);
TUtf16String NormalizeByLemmas(const TUtf16String& req, const TNormalizeByLemmasInfo& info, NReMorph::TMatchResults* matchResults = nullptr);
