#pragma once

#include <kernel/gazetteer/gztarticle.h>
#include <kernel/remorph/input/input_symbol.h>

#include <library/cpp/solve_ambig/occurrence.h>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>

#include <util/generic/bitmap.h>
#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <util/generic/ptr.h>
#include <util/generic/hash.h>
#include <util/generic/vector.h>
#include <utility>

namespace NFact {

using namespace NSymbol;

using google::protobuf::Message;

// Forward definitions from facttype.h
class TFieldType;
class TFieldTypeContainer;
class TFactType;

class TRangeInfo {
protected:
    std::pair<size_t, size_t> SrcPos;
    std::pair<size_t, size_t> WholeExprPos;
    TInputSymbols Symbols;
    TVector<TDynBitMap> Contexts;

protected:
    TRangeInfo(const TInputSymbols& symbols, const TVector<TDynBitMap>& contexts);

public:
    virtual ~TRangeInfo() {
    }

    // Original position in source objects
    inline const std::pair<size_t, size_t>& GetSrcPos() const {
        return SrcPos;
    }

    // Original position in source objects
    inline std::pair<size_t, size_t>& GetSrcPos() {
        return SrcPos;

    }
    // Original position of the whole matched sequence (SrcPos can be strictly contained in it)
    inline const std::pair<size_t, size_t>& GetWholeExprPos() const {
        return WholeExprPos;
    }

    // Original position of the whole matched sequence (SrcPos can be strictly contained in it)
    inline std::pair<size_t, size_t>& GetWholeExprPos() {
        return WholeExprPos;
    }

    inline const TInputSymbols& GetSymbols() const {
        return Symbols;
    }

    inline const TVector<TDynBitMap>& GetContexts() const {
        return Contexts;
    }

    void RemapPositions(const TVector<size_t>& offsetMap) {
        SrcPos.first = offsetMap[SrcPos.first];
        SrcPos.second = offsetMap[SrcPos.second];
        WholeExprPos.first = offsetMap[WholeExprPos.first];
        WholeExprPos.second = offsetMap[WholeExprPos.second];
    }
};

class TFieldValueContainer;

class TFieldValue: public TRangeInfo, public TSimpleRefCount<TFieldValue> {
    friend class TFieldValueContainer;
private:
    const TFieldType* FieldType;
    TUtf16String Text;
    TVector<NGzt::TArticlePtr> Articles;

protected:
    TFieldValue(const TFieldType& type, const TUtf16String& text, const TVector<NGzt::TArticlePtr>& articles,
        const TInputSymbols& symbols, const TVector<TDynBitMap>& contexts);

public:
    ~TFieldValue() override {
    }

    // Field name
    const TString& GetName() const;

    // Field type descriptor
    Y_FORCE_INLINE const TFieldType& GetFieldType() const {
        return *FieldType;
    }

    // Normalized field value (according to 'norm' and 'text_case' attributes)
    Y_FORCE_INLINE const TUtf16String& GetText() const {
        return Text;
    }

    // All gzt articles, which are covered by this range
    Y_FORCE_INLINE const TVector<NGzt::TArticlePtr>& GetArticles() const {
        return Articles;
    }
};

typedef TIntrusivePtr<TFieldValue> TFieldValuePtr;

class TCompoundFieldValue;
typedef TIntrusivePtr<TCompoundFieldValue> TCompoundFieldValuePtr;

class TFieldValueContainer {
private:
    THashMultiMap<TStringBuf, TFieldValuePtr> Values;
    THashMultiMap<TStringBuf, TCompoundFieldValuePtr> CompoundValues;
    TString RuleName;
    TString CompoundRuleName; // Comma-separated list of current rule and all sub-rules from compound values
    double Weight;
    size_t HeadPos;

protected:
    TFieldValueContainer()
        : Weight(1.0)
        , HeadPos(0)
    {
    }

    void Add(const TFieldType& type, const TUtf16String& text, const TVector<NGzt::TArticlePtr>& articles, const TInputSymbols& symbols, const TVector<TDynBitMap>& contexts);
    void Add(const TFieldType& type, const TInputSymbols& symbols, const TVector<TDynBitMap>& contexts);

    // Validates that all required fields are filled
    bool IsComplete(const TFieldTypeContainer& types) const;

    template <class TValue>
    TVector<TValue> GetValuesImpl(const THashMultiMap<TStringBuf, TValue>& values, const TStringBuf& name) const {
        TVector<TValue> res;
        auto range = values.equal_range(name);
        for (typename THashMultiMap<TStringBuf, TValue>::const_iterator i = range.first; i != range.second; ++i) {
            res.push_back(i->second);
        }
        return res;
    }

    inline void SetRuleName(const TString& name) {
        RuleName = CompoundRuleName = name;
    }

    void RemapPositions(const TVector<size_t>& offsetMap);

public:
    virtual ~TFieldValueContainer() {
    }

    // Normal values (non-repeating)
    inline const THashMultiMap<TStringBuf, TFieldValuePtr>& GetValues() const {
        return Values;
    }

    // Normal values by name
    inline TVector<TFieldValuePtr> GetValues(const TStringBuf& name) const {
        return GetValuesImpl(Values, name);
    }

    // The first value by name
    bool GetFirstValue(const TStringBuf& name, TUtf16String& val, std::pair<size_t, size_t>* srcPos = nullptr) const {
        auto it = Values.find(name);
        if (it != Values.end()) {
            val = it->second->GetText();
            if (srcPos)
                *srcPos = it->second->GetSrcPos();
            return true;
        }
        return false;
    }

    // Compound values (may repeat)
    inline const THashMultiMap<TStringBuf, TCompoundFieldValuePtr>& GetCompoundValues() const {
        return CompoundValues;
    }

    // Compound values by name
    inline TVector<TCompoundFieldValuePtr> GetCompoundValues(const TStringBuf& name) const {
        return GetValuesImpl(CompoundValues, name);
    }

    inline const TString& GetRuleName() const {
        return RuleName;
    }

    inline const TString& GetCompoundRuleName() const {
        return CompoundRuleName;
    }

    inline void SetHeadPos(size_t pos) {
        HeadPos = pos;
    }

    inline size_t GetHeadPos() const {
        return HeadPos;
    }

    inline double GetWeight() const {
        return Weight;
    }

    inline void SetWeight(double w) {
        Weight = w;
    }

    TVector<NGzt::TArticlePtr> GetAllArticles() const;
    bool HasArticle(const TWtringBuf& name) const;
};

// Compound field value, which contains sub-fields
class TCompoundFieldValue: public TFieldValue, public TFieldValueContainer {
    friend class TFieldValueContainer;
protected:
    TCompoundFieldValue(const TFieldType& type, const TUtf16String& text, const TVector<NGzt::TArticlePtr>& articles,
        const TInputSymbols& symbols, const TVector<TDynBitMap>& contexts, size_t compundSymbolPos);
public:
    ~TCompoundFieldValue() override {
    }
    // Validates that all required fields are filled
    bool IsComplete() const;

    void RemapPositions(const TVector<size_t>& offsetMap) {
        TFieldValue::RemapPositions(offsetMap); // Remap positions of the field itself
        TFieldValueContainer::RemapPositions(offsetMap); // Remap positions of sub-fields
    }
};

class TFact: public TRangeInfo, public TFieldValueContainer, public TSimpleRefCount<TFact> {
    friend class TFactType;
private:
    const TFactType* Type;
    size_t Coverage;

protected:
    TFact(const TFactType& type, const TInputSymbols& symbols, const TVector<TDynBitMap>& contexts,
        const TString& ruleName);

    void ConstructString(TString& res, const TFieldTypeContainer& types, const TFieldValueContainer& fields,
        const TString& prefix, bool onlyPrime) const;
    void FillMessage(Message& msg, const TFieldValueContainer& fields) const;

public:
    ~TFact() override {
    }

    // Validates that all required fields are filled
    bool IsComplete() const;
    // The returned message is valid while the corresponding TFactType object is valid
    TAutoPtr<Message> ToMessage() const;

    Y_FORCE_INLINE const TFactType& GetType() const {
        return *Type;
    }

    inline size_t GetCoverage() const {
        return Coverage;
    }

    inline void SetCoverage(size_t coverage) {
        Coverage = coverage;
    }

    TString ToString(bool onlyPrime = true) const;

    void RemapPositions(const TVector<size_t>& offsetMap) {
        TRangeInfo::RemapPositions(offsetMap); // Remap positions of the field itself
        TFieldValueContainer::RemapPositions(offsetMap); // Remap positions of sub-fields
    }
};

typedef TIntrusivePtr<TFact> TFactPtr;

} // NFact

namespace NSolveAmbig {

template <>
struct TOccurrenceTraits<NFact::TFact> {
    inline static TStringBuf GetId(const NFact::TFact& f) {
        return f.GetCompoundRuleName();
    }
    inline static size_t GetCoverage(const NFact::TFact& f) {
        return f.GetCoverage();
    }
    inline static size_t GetStart(const NFact::TFact& f) {
        return f.GetSrcPos().first;
    }
    inline static size_t GetStop(const NFact::TFact& f) {
        return f.GetSrcPos().second;
    }
    inline static double GetWeight(const NFact::TFact& f) {
        return f.GetWeight();
    }
};

} // NSolveAmbig
