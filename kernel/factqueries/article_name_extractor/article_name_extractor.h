#pragma once

#include <kernel/normalize_by_lemmas/normalize.h>

#include <util/generic/fwd.h>

bool TryMakeArticleName(
    const TString& query,
    const TString& text,
    const TVector<THolder<TNormalizeByLemmasInfo>>& normalizeInfos,
    TString& articleName,
    TString& answerBody
);
