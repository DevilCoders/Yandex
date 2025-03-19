#include "cascade_item.h"
#include "remorph_item.h"
#include "tokenlogic_item.h"
#include "char_item.h"
#include "version.h"

#include <kernel/remorph/common/magic_input.h>

#include <library/cpp/solve_ambig/rank.h>

#include <util/generic/string.h>
#include <util/generic/yexception.h>
#include <util/stream/file.h>

namespace NCascade {

const static TString FIELD_PRIORITY = "priority";
const static TString FIELD_FILTER = "filter";
const static TString FIELD_SUB_CASCADE_AMBIGUITY = "subCascadeAmbiguity";
const static TString FIELD_SUB_CASCADE_RANK_METHOD = "subCascadeRankMethod";

void TCascadeItem::InitFromArticle(const TArticlePtr& a, const NGzt::TGazetteer& gzt) {
    // Order priority
    Priority = 0;
    a.GetField<i32, const TString&>(FIELD_PRIORITY, Priority);

    bool subCascadeAmbig = true;
    a.GetField<bool, const TString&>(FIELD_SUB_CASCADE_AMBIGUITY, subCascadeAmbig);
    ResolveCascadeAmbiguity = !subCascadeAmbig;

    TString subCascadeRankMethod;
    a.GetField<TString, const TString&>(FIELD_SUB_CASCADE_RANK_METHOD, subCascadeRankMethod);

    if (!subCascadeRankMethod.empty()) {
        CascadeRankMethod = NSolveAmbig::TRankMethod(subCascadeRankMethod.data());
    }

    TString filter;
    a.GetField<TString, const TString&>(FIELD_FILTER, filter);

    if (!filter.empty()) {
        GetMatcher().SetFilter(filter, &gzt);
    }
}

void TCascadeItem::Save(IOutputStream& out) const {
    ::Save(&out, (ui8)Type);
    ::Save(&out, ArticleTitle);
    ::Save(&out, SubCascades);
}

void TCascadeItem::Load(IInputStream& in) {
    // Type is loaded before constructing object
    ::Load(&in, ArticleTitle);
    ::Load(&in, SubCascades);
}

TCascadeItemPtr TCascadeItem::LoadCascadeItem(IInputStream& in) {
    ui8 type;
    ::Load(&in, type);
    TCascadeItemPtr ptr;
    switch (NMatcher::EMatcherType(type)) {
    case NMatcher::MT_REMORPH: ptr = new TRemorphCascadeItem(); break;
    case NMatcher::MT_TOKENLOGIC: ptr = new TTokenLogicCascadeItem(); break;
    case NMatcher::MT_CHAR: ptr = new TCharCascadeItem(); break;
    default: ythrow yexception() << "Loaded unknown cascade type: " << (ui16)type;
    }
    ptr->Load(in);
    return ptr;
}

} // NCascade
