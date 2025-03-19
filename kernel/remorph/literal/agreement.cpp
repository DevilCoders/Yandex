#include "agreement.h"

#include <util/ysaveload.h>
#include <util/string/cast.h>
#include <util/generic/strbuf.h>

namespace NLiteral {

bool TAgreement::Check(const TInputSymbolPtr& input, TDynBitMap& ctx, TAgreementContext& context) const {
    bool res = true;
    TAgreementContext::iterator i = context.find(ContextID);
    if (context.end() != i) {
        if (Negated) {
            TDynBitMap ctxCopy(ctx);
            TDynBitMap ctxControllerCopy(*i->second.second);
            res = !DoCheck(i->second.first, ctxControllerCopy, input, ctxCopy);
        } else {
            res = DoCheck(i->second.first, *i->second.second, input, ctx);
        }
        // Update controller for Distance agreement.
        // So, each subsequence token will test agreement with the previous one
        // All other agreements are tested with the first token only
        if (Distance == Type) {
            context[ContextID] = std::make_pair(input, &ctx);
        }
    } else {
        context.insert(std::make_pair(ContextID, std::make_pair(input, &ctx)));
    }
    return res;
}

void TAgreement::Load(IInputStream* in) {
    ::Load(in, ContextID);
    ::Load(in, Negated);
}

void TAgreement::Save(IOutputStream* out) const {
    ::Save(out, (ui8)Type);
    ::Save(out, ContextID);
    ::Save(out, Negated);
}

bool TGazetteerAgreement::DoCheck(const TInputSymbolPtr& controller, TDynBitMap& controllerCtx,
    const TInputSymbolPtr& target, TDynBitMap& targetCtx) const {

    return controller->AgreeGztArticle(controllerCtx, *target, targetCtx);
}

bool TGeoGazetteerAgreement::DoCheck(const TInputSymbolPtr& controller, TDynBitMap& controllerCtx,
    const TInputSymbolPtr& target, TDynBitMap& targetCtx) const {

    TDynBitMap bakControllerCtx(controllerCtx);
    TDynBitMap bakTargetCtx(targetCtx);
    // AgreeGeoGztArticle implementation is asymmetric. Try both directions
    bool res1 = controller->AgreeGeoGztArticle(controllerCtx, *target, targetCtx);
    bool res2 = target->AgreeGeoGztArticle(bakTargetCtx, *controller, bakControllerCtx);
    if (res1) {
        if (res2) {
            controllerCtx.Or(bakControllerCtx);
            targetCtx.Or(bakTargetCtx);
        }
    } else if (res2) {
        DoSwap(controllerCtx, bakControllerCtx);
        DoSwap(targetCtx, bakTargetCtx);
    }
    return res1 || res2;
}

bool TGazetteerIdAgreement::DoCheck(const TInputSymbolPtr& controller, TDynBitMap& controllerCtx,
    const TInputSymbolPtr& target, TDynBitMap& targetCtx) const {

    return controller->AgreeGztIds(controllerCtx, *target, targetCtx);
}

bool TDistanceAgreement::DoCheck(const TInputSymbolPtr& controller, TDynBitMap&,
    const TInputSymbolPtr& target, TDynBitMap&) const {

    Y_ASSERT(target->GetSourcePos().first >= controller->GetSourcePos().second);
    return target->GetSourcePos().first < controller->GetSourcePos().second + DistanceLength;
}

void TDistanceAgreement::Load(IInputStream* in) {
    TAgreement::Load(in);
    ::Load(in, DistanceLength);
}

void TDistanceAgreement::Save(IOutputStream* out) const {
    TAgreement::Save(out);
    ::Save(out, DistanceLength);
}

bool TTextAgreement::DoCheck(const TInputSymbolPtr& controller, TDynBitMap&,
    const TInputSymbolPtr& target, TDynBitMap&) const {

    return controller->GetNormalizedText() == target->GetNormalizedText();
}

void TGrammarAgreementBase::Init() {
    switch (Type) {
#define X(A) case A: AgreeFunc = NGleiche::A##Check; break;
    GR_AGREE_TYPE_LIST
#undef X
    default:
        Y_ASSERT(false);
    }
}

bool TGrammarAgreementBase::DoCheck(const TInputSymbolPtr& controller, TDynBitMap& controllerCtx,
    const TInputSymbolPtr& target, TDynBitMap& targetCtx) const {

    return controller->AgreeGrammems(AgreeFunc, controllerCtx, *target, targetCtx);
}

TAgreementNames::TAgreementNames() {
    NameToType["gzt"] = TAgreement::Gazetteer;
    NameToType["c"] = TAgreement::Case;
    NameToType["n"] = TAgreement::Number;
    NameToType["t"] = TAgreement::Tense;
    NameToType["cn"] = TAgreement::CaseNumber;
    NameToType["cfp"] = TAgreement::CaseFirstPlural;
    NameToType["gn"] = TAgreement::GenderNumber;
    NameToType["gc"] = TAgreement::GenderCase;
    NameToType["pn"] = TAgreement::PersonNumber;
    NameToType["gnc"] = TAgreement::GenderNumberCase;
    NameToType["geogzt"] = TAgreement::GeoGazetteer;
    NameToType["dist"] = TAgreement::Distance;
    NameToType["txt"] = TAgreement::Text;
    NameToType["gztid"] = TAgreement::GazetteerId;
}

TAgreement::EAgreeType TAgreementNames::GetType(const TString& name) {
    const TAgreementNames* names = Singleton<TAgreementNames>();
    TMap<TString, TAgreement::EAgreeType>::const_iterator it = names->NameToType.find(name);
    if (it == names->NameToType.end())
        throw yexception() << "Unsupported agreement type: " << name;

    return it->second;
}

TAgreementPtr CreateAgreement(const TString& name) {
    TString lookup = name;
    bool negated = false;
    if (lookup.StartsWith("no-")) {
        lookup.erase(0, 3);
        negated = true;
    }
    size_t dist = 1;
    if (lookup.StartsWith("dist") && lookup.size() > 4) {
        try {
            dist = FromString<size_t>(TStringBuf(lookup).Tail(4));
        } catch (const TBadCastException& e) {
            throw yexception() << "Bad distance value of distance agreement type: " << name;
        }
        lookup.erase(4);
    }
    TAgreementPtr res = AllocAgreement(TAgreementNames::GetType(lookup));
    if (res) {
        res->SetNegated(negated);
        if (res->GetType() == TAgreement::Distance) {
            dynamic_cast<TDistanceAgreement*>(res.Get())->SetDistance(dist);
        }
    }
    return res;
}


} // NLiteral
