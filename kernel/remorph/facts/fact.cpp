#include "fact.h"
#include "facttype.h"

#include <kernel/remorph/core/core.h>
#include <kernel/remorph/input/input_symbol_util.h>

#include <google/protobuf/dynamic_message.h>

#include <util/generic/yexception.h>
#include <util/generic/singleton.h>
#include <util/generic/utility.h>
#include <util/generic/ylimits.h>
#include <util/system/yassert.h>
#include <util/charset/wide.h>

namespace NFact {

class TDynamicMessageFactory: public google::protobuf::DynamicMessageFactory {
public:
    TDynamicMessageFactory() {
        SetDelegateToGeneratedFactory(true);
    }
};

TRangeInfo::TRangeInfo(const TInputSymbols& symbols, const TVector<TDynBitMap>& contexts)
    : Symbols(symbols)
    , Contexts(contexts)
{
    Y_ASSERT(!symbols.empty());
    Y_ASSERT(symbols.size() == contexts.size());

    SrcPos.first = symbols.front()->GetSourcePos().first;
    SrcPos.second = symbols.back()->GetSourcePos().second;
}

TFieldValue::TFieldValue(const TFieldType& type, const TUtf16String& text, const TVector<NGzt::TArticlePtr>& articles,
    const TInputSymbols& symbols, const TVector<TDynBitMap>& contexts)
    : TRangeInfo(symbols, contexts)
    , FieldType(&type)
    , Text(text)
    , Articles(articles)
{
    switch (type.GetTextCase()) {
    case TFieldType::AsIs:
        break;
    case TFieldType::Upper:
        Text = to_upper(Text);
        break;
    case TFieldType::Title:
        Text = to_title(Text);
        break;
    case TFieldType::Lower:
        Text = to_lower(Text);
        break;
    case TFieldType::Camel:
        Text = NSymbol::ToCamelCase(Text);
        break;
    default:
        Y_FAIL("Unimplemented field case flag");
    }
}

const TString& TFieldValue::GetName() const {
    return FieldType->GetName();
}

void TFieldValueContainer::Add(const TFieldType& type, const TUtf16String& text, const TVector<NGzt::TArticlePtr>& articles, const TInputSymbols& symbols, const TVector<TDynBitMap>& contexts) {
    if (type.IsCompound()) {
        for (size_t i = 0; i < symbols.size(); ++i) {
            if (!symbols[i]->GetChildren().empty() && !symbols[i]->GetNamedSubRanges().empty()) {
                TCompoundFieldValuePtr val = new TCompoundFieldValue(type, text, articles, symbols, contexts, i);
                CompoundRuleName.append(',').append(val->GetCompoundRuleName());
                CompoundValues.insert(std::make_pair(TStringBuf(type.GetName()), val));
                if (!type.GetProtoField().is_repeated())
                    break;
            }
        }
    } else {
        Values.insert(std::make_pair(TStringBuf(type.GetName()), new TFieldValue(type, text, articles, symbols, contexts)));
    }
}

static TVector<NGzt::TArticlePtr> CollectArticles(const TInputSymbols& symbols, const TVector<TDynBitMap>& contexts) {
    Y_ASSERT(symbols.size() == contexts.size());
    TVector<NGzt::TArticlePtr> articles;
    for (size_t i = 0; i < symbols.size(); ++i) {
        symbols[i]->TraverseArticles(contexts[i], NRemorph::TReturnPusher<NGzt::TArticlePtr, bool, false>(articles));
    }
    return articles;
}

void TFieldValueContainer::Add(const TFieldType& type, const TInputSymbols& symbols, const TVector<TDynBitMap>& contexts) {
    switch (type.GetNormFlag()) {
    case TFieldType::None:
        Add(type, NSymbol::ToWtroku(symbols), CollectArticles(symbols, contexts), symbols, contexts);
        break;
    case TFieldType::Nominative:
        Add(type, NSymbol::GetNominativeForm(symbols, contexts), CollectArticles(symbols, contexts), symbols, contexts);
        break;
    case TFieldType::Gazetteer: {
        if (type.GetProtoField().is_repeated()) {
            TGztLemmas forms = NSymbol::GetAllGazetteerForms(symbols, contexts);
            for (TGztLemmas::const_iterator iForm = forms.begin(); iForm != forms.end(); ++iForm) {
                Add(type, iForm->Text, iForm->Articles, symbols, contexts);
            }
        } else {
            TGztLemma lemma = NSymbol::GetGazetteerForm(symbols, contexts);
            Add(type, lemma.Text, lemma.Articles, symbols, contexts);
        }
        break;
    }
    case TFieldType::Lemmer:
        Add(type, NSymbol::JoinInputSymbolText(symbols.begin(), symbols.end(), NSymbol::TNormalizedTextExtractor()),
            CollectArticles(symbols, contexts), symbols, contexts);
        break;
    default:
        Y_FAIL("Unsupported field normalization type");
    }
}

TVector<NGzt::TArticlePtr> TFieldValueContainer::GetAllArticles() const {
    TVector<NGzt::TArticlePtr> res;
    for (THashMultiMap<TStringBuf, TFieldValuePtr>::const_iterator i = Values.begin(); i != Values.end(); ++i) {
        res.insert(res.end(), i->second->GetArticles().begin(), i->second->GetArticles().end());
    }

    for (THashMultiMap<TStringBuf, TCompoundFieldValuePtr>::const_iterator i = CompoundValues.begin(); i != CompoundValues.end(); ++i) {
        TVector<NGzt::TArticlePtr> innerArticles = i->second->GetAllArticles();
        res.insert(res.end(), innerArticles.begin(), innerArticles.end());
        res.insert(res.end(), i->second->GetArticles().begin(), i->second->GetArticles().end());
    }
    return res;
}

bool TFieldValueContainer::HasArticle(const TWtringBuf& name) const {
    for (THashMultiMap<TStringBuf, TFieldValuePtr>::const_iterator i = Values.begin(); i != Values.end(); ++i) {
        for (TVector<NGzt::TArticlePtr>::const_iterator iArt = i->second->GetArticles().begin();
            iArt != i->second->GetArticles().end(); ++iArt)
        {
            if (iArt->GetTitle() == name || iArt->GetTypeName() == name)
                return true;
        }
    }
    for (THashMultiMap<TStringBuf, TCompoundFieldValuePtr>::const_iterator i = CompoundValues.begin(); i != CompoundValues.end(); ++i) {
        for (TVector<NGzt::TArticlePtr>::const_iterator iArt = i->second->GetArticles().begin();
            iArt != i->second->GetArticles().end(); ++iArt)
        {
            if (iArt->GetTitle() == name || iArt->GetTypeName() == name)
                return true;
        }
        if (i->second->HasArticle(name))
            return true;
    }
    return false;
}

void TFieldValueContainer::RemapPositions(const TVector<size_t>& offsetMap) {
    for (THashMultiMap<TStringBuf, TFieldValuePtr>::iterator i = Values.begin(); i != Values.end(); ++i) {
        i->second->RemapPositions(offsetMap);
    }
    for (THashMultiMap<TStringBuf, TCompoundFieldValuePtr>::iterator i = CompoundValues.begin(); i != CompoundValues.end(); ++i) {
        i->second->RemapPositions(offsetMap);
    }
}

TCompoundFieldValue::TCompoundFieldValue(const TFieldType& type, const TUtf16String& text, const TVector<NGzt::TArticlePtr>& articles,
                                         const TInputSymbols& symbols, const TVector<TDynBitMap>& contexts, size_t compundSymbolPos)
    : TFieldValue(type, text, articles, symbols, contexts)
{
    Y_ASSERT(type.IsCompound());
    Y_ASSERT(compundSymbolPos < symbols.size());
    Y_ASSERT(!symbols[compundSymbolPos]->GetChildren().empty());
    Y_ASSERT(!symbols[compundSymbolPos]->GetNamedSubRanges().empty());

    const TInputSymbol& compound = *symbols[compundSymbolPos];

    // By default, use head from the rule
    if (symbols.size() == 1) {
        const TInputSymbol* headSymbol = &compound;
        while (headSymbol->GetHead() != nullptr) {
            headSymbol = headSymbol->GetHead();
        }
        SetHeadPos(headSymbol->GetSourcePos().first);
    }

    SetRuleName(compound.GetRuleName());
    SetWeight(compound.CalcWeight(contexts[compundSymbolPos]));
    WholeExprPos = compound.GetWholeSourcePos();

    TInputSymbols children;
    TVector<TDynBitMap> childContexts;
    NPrivate::THeadCalculator headCalc;
    for (NRemorph::TNamedSubmatches::const_iterator iSub = compound.GetNamedSubRanges().begin();
        iSub != compound.GetNamedSubRanges().end(); ++iSub) {
        const TFieldType* f = type.FindFieldByName(iSub->first);
        if (nullptr != f) {
            children.assign(compound.GetChildren().begin() + iSub->second.first, compound.GetChildren().begin() + iSub->second.second);
            childContexts.assign(compound.GetContexts().begin() + iSub->second.first, compound.GetContexts().begin() + iSub->second.second);
            Add(*f, children, childContexts);
            headCalc.Update(f, iSub->second);
        }
    }

    // Set head, if it is specified explicitly in fact fields
    if (headCalc.IsValid()) {
        SetHeadPos(headCalc.GetHeadPos(compound.GetChildren()));
    }
}

bool TCompoundFieldValue::IsComplete() const {
    return TFieldValueContainer::IsComplete(GetFieldType());
}

// Validates that all required fields are filled
bool TFieldValueContainer::IsComplete(const TFieldTypeContainer& types) const {
    for (TVector<TFieldTypePtr>::const_iterator i1 = types.GetFields().begin(); i1 != types.GetFields().end(); ++i1) {
        const TFieldType& type = *i1->Get();
        if (type.IsCompound()) {
            // Validate that all compound values are also complete
            auto range = CompoundValues.equal_range(type.GetName());
            for (THashMultiMap<TStringBuf, TCompoundFieldValuePtr>::const_iterator i2 = range.first; i2 != range.second; ++i2) {
                if (!i2->second->IsComplete()) {
                    return false;
                }
            }
            // If the type is required than we should have at least one value
            if (range.first == range.second && type.GetProtoField().is_required()) {
                return false;
            }
        } else if (type.GetProtoField().is_required() && Values.find(type.GetName()) == Values.end()) {
            return false;
        }
    }
    return true;
}

TFact::TFact(const TFactType& type, const TInputSymbols& symbols, const TVector<TDynBitMap>& contexts,
    const TString& ruleName)
    : TRangeInfo(symbols, contexts)
    , Type(&type)
    , Coverage(SrcPos.second - SrcPos.first)
{
    SetRuleName(ruleName);
}

void TFact::ConstructString(TString& res, const TFieldTypeContainer& types, const TFieldValueContainer& fields,
    const TString& prefix, bool onlyPrime) const {

    for (TVector<TFieldTypePtr>::const_iterator i = types.GetFields().begin(); i != types.GetFields().end(); ++i) {
        const TFieldType& type = *i->Get();
        if (!onlyPrime || type.IsPrime()) {
            if (type.IsCompound()) {
                TString innerPrefix;
                innerPrefix.append(prefix).append(type.GetName()).append('.');
                TVector<TCompoundFieldValuePtr> values = fields.GetCompoundValues(type.GetName());
                for (TVector<TCompoundFieldValuePtr>::const_iterator iSub = values.begin(); iSub != values.end(); ++iSub) {
                    ConstructString(res, type, **iSub, innerPrefix, onlyPrime);
                }
            } else {
                TVector<TFieldValuePtr> values = fields.GetValues(type.GetName());
                for (TVector<TFieldValuePtr>::const_iterator iSub = values.begin(); iSub != values.end(); ++iSub) {
                    res.append('\t').append(prefix).append(type.GetName()).append('[')
                        .append(WideToUTF8(iSub->Get()->GetText())).append(']');
                }
            }
        }
    }
}

static Y_FORCE_INLINE Message* NewMessageField(Message& outer, const FieldDescriptor& field) {
    return field.is_repeated()
        ? outer.GetReflection()->AddMessage(&outer, &field, Singleton<TDynamicMessageFactory>())
        : outer.GetReflection()->MutableMessage(&outer, &field, Singleton<TDynamicMessageFactory>());
}

void TFact::FillMessage(Message& msg, const TFieldValueContainer& fields) const {
    for (THashMultiMap<TStringBuf, TFieldValuePtr>::const_iterator iVal = fields.GetValues().begin(); iVal != fields.GetValues().end(); ++iVal) {
        const TFieldType& type = iVal->second->GetFieldType();
        switch (type.GetProtoField().type()) {
        case FieldDescriptor::TYPE_STRING:
            if (type.GetProtoField().is_repeated()) {
                msg.GetReflection()->AddString(&msg, &type.GetProtoField(), WideToUTF8(iVal->second->GetText()));
            } else {
                msg.GetReflection()->SetString(&msg, &type.GetProtoField(), WideToUTF8(iVal->second->GetText()));
            }
            break;
        case FieldDescriptor::TYPE_BOOL:
            Y_VERIFY(!type.GetProtoField().is_repeated(), "Repeated boolean fact field is not supported");
            msg.GetReflection()->SetBool(&msg, &type.GetProtoField(), true);
            break;
        default:
            Y_FAIL("Unsupported type of fact field");
        }
    }

    for (THashMultiMap<TStringBuf, TCompoundFieldValuePtr>::const_iterator iVal = fields.GetCompoundValues().begin(); iVal != fields.GetCompoundValues().end(); ++iVal) {
        Y_ASSERT(iVal->second->GetFieldType().IsCompound());
        FillMessage(*NewMessageField(msg, iVal->second->GetFieldType().GetProtoField()), *(iVal->second));
    }
}

bool TFact::IsComplete() const {
    return TFieldValueContainer::IsComplete(GetType());
}

TAutoPtr<Message> TFact::ToMessage() const {
    TAutoPtr<Message> res(Singleton<TDynamicMessageFactory>()->GetPrototype(&Type->GetDescriptor())->New());
    FillMessage(*res, *this);
    return res;
}

TString TFact::ToString(bool onlyPrime) const {
    TString res;
    res.append(GetType().GetTypeName());
    ConstructString(res, *(this->Type), *this, TString(), onlyPrime);
    return res;
}

} // NFact
