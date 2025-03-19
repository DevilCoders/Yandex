#pragma once

#include "fact.h"

#include <kernel/remorph/input/input_symbol_util.h>
#include <kernel/remorph/proc_base/matcher_base.h>
#include <kernel/remorph/proc_base/result_base.h>
#include <kernel/remorph/common/verbose.h>

#include <library/cpp/solve_ambig/rank.h>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/compiler/importer.h>

#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <util/generic/ptr.h>
#include <util/generic/hash.h>
#include <util/generic/utility.h>
#include <utility>

namespace NFact {

using namespace NSymbol;
using ::google::protobuf::Descriptor;
using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::compiler::DiskSourceTree;

class TFact;
typedef TIntrusivePtr<TFact> TFactPtr;
class TFieldType;
typedef TIntrusivePtr<TFieldType> TFieldTypePtr;

class TFieldTypeContainer {
private:
    THashMap<TString, size_t> NameToField;
    TVector<TFieldTypePtr> Fields;

protected:
    void Init(const Descriptor& desc);

public:
    inline const TFieldType* FindFieldByName(const TString& name) const {
        THashMap<TString, size_t>::const_iterator i = NameToField.find(name);
        return NameToField.end() == i ? nullptr : Fields[i->second].Get();
    }

    inline const TVector<TFieldTypePtr>& GetFields() const {
        return Fields;
    }
};

class TFieldType: public TFieldTypeContainer, public TSimpleRefCount<TFieldType> {
public:
    enum ETextCase {
        AsIs  = 0,
        Title = 1,
        Camel = 2,
        Upper = 3,
        Lower = 4,
    };

    enum ENormalization {
        None  = 0,
        Nominative = 1,
        Gazetteer = 2,
        Lemmer = 3,
    };

private:
    friend class TFieldTypeContainer;

    const TString Name;
    const FieldDescriptor* ProtoField;
    const bool Prime;
    const ENormalization Norm;
    const ETextCase TextCase;
    const bool Head;
    const i32 HeadOffset;

private:
    TFieldType(const FieldDescriptor& f);

public:
    virtual ~TFieldType() {
    }

    inline const TString& GetName() const {
        return Name;
    }

    inline const FieldDescriptor& GetProtoField() const {
        return *ProtoField;
    }

    inline bool IsPrime() const {
        return Prime;
    }

    inline ENormalization GetNormFlag() const {
        return Norm;
    }

    inline ETextCase GetTextCase() const {
        return TextCase;
    }

    inline bool IsCompound() const {
        return FieldDescriptor::TYPE_MESSAGE == ProtoField->type();
    }

    inline bool IsHead() const {
        return Head;
    }

    inline i32 GetHeadOffset() const {
        return HeadOffset;
    }
};

namespace NPrivate {

class THeadCalculator {
private:
    const TFieldType* HeadField;
    NRemorph::TSubmatch HeadRange;

private:
    size_t GetSymbolOffset() const {
        Y_ASSERT(IsValid());
        size_t headPos = HeadRange.first;
        if (HeadField->GetHeadOffset() < 0) {
            headPos += -HeadField->GetHeadOffset() <= i32(HeadRange.Size()) ? HeadRange.Size() + HeadField->GetHeadOffset() : 0;
        } else {
            headPos += HeadField->GetHeadOffset() < i32(HeadRange.Size()) ? HeadField->GetHeadOffset() : HeadRange.Size() - 1;
        }
        return headPos;
    }

public:
    THeadCalculator()
        : HeadField(nullptr)
    {
    }

    void Update(const TFieldType* field, const NRemorph::TSubmatch& range);
    inline bool IsValid() const {
        return HeadField != nullptr && !HeadRange.IsEmpty();
    }

    size_t GetHeadPos(const TInputSymbols& symbols) const;

    template <class TInputSource, class TResult>
    size_t GetHeadPos(const TInputSource& inputSource, const TResult& res) const {
        const size_t headPos = GetSymbolOffset();
        Y_ASSERT(headPos < res.GetMatchedCount());
        const TInputSymbol* headSymbol = res.GetMatchedSymbol(inputSource, headPos).Get();
        while (headSymbol->GetHead() != nullptr) {
            headSymbol = headSymbol->GetHead();
        }
        return headSymbol->GetSourcePos().first;
    }
};

} // NPrivate

class TFactType: public TFieldTypeContainer, public TSimpleRefCount<TFactType> {
private:
    const Descriptor* Descript;
    NMatcher::EMatcherType MatcherType;
    TString MatcherPath;
    TString GazetteerPath;
    TString Filter;
    TVector<TString> Dominants;
    bool AmbigGazetteer;
    NSolveAmbig::TRankMethod GazetteerRankMethod;
    bool AmbigCascade;
    NSolveAmbig::TRankMethod CascadeRankMethod;
    NMatcher::ESearchMethod SearchMethod;

private:
    void Init(const Descriptor& desc);

    void FillFact(TFact& fact, const TFieldType& fieldType, const TInputSymbols& symbols, const TVector<TDynBitMap>& contexts) const;

    // If two ranges are intersected then replaces them by the single extended range, which covers both ones
    // On the output, coveredRanges contains sorted non-intersected collection of ranges
    static void JoinRanges(TVector<NRemorph::TSubmatch>& coveredRanges);

    template <class TInputSource, class TResult>
    static size_t CalcCoverage(const TVector<NRemorph::TSubmatch>& coveredRanges,
        const TInputSource& inputSource, const TResult& res) {
        size_t coverage = 0;
        for (TVector<NRemorph::TSubmatch>::const_iterator i = coveredRanges.begin(); i != coveredRanges.end(); ++i) {
            coverage += res.SubmatchToOriginal(inputSource, *i).Size();
        }
        return coverage;
    }

    template <class TInputSource, class TResult>
    static double CalcWeight(const TVector<NRemorph::TSubmatch>& coveredRanges,
        const TInputSource& inputSource, const TResult& res) {
        double weight = 1.0;
        for (TVector<NRemorph::TSubmatch>::const_iterator i = coveredRanges.begin(); i != coveredRanges.end(); ++i) {
            weight *= res.GetRangeWeight(inputSource, *i, false);
        }
        return weight;
    }

public:
    // Passed Descriptor reference must be valid during the entire TFactType life-cycle
    TFactType(const Descriptor& desc);
    TFactType(const Descriptor& desc, const TString& descrPath);

    Y_FORCE_INLINE const TString& GetTypeName() const {
        return Descript->name();
    }

    Y_FORCE_INLINE const Descriptor& GetDescriptor() const {
        return *Descript;
    }

    // Returns absolute path of matcher rules if it is declared in the proto-file
    // Relative matcher path is resolved against proto-file location
    Y_FORCE_INLINE const TString& GetMatcherPath() const {
        return MatcherPath;
    }

    // Returns matcher type
    Y_FORCE_INLINE NMatcher::EMatcherType GetMatcherType() const {
        return MatcherType;
    }

    // Returns absolute path of gazetteer dictionary if it is declared in the proto-file
    // Relative gazetteer path is resolved against proto-file location
    Y_FORCE_INLINE const TString& GetGazetteerPath() const {
        return GazetteerPath;
    }

    Y_FORCE_INLINE const TString& GetFilter() const {
        return Filter;
    }

    Y_FORCE_INLINE const TVector<TString>& GetDominants() const {
        return Dominants;
    }

    Y_FORCE_INLINE bool IsAmbigGazetteer() const {
        return AmbigGazetteer;
    }

    Y_FORCE_INLINE const NSolveAmbig::TRankMethod& GetGazetteerRankMethod() const {
        return GazetteerRankMethod;
    }

    Y_FORCE_INLINE bool IsAmbigCascade() const {
        return AmbigCascade;
    }

    Y_FORCE_INLINE const NSolveAmbig::TRankMethod& GetCascadeRankMethod() const {
        return CascadeRankMethod;
    }

    Y_FORCE_INLINE NMatcher::ESearchMethod GetSearchMethod() const {
        return SearchMethod;
    }

    // Create the fact from the remorph result
    template <class TInputSource, class TResult>
    TFactPtr CreateFact(const TInputSource& inputSource, const TResult& res) const {
        // Ignore empty matches
        if (res.MatchTrack.Size() == 0) {
            return TFactPtr();
        }
        TInputSymbols symbols;
        TVector<TDynBitMap> contexts;
        res.ExtractMatched(inputSource, symbols, contexts);
        TFactPtr fact(new TFact(*this, symbols, contexts, res.RuleName));

        NRemorph::TNamedSubmatches subs;
        res.GetNamedRanges(subs);
        NRemorph::TSubmatch primePos;
        TVector<NRemorph::TSubmatch> coveredRanges;
        NPrivate::THeadCalculator headCalc;
        for (NRemorph::TNamedSubmatches::const_iterator iSub = subs.begin(); iSub != subs.end(); ++iSub) {
            const TFieldType* f = FindFieldByName(iSub->first);
            if (nullptr != f) {
                symbols.clear();
                contexts.clear();
                res.ExtractMatched(inputSource, iSub->second, symbols, contexts);
                ExpungeChildrenGztArticles(symbols, contexts);

                fact->Add(*f, symbols, contexts);
                if (f->IsPrime()) {
                    // Store the position of prime field
                    if (primePos.IsEmpty()) {
                        primePos = iSub->second;
                        // If fact has at least one prime field then use only prime fields for coverage
                        // Reset all ranges, which are collected before
                        coveredRanges.clear();
                    } else {
                        primePos.first = Min(primePos.first, iSub->second.first);
                        primePos.second = Max(primePos.second, iSub->second.second);
                    }
                    coveredRanges.push_back(iSub->second);
                } else if (primePos.IsEmpty()) {
                    coveredRanges.push_back(iSub->second);
                }
                headCalc.Update(f, iSub->second);
            }
        }

        // Ignore facts without required fields
        if (!fact->IsComplete()) {
            REPORT(NOTICE, "Ignoring fact w/o required fields: " << fact->ToString(false));
            return TFactPtr();
        }

        JoinRanges(coveredRanges);
        const size_t coverage = CalcCoverage(coveredRanges, inputSource, res);
        if (coverage > 0)
            fact->SetCoverage(coverage);

        // Update fact position if it has prime fields
        if (!primePos.IsEmpty())
            fact->GetSrcPos() = res.SubmatchToOriginal(inputSource, primePos);
        fact->GetWholeExprPos() = res.WholeExpr;

        // The more context is used - the better fact
        Y_ASSERT(fact->GetCoverage() > 0);
        const double weight = res.GetWeight() * CalcWeight(coveredRanges, inputSource, res)
            * double(res.WholeExpr.Size()) / double(fact->GetCoverage());
        fact->SetWeight(weight);

        // Set head
        if (headCalc.IsValid()) {
            fact->SetHeadPos(headCalc.GetHeadPos(inputSource, res));
        }

        return fact;
    }

    // Convert collection of remorph results to the collection of facts
    template <class TInputSource, class TResultPtr>
    void ResultsToFacts(const TInputSource& inputSource, const TVector<TResultPtr>& results, TVector<TFactPtr>& facts) const {
        for (typename TVector<TResultPtr>::const_iterator iRes = results.begin(); iRes != results.end(); ++iRes) {
            const typename TResultPtr::TValueType& matchRes = **iRes;
            TFactPtr fact = CreateFact(inputSource, matchRes);
            if (fact) {
                facts.push_back(fact);
                REPORT(DEBUG, "Creating fact \"" << GetTypeName() << "\", w=" << fact->GetWeight() << " from: "
                    << matchRes.ToDebugString(GetVerbosityLevel(), inputSource));
            }
        }
    }
};

typedef TIntrusivePtr<TFactType> TFactTypePtr;

} // NFact
