#pragma once

#include <kernel/qtree/richrequest/protos/thesaurus_exttype.pb.h>

#include <library/cpp/wordpos/wordpos.h>

#include "markup.h"

// enum EThesExtType is an int
typedef int TThesaurusExtensionId;

class TRichRequestNode;
class TCreateTreeOptions;

namespace NSearchQuery {
    class TSynonymData {
    private:
        TThesaurusExtensionId Type;
        double Relev;
        EFormClass BestFormClass;

    public:
        TIntrusivePtr<TRichRequestNode> SubTree;

    public:
        TSynonymData(TIntrusivePtr<TRichRequestNode> subTree, TThesaurusExtensionId type,
                     double relev = 0, EFormClass match = EQUAL_BY_SYNONYM);

        TSynonymData(const TSynonymData& synonym);

        // making synonym tree directly from synonym text
        TSynonymData(const TUtf16String& synonymText, const TCreateTreeOptions& options, TThesaurusExtensionId type,
                     double relev = 0, EFormClass match = EQUAL_BY_SYNONYM);

        virtual ~TSynonymData() {
        }

        bool operator ==(const TSynonymData& s) const;

        bool HasType(TThesaurusExtensionId type) const {
            return Type & type;
        }

        TThesaurusExtensionId GetType() const {
            return Type;
        }

        void AddType(TThesaurusExtensionId type) {
            Type |= type;
        }

        void RemoveType(TThesaurusExtensionId type) {
            Type &= ~type;
        }

        EFormClass GetBestFormClass() const {
            return BestFormClass;
        }

        void SetBestFormClass(EFormClass match) {
            BestFormClass = match;
        }

        double GetRelev() const {
            return Relev;
        }

        bool HasSameRichTree(const TRichRequestNode& tree) const;

        bool MergeData(TSynonymData& roommate);

        bool Serialize(NRichTreeProtocol::TMarkupDataBase& message, bool humanReadable) const;
        static TSynonymData Deserialize(const ::NRichTreeProtocol::TMarkupDataBase& message, EQtreeDeserializeMode mode);
    };

    template<class TDerived, EMarkupType SynType>
    class TSynonymBase: public TMarkupData<SynType>, public TSynonymData {
    private:
        typedef TMarkupData<SynType> TBase;
        typedef TSynonymBase<TDerived, SynType> TThis;
    public:
        explicit TSynonymBase(const TSynonymData& synonym)
            : TBase()
            , TSynonymData(synonym)
        {
        }

        ~TSynonymBase() override {
        }

        // Return a synonym corresponding to i-th child of current synonym sub-tree.
        TAutoPtr<TDerived> SplitChild(size_t i) {
            TAutoPtr<TDerived> ret = CheckedCast<TDerived*>(Clone().Release());
            ret->SubTree = ret->SubTree->Children[i];
            return ret;
        }

        bool operator ==(const TThis& s) const {
            return static_cast<const TBase&>(*this) == static_cast<const TBase&>(*s)
                && static_cast<const TSynonymData&>(*this) == static_cast<const TSynonymData&>(*s);
        }

        bool Merge(TMarkupDataBase& roommate) override {
            return MergeData(roommate.As<TThis>());
        }

        TMarkupDataPtr Clone() const override {
            return new TDerived(*this);
        }

        bool Serialize(NRichTreeProtocol::TMarkupDataBase& message, bool humanReadable) const override {
            return TSynonymData::Serialize(message, humanReadable);
        }
        static TMarkupDataPtr Deserialize(const NRichTreeProtocol::TMarkupDataBase& message, EQtreeDeserializeMode mode) {
            return new TDerived(TSynonymData::Deserialize(message, mode));
        }

    private:
        bool DoEqualTo(const TMarkupDataBase& rhs) const override {
            return static_cast<TSynonymData>(*this) == rhs.As<TThis>();
        }
    };
} // NSearchQuery

class TSynonym: public NSearchQuery::TSynonymBase<TSynonym, NSearchQuery::MT_SYNONYM> {
private:
    typedef NSearchQuery::TSynonymBase<TSynonym, NSearchQuery::MT_SYNONYM> TBase;
public:
    TSynonym(TIntrusivePtr<TRichRequestNode> subTree, TThesaurusExtensionId type,
             double relev = 0, EFormClass match = EQUAL_BY_SYNONYM)
        : TBase(TSynonymData(subTree, type, relev, match))
    {
    }

    // making synonym tree directly from synonym text
    TSynonym(const TUtf16String& synonymText, const TCreateTreeOptions& options, TThesaurusExtensionId type)
        : TBase(TSynonymData(synonymText, options, type))   // using default values for relev and match-type
    {
    }

    explicit TSynonym(const TSynonymData& synonym)
        : TBase(synonym)
    {
    }

    bool Serialize(NRichTreeProtocol::TMarkupDataBase& message, bool humanReadable) const override;
};

using TSynonymPtr = TIntrusivePtr<TSynonym>;

class TTechnicalSynonym: public NSearchQuery::TSynonymBase<TTechnicalSynonym, NSearchQuery::MT_TECHNICAL_SYNONYM> {
private:
    typedef NSearchQuery::TSynonymBase<TTechnicalSynonym, NSearchQuery::MT_TECHNICAL_SYNONYM> TBase;
public:
    TTechnicalSynonym(TIntrusivePtr<TRichRequestNode> subTree, TThesaurusExtensionId type, double relev = 0, EFormClass match = EQUAL_BY_SYNONYM)
        : TBase(TSynonymData(subTree, type, relev, match))
    {
    }

    // making synonym tree directly from synonym text
    TTechnicalSynonym(const TUtf16String& synonymText, const TCreateTreeOptions& options, TThesaurusExtensionId type)
        : TBase(TSynonymData(synonymText, options, type))   // using default values for relev and match-type
    {
    }

    explicit TTechnicalSynonym(const TSynonymData& synonym)
        : TBase(synonym)
    {
    }
};

namespace NSearchQuery {
    struct TSynonymTypeCheck {
        const TThesaurusExtensionId Type;

        TSynonymTypeCheck(TThesaurusExtensionId type)
            : Type(type)
        {
        }

        bool operator () (const TMarkupItem& syn) {
            return syn.GetDataAs<TSynonym>().GetType() == Type;
        }
    };
}
