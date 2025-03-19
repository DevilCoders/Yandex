#pragma once

#include "matcher.h"

#include <kernel/qtree/richrequest/richnode_fwd.h>
#include <library/cpp/langs/langs.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/noncopyable.h>
#include <util/system/spinlock.h>

namespace NUrlCutter {
    class TRTreeWandHelper;

    class TRichTreeWanderer : private TNonCopyable {
    private:
        using TLang2WandererMap = THashMap<ELanguage, TAtomicSharedPtr<TRTreeWandHelper>>;

        TRichTreeConstPtr Richtree;
        TLang2WandererMap Lang2Wanderer;
        TAdaptiveLock Lock;
    private:
        void AddLang(ELanguage lang);
    public:
        explicit TRichTreeWanderer(TRichTreeConstPtr rtree);
        ~TRichTreeWanderer();
        const TMatcherSearcher& GetMatcherSearcher(ELanguage lang);
        const THashSet<TUtf16String>& GetDynamicStopWords(ELanguage lang);
    };
}
