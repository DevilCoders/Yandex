#pragma once

#include "cascade_base.h"

#include <kernel/remorph/proc_base/matcher_base.h>
#include <kernel/gazetteer/gazetteer.h>

#include <util/generic/string.h>
#include <util/stream/output.h>
#include <util/ysaveload.h>

namespace NCascade {

class TCascadeItem: public TCascadeBase, public TAtomicRefCount<TCascadeItem> {
public:
    NMatcher::EMatcherType Type;
    NGzt::TArticlePtr Article;
    TUtf16String ArticleTitle;
    i32 Priority;

private:
    void InitFromArticle(const TArticlePtr& a, const NGzt::TGazetteer& gzt);

public:
    TCascadeItem(NMatcher::EMatcherType type)
        : Type(type)
        , Priority(0)
    {
    }
    ~TCascadeItem() override {
    }

    virtual void Init(const NGzt::TArticlePtr& a, const NGzt::TGazetteer& gzt) {
        Article = a;
        if (ArticleTitle.empty())
            ArticleTitle = a.GetTitle();
        InitFromArticle(a, gzt);
    }

    virtual void Save(IOutputStream& out) const;
    virtual void Load(IInputStream& in);

    static TCascadeItemPtr LoadCascadeItem(IInputStream& in);
};

template <NMatcher::EMatcherType type>
class TCascadeItemT: public TCascadeItem {
public:
    TCascadeItemT()
        : TCascadeItem(type)
    {
    }
};

} // NCascade

template<>
class TSerializer<NCascade::TCascadeItemPtr> {
public:
    static inline void Save(IOutputStream* rh, const NCascade::TCascadeItemPtr& p) {
        p->Save(*rh);
    }

    static inline void Load(IInputStream* rh, NCascade::TCascadeItemPtr& p) {
        p = NCascade::TCascadeItem::LoadCascadeItem(*rh);
    }
};
