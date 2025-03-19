#include "markup.h"

#include "synonym.h"
#include "fiomarkup.h"
#include "ontotypemarkup.h"
#include "socialmarkup.h"
#include "protobufmarkup.h"
#include "syntaxmarkup.h"

#include <kernel/qtree/richrequest/nodeiterator.h>
#include <kernel/qtree/richrequest/protos/rich_tree.pb.h>
#include <kernel/qtree/richrequest/richnode.h>
#include <library/cpp/langmask/serialization/langmask.h>

using namespace NSearchQuery;

namespace {
    bool HasText(const TRequestNodeBase& n) {
        return !n.GetText().empty();
    }

#define CMP(a, b)\
    if ((a) < (b)) return true; \
    if ((b) < (a)) return false;

#define CMP2(a, b)\
    if ((a) < (b)) return -1; \
    if ((b) < (a)) return 1;

    struct TDeepItemOrder {
        bool operator ()(const TMarkupItem& a, const TMarkupItem& b) {
            CMP(a.Range, b.Range);

            if (a.Data->MarkupType() == MT_SYNONYM)
                return CmpSyn(a.GetDataAs<TSynonym>(), b.GetDataAs<TSynonym>());

            if (a.Data->MarkupType() == MT_TECHNICAL_SYNONYM)
                return CmpSyn(a.GetDataAs<TTechnicalSynonym>(), b.GetDataAs<TTechnicalSynonym>());

            return false;
        }


        int CmpText(const TRequestNodeBase& a, const TRequestNodeBase& b) {
            CMP2(a.GetTextName(), b.GetTextName());
            CMP2(a.GetText(), b.GetText());
            CMP2(a.GetAttrValueHi(), b.GetAttrValueHi());
            return 0;
        }

        int CmpText(TRichNodePtr a, TRichNodePtr b) {
            typedef TTIterator<TForwardChildrenTraversal, HasText> TWordInfoIterator;
            TWordInfoIterator ita(a);
            TWordInfoIterator itb(b);
            for (;;++ita, ++itb) {
                if (ita.IsDone() && itb.IsDone())
                    return 0;
                if (itb.IsDone())
                    return 1;
                if (ita.IsDone())
                    return -1;
                int c = CmpText(*a, *b);
                if (c)
                    return c;
            }
        }

        template<class TSyn>
        bool CmpSyn(const TSyn& a, const TSyn& b) {
            int c = CmpText(a.SubTree, b.SubTree);
            if (c < 0)
                return true;
            if (c > 0)
                return false;

            if (a.GetBestFormClass() < b.GetBestFormClass())
                return true;
            if (b.GetBestFormClass() < a.GetBestFormClass())
                return false;

            if (a.GetType() < b.GetType())
                return true;
            if (b.GetType() < a.GetType())
                return false;

            return false;
        }
    };
}

#undef CMP
#undef CMP2

void TMarkup::DeepSort() {
    for (auto& item : ItemsArray)
        ::StableSort(item.begin(), item.end(), TDeepItemOrder());
}

#define SERIALIZE_OLD_SYNONYMS

//// TSynonym
static void SerializeSynonym(const TSynonymData& sin, NRichTreeProtocol::TSynonym& message, bool humanReadable) {
    message.SetType(sin.GetType());
    message.SetRelev((float)sin.GetRelev());
    message.SetBestFormClass((i32)sin.GetBestFormClass());
    Serialize(*sin.SubTree, *message.MutableSubTree(), humanReadable);
}

bool TSynonym::Serialize(NRichTreeProtocol::TMarkupDataBase& message, bool humanReadable) const {
    #ifdef SERIALIZE_OLD_SYNONYMS
        Y_UNUSED(message);
        Y_UNUSED(humanReadable);
        return false;
    #else
        return TSynonymData::Serialize(message, humanReadable);
    #endif
}

namespace NSearchQuery {
    bool TSynonymData::Serialize(NRichTreeProtocol::TMarkupDataBase& message, bool humanReadable) const {
        ::SerializeSynonym(*this, *message.MutableSynonym(), humanReadable);
        return true;
    }

    static inline TSynonymData DeserializeSynonym(const NRichTreeProtocol::TSynonym& message, EQtreeDeserializeMode mode) {
        TRichNodePtr subTree = ::Deserialize(message.GetSubTree(), mode);
        EFormClass match = message.HasBestFormClass() ? (EFormClass) message.GetBestFormClass() : EQUAL_BY_SYNONYM;
        return TSynonymData(subTree, message.GetType(), message.GetRelev(), match);
    }

    TSynonymData TSynonymData::Deserialize(const NRichTreeProtocol::TMarkupDataBase& message, EQtreeDeserializeMode mode) {
        return DeserializeSynonym(message.GetSynonym(), mode);
    }
} //NSearchQuery

// TFIOMarkup::TField
static void Serialize(NRichTreeProtocol::TFioMarkup_TField& to, const TFioMarkup::TField& from, bool ) {
    to.MutablePos()->SetBegin(from.Pos.Beg);
    to.MutablePos()->SetEnd(from.Pos.End);
    for (size_t i = 0; i < from.Parts.size(); ++i)
        *to.AddParts() = WideToUTF8(from.Parts[i]);
}

static void Deserialize(TFioMarkup::TField& to, const NRichTreeProtocol::TFioMarkup_TField& from) {
    to.Pos.Beg = from.GetPos().GetBegin();
    to.Pos.End = from.GetPos().GetEnd();
    for (size_t i = 0; i < from.PartsSize(); ++i)
        to.Parts.push_back(UTF8ToWide(from.GetParts(i)));
}

// TFIOMarkup
bool TFioMarkup::Serialize(NRichTreeProtocol::TMarkupDataBase& message, bool humanReadable) const {
    NRichTreeProtocol::TFioMarkup* m = message.MutableFioMarkup();

    m->SetType(FioType);
    ::Serialize(*m->MutableFirstNameField(), FirstNameField, humanReadable);
    ::Serialize(*m->MutableSurnameField(),   SurnameField,   humanReadable);
    for (size_t i = 0; i < SecondNameFields.size(); ++i)
        ::Serialize(*m->AddSecondNameFields(), SecondNameFields[i], humanReadable);
    ::Serialize(*(m->MutableLanguage()), Language, humanReadable);
    m->SetIsSurnameReliable(IsSurnameReliable);
    return true;
}

TMarkupDataPtr TFioMarkup::Deserialize(const NRichTreeProtocol::TMarkupDataBase& message) {
    Y_ASSERT(message.GetType() == SType);
    const NRichTreeProtocol::TFioMarkup& m = message.GetFioMarkup();
    THolder<TFioMarkup> ret(new TFioMarkup());

    ret->FioType = static_cast<EFIOType>(m.GetType());

    if (m.HasLanguage())
        ret->Language = ::Deserialize(m.GetLanguage());
    else
        ret->Language = TLangMask(LANG_RUS);

    if (m.HasFirstNameField())
        ::Deserialize(ret->FirstNameField, m.GetFirstNameField());

    if (m.HasSurnameField())
        ::Deserialize(ret->SurnameField, m.GetSurnameField());

    ret->SecondNameFields.resize(m.SecondNameFieldsSize());
    for (size_t i = 0; i < m.SecondNameFieldsSize(); ++i) {
        ::Deserialize(ret->SecondNameFields[i], m.GetSecondNameFields(i));
    }

    ret->IsSurnameReliable = m.GetIsSurnameReliable();

    return ret.Release();
}

//// TOntoTypeMarkup

bool TOntoTypeMarkup::Serialize(NRichTreeProtocol::TMarkupDataBase& message, bool humanReadable) const
{
    Y_UNUSED(humanReadable);

    NRichTreeProtocol::TOntoMarkup* m = message.MutableOnto();

    m->SetType(static_cast<ui32>(Type));
    m->SetWeight(Weight);
    m->SetOne(One);
    m->SetIntent(static_cast<ui32>(Intent));
    m->SetIntWght(IntWght);
    return true;
}

TMarkupDataPtr TOntoTypeMarkup::Deserialize(const NRichTreeProtocol::TMarkupDataBase& message) {
    Y_ASSERT(message.GetType() == SType);
    const NRichTreeProtocol::TOntoMarkup& m = message.GetOnto();
    return new TOntoTypeMarkup(static_cast<EOntoCatsType>(m.GetType()), m.GetWeight(), m.GetOne(), static_cast<EOntoIntsType>(m.GetIntent()), m.GetIntWght());
}

//// TSocialMarkup

bool TSocialMarkup::Serialize(NRichTreeProtocol::TMarkupDataBase& message, bool /*humanReadable*/) const {
    message.SetSocialMarkup(static_cast<ui32>(Type));
    return true;
}

TMarkupDataPtr TSocialMarkup::Deserialize(const NRichTreeProtocol::TMarkupDataBase& message) {
    Y_ASSERT(message.GetType() == SType);
    return new TSocialMarkup(static_cast<ESocialOnto>(message.GetSocialMarkup()));
}

//// TSyntaxMarkup

bool TSyntaxMarkup::Serialize(NRichTreeProtocol::TMarkupDataBase& message, bool /*humanReadable*/) const {
    NRichTreeProtocol::TSyntaxMarkup* m = message.MutableSyntaxMarkup();

    m->SetLevel(Level);
    *m->MutableSyntaxType() = WideToUTF8(SyntaxType);
    return true;
}

TMarkupDataPtr TSyntaxMarkup::Deserialize(const NRichTreeProtocol::TMarkupDataBase& message) {
    Y_ASSERT(message.GetType() == SType);
    const NRichTreeProtocol::TSyntaxMarkup& m = message.GetSyntaxMarkup();
    return new TSyntaxMarkup(UTF8ToWide(m.GetSyntaxType()), m.GetLevel());
}


//// General
static TAutoPtr<NRichTreeProtocol::TMarkupItem> Serialize(const TMarkupItem& mi, bool humanReadable) {
    TAutoPtr<NRichTreeProtocol::TMarkupItem> message(new NRichTreeProtocol::TMarkupItem);
    if (!mi.Data->Serialize(*message->MutableData(), humanReadable))
        return nullptr;

    message->MutableData()->SetType(mi.Data->MarkupType());
    message->MutableRange()->SetBegin(mi.Range.Beg);
    message->MutableRange()->SetEnd(mi.Range.End);

    return message;
}

#define CASE_DESERIALIZE_MARKUP(type, ...) case type::SType: return type::Deserialize(message, ##__VA_ARGS__)

static TMarkupDataPtr Deserialize(const NRichTreeProtocol::TMarkupDataBase& message, EQtreeDeserializeMode mode) {
    switch (message.GetType()) {
        CASE_DESERIALIZE_MARKUP(TSynonym, mode);
        CASE_DESERIALIZE_MARKUP(TTechnicalSynonym, mode);
        CASE_DESERIALIZE_MARKUP(TFioMarkup);
        CASE_DESERIALIZE_MARKUP(TOntoTypeMarkup);
        CASE_DESERIALIZE_MARKUP(TSocialMarkup);
        //CASE_DESERIALIZE_MARKUP(TProtobufMarkup);
        //CASE_DESERIALIZE_MARKUP(TGztMarkup);
        CASE_DESERIALIZE_MARKUP(TGeoAddrMarkup);
        CASE_DESERIALIZE_MARKUP(TSyntaxMarkup);
        //CASE_DESERIALIZE_MARKUP(TMangoMarkup);
        CASE_DESERIALIZE_MARKUP(TDateMarkup);
        CASE_DESERIALIZE_MARKUP(TVinsMarkup);
        CASE_DESERIALIZE_MARKUP(TQSegmentsMarkup);
        CASE_DESERIALIZE_MARKUP(TTransitMarkup);
        CASE_DESERIALIZE_MARKUP(TUrlMarkup);
        default: return nullptr;
    }
}

#undef CASE_DESERIALIZE_MARKUP

void SerializeMarkup(const TRichRequestNode& node, NRichTreeProtocol::TRichRequestNode& message, bool humanReadable) {
    for (TAllMarkupConstIterator i(node.Markup()); !i.AtEnd(); ++i) {
        TAutoPtr<NRichTreeProtocol::TMarkupItem> res = Serialize(*i, humanReadable);
        if (res.Get()) {
            NRichTreeProtocol::TMarkupItem* markup = message.AddMarkup();
            markup->Swap(res.Get());
        }
    }

#ifdef SERIALIZE_OLD_SYNONYMS
    //for backward compatibility, please remove this block after the all production components will be relized
    for (TForwardMarkupIterator<TSynonym, true> i(node.Markup()); !i.AtEnd(); ++i) {
        NRichTreeProtocol::TSynonym* synmess = message.AddSynonyms();
        SerializeSynonym(i.GetData(), *synmess, humanReadable);
        NRichTreeProtocol::TRange* r = synmess->MutableRange();
        r->SetBegin(i->Range.Beg);
        r->SetEnd(i->Range.End);
    }
#endif
}

void DeserializeMarkup(const NRichTreeProtocol::TRichRequestNode& message, TRichRequestNode& node, EQtreeDeserializeMode mode) {
    bool hasSynonymMarkup = false;

    for (const auto& mimess : message.GetMarkup()) {
        TMarkupDataPtr data = Deserialize(mimess.GetData(), mode);
        if (!data)
            continue;

        const auto& rm = mimess.GetRange();
        node.MutableMarkup().Add(TMarkupItem(TRange(rm.GetBegin(), rm.GetEnd()), data));

        if (data->MarkupType() == MT_SYNONYM)
            hasSynonymMarkup = true;
    }

    //for backward compatibility, please remove this block after the all production components will be relized
    if (!hasSynonymMarkup) {
        for (const auto& synmess : message.GetSynonyms()) {
            const auto& rm = synmess.GetRange();
            TMarkupDataPtr data = new TSynonym(DeserializeSynonym(synmess, mode));
            node.MutableMarkup().Add(TMarkupItem(TRange(rm.GetBegin(), rm.GetEnd()), data));
        }
    }
}


#define IMPLEMENT_PROTOBUF_MARKUP(TMarkupType, Name)                                        \
    TMarkupType::TMarkupType(NRichTreeProtocol::TMarkupType* msg)                           \
        : TBase(msg ? msg : new NRichTreeProtocol::TMarkupType)                             \
    {                                                                                       \
    }                                                                                       \
    bool TMarkupType::Serialize(NRichTreeProtocol::TMarkupDataBase& msg, bool) const {      \
        /* simple copying */                                                                \
        msg.Mutable##Name##Markup()->CopyFrom(Data());                                      \
        return true;                                                                        \
    }                                                                                       \
    TMarkupDataPtr TMarkupType::Deserialize(const NRichTreeProtocol::TMarkupDataBase& msg) {\
        /* simple copying */                                                                \
        const auto& inputMarkup = msg.Get##Name##Markup();                                  \
        return new TMarkupType(new NRichTreeProtocol::TMarkupType(inputMarkup));            \
    }


IMPLEMENT_PROTOBUF_MARKUP(TGeoAddrMarkup,   GeoAddr);
IMPLEMENT_PROTOBUF_MARKUP(TDateMarkup,      Date);
IMPLEMENT_PROTOBUF_MARKUP(TQSegmentsMarkup, QSegments);
IMPLEMENT_PROTOBUF_MARKUP(TTransitMarkup,   Transit);
IMPLEMENT_PROTOBUF_MARKUP(TUrlMarkup,       Url);


#undef IMPLEMENT_PROTOBUF_MARKUP


