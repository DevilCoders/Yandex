#pragma once

#include <kernel/remorph/input/input_symbol.h>

#include <kernel/lemmer/dictlib/grambitset.h>
#include <kernel/lemmer/dictlib/gleiche.h>

#include <util/generic/vector.h>
#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/generic/ptr.h>
#include <util/generic/bitmap.h>
#include <util/system/yassert.h>
#include <util/stream/output.h>
#include <library/cpp/containers/sorted_vector/sorted_vector.h>

namespace NLiteral {

using namespace NSymbol;

// Key - unique context ID, which is formed using agreement type/subtype and agreement label
typedef NSorted::TSimpleMap<TString, std::pair<TInputSymbolPtr, TDynBitMap*>> TAgreementContext;

#define AGREE_TYPE_LIST     \
    X(Gazetteer)            \
    X(GeoGazetteer)         \
    X(Distance)             \
    X(Text)                 \
    X(GazetteerId)

#define GR_AGREE_TYPE_LIST  \
    X(Case)                 \
    X(Number)               \
    X(Tense)                \
    X(CaseNumber)           \
    X(CaseFirstPlural)      \
    X(GenderNumber)         \
    X(GenderCase)           \
    X(PersonNumber)         \
    X(GenderNumberCase)

class TAgreement : public TSimpleRefCount<TAgreement> {
public:
    enum EAgreeType {
#define X(A) A,
        AGREE_TYPE_LIST
        GR_AGREE_TYPE_LIST
#undef X
    };

protected:
    EAgreeType Type;
    TString ContextID;
    bool Negated;

protected:
    TAgreement(EAgreeType type)
        : Type(type)
        , Negated(false)
    {
    }

    virtual bool DoCheck(const TInputSymbolPtr& controller, TDynBitMap& controllerCtx,
        const TInputSymbolPtr& target, TDynBitMap& targetCtx) const = 0;

public:
    virtual ~TAgreement() {
    }

    EAgreeType GetType() const {
        return Type;
    }

    void SetContext(const TString& context) {
        ContextID = context;
    }

    const TString& GetContext() const {
        return ContextID;
    }

    void SetNegated(bool neg) {
        Negated = neg;
    }

    bool Check(const TInputSymbolPtr& input, TDynBitMap& ctx, TAgreementContext& context) const;

    virtual void Load(IInputStream* in);
    virtual void Save(IOutputStream* out) const;
};

typedef TIntrusivePtr<TAgreement> TAgreementPtr;


class TGazetteerAgreement : public TAgreement {
protected:
    bool DoCheck(const TInputSymbolPtr& controller, TDynBitMap& controllerCtx,
        const TInputSymbolPtr& target, TDynBitMap& targetCtx) const override;

public:
    TGazetteerAgreement()
        : TAgreement(TAgreement::Gazetteer)
    {
    }
};

class TGeoGazetteerAgreement : public TAgreement {
protected:
    bool DoCheck(const TInputSymbolPtr& controller, TDynBitMap& controllerCtx,
        const TInputSymbolPtr& target, TDynBitMap& targetCtx) const override;

public:
    TGeoGazetteerAgreement()
        : TAgreement(TAgreement::GeoGazetteer)
    {
    }
};

class TGazetteerIdAgreement : public TAgreement {
protected:
    bool DoCheck(const TInputSymbolPtr& controller, TDynBitMap& controllerCtx,
        const TInputSymbolPtr& target, TDynBitMap& targetCtx) const override;

public:
    TGazetteerIdAgreement()
        : TAgreement(TAgreement::GazetteerId)
    {
    }
};


class TDistanceAgreement : public TAgreement {
private:
    size_t DistanceLength;
protected:
    bool DoCheck(const TInputSymbolPtr& controller, TDynBitMap& controllerCtx,
        const TInputSymbolPtr& target, TDynBitMap& targetCtx) const override;

public:
    TDistanceAgreement()
        : TAgreement(TAgreement::Distance)
        , DistanceLength(1)
    {
    }

    void SetDistance(size_t dist) {
        DistanceLength = dist;
    }

    size_t GetDistance() const {
        return DistanceLength;
    }

    void Load(IInputStream* in) override;
    void Save(IOutputStream* out) const override;
};

class TTextAgreement : public TAgreement {
protected:
    bool DoCheck(const TInputSymbolPtr& controller, TDynBitMap& controllerCtx,
        const TInputSymbolPtr& target, TDynBitMap& targetCtx) const override;

public:
    TTextAgreement()
        : TAgreement(TAgreement::Text)
    {
    }
};

class TGrammarAgreementBase : public TAgreement {
private:
    NGleiche::TGleicheFunc AgreeFunc;

private:
    void Init();

protected:
    bool DoCheck(const TInputSymbolPtr& controller, TDynBitMap& controllerCtx,
        const TInputSymbolPtr& target, TDynBitMap& targetCtx) const override;

public:
    TGrammarAgreementBase(EAgreeType type)
        : TAgreement(type)
        , AgreeFunc(nullptr)
    {
        Init();
    }
};

#define X(A) \
    class T##A##Agreement : public TGrammarAgreementBase { \
    public: \
        T##A##Agreement() \
            : TGrammarAgreementBase(TAgreement::A) \
        { \
        } \
    };
    GR_AGREE_TYPE_LIST
#undef X

class TAgreementGroup: public TSimpleRefCount<TAgreementGroup> {
private:
    TVector<TAgreementPtr> Agreements;
public:
    TAgreementGroup() {
    }
    virtual ~TAgreementGroup() {
    }

    void Load(IInputStream* in) {
        ::Load(in, Agreements);
    }
    void Save(IOutputStream* out) const {
        ::Save(out, Agreements);
    }

    const TVector<TAgreementPtr>& GetAgreements() const {
        return Agreements;
    }

    void AddAgreement(const TAgreementPtr& a) {
        Agreements.push_back(a);
    }

    void AddAgreements(const TVector<TAgreementPtr>& v) {
        Agreements.insert(Agreements.end(), v.begin(), v.end());
    }

    bool Check(const TInputSymbolPtr& s, TDynBitMap& ctx, TAgreementContext& context) const {
        for (TVector<TAgreementPtr>::const_iterator i = Agreements.begin(); i != Agreements.end(); ++i) {
            if (!i->Get()->Check(s, ctx, context))
                return false;
        }
        return true;
    }
};

typedef TIntrusivePtr<TAgreementGroup> TAgreementGroupPtr;

class TAgreementNames {
public:
    TAgreementNames();
    static TAgreement::EAgreeType GetType(const TString& name);

private:
    TMap<TString, TAgreement::EAgreeType> NameToType;
};

inline TAgreementPtr AllocAgreement(TAgreement::EAgreeType type ) {
    TAgreementPtr res;
    switch (type) {
#define X(A) case TAgreement::A: res = new T##A##Agreement(); break;
    AGREE_TYPE_LIST
    GR_AGREE_TYPE_LIST
#undef X
    default:
        Y_FAIL("Unimplemented agreement type");
    }
    return res;
}

inline TAgreementPtr LoadAgreement(IInputStream* in) {
    ui8 type = 0;
    ::Load(in, type);
    TAgreementPtr res = AllocAgreement((TAgreement::EAgreeType)type);
    if (res) {
        res->Load(in);
    }
    return res;
}

TAgreementPtr CreateAgreement(const TString& name);

} // NLiteral

template<>
class TSerializer<NLiteral::TAgreementPtr> {
public:
    static inline void Save(IOutputStream* rh, const NLiteral::TAgreementPtr& p) {
        p->Save(rh);
    }

    static inline void Load(IInputStream* rh, NLiteral::TAgreementPtr& p) {
        p = NLiteral::LoadAgreement(rh);
    }
};

template<>
class TSerializer<NLiteral::TAgreementGroupPtr> {
public:
    static inline void Save(IOutputStream* rh, const NLiteral::TAgreementGroupPtr& p) {
        p->Save(rh);
    }

    static inline void Load(IInputStream* rh, NLiteral::TAgreementGroupPtr& p) {
        p = new NLiteral::TAgreementGroup();
        p->Load(rh);
    }
};
