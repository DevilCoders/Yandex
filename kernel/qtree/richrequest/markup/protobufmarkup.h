#pragma once

#include "markup.h"
#include <kernel/qtree/richrequest/markup/protos/markup.pb.h>

#include <library/cpp/protobuf/util/is_equal.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>

#include <util/generic/hash.h>
#include <util/generic/set.h>


template <NSearchQuery::EMarkupType T, typename TDerivedMarkup, typename TProto = NProtoBuf::Message>
class TProtobufMarkupBase: public NSearchQuery::TMarkupData<T> {
public:
    typedef TProtobufMarkupBase<T, TDerivedMarkup, TProto> TSelf;

    TProtobufMarkupBase(TProto* data)
        : Data_(data)    // take ownership
    {
        Y_ASSERT(data != nullptr);
    }

    inline bool operator == (const TSelf& rhs) const {
        return NProtoBuf::IsEqualDefault(*Data_, *rhs.Data_);
    }

    bool Merge(NSearchQuery::TMarkupDataBase& markupData) override {
        return DoEqualTo(markupData);
    }

    NSearchQuery::TMarkupDataPtr Clone() const override {
        THolder<TProto> clone(Data_->New());
        clone->CopyFrom(*Data_);
        return new TDerivedMarkup(clone.Release());
    }

    const TProto& Data() const {
        return *Data_;
    }

    TProto& Data() {
        return *Data_;
    }

protected:
    bool DoEqualTo(const NSearchQuery::TMarkupDataBase& markupData) const override {
        const TSelf* proto = dynamic_cast<const TSelf*>(&markupData);
        return proto != nullptr && *this == *proto;
    }

private:
    TAtomicSharedPtr<TProto> Data_;
};

// Yari markup, based on TProtobufMarkup but with its own EMarkupType id and without serialization.
class TYariMarkup: public TProtobufMarkupBase<NSearchQuery::MT_YARI, TYariMarkup> {
    typedef TProtobufMarkupBase<NSearchQuery::MT_YARI, TYariMarkup> TBase;
public:
    TYariMarkup(NProtoBuf::Message* message)
        : TBase(message)    // take ownership
    {
    }

    bool Serialize(NRichTreeProtocol::TMarkupDataBase&, bool) const override {
        return false;
    }

    static NSearchQuery::TMarkupDataPtr Deserialize(const NRichTreeProtocol::TMarkupDataBase&) {
        return nullptr;
    }

    THashMap< TString, TSet<ui32> > IndexesInFavourites;
    TString RuleName;
};

// Vins markup, based on TProtobufMarkup but with its own EMarkupType id and without serialization.
class TVinsMarkup: public TProtobufMarkupBase<NSearchQuery::MT_VINS, TVinsMarkup> {
    typedef TProtobufMarkupBase<NSearchQuery::MT_VINS, TVinsMarkup> TBase;
public:

    TVinsMarkup(NProtoBuf::Message* message)
        : TBase(message)    // take ownership
    {
    }

    bool Serialize(NRichTreeProtocol::TMarkupDataBase&, bool) const override {
        return false;
    }

    static NSearchQuery::TMarkupDataPtr Deserialize(const NRichTreeProtocol::TMarkupDataBase&) {
        return nullptr;
    }

    TString RuleName;
};


// Macro for standard markup implementation

#define DECLARE_PROTOBUF_MARKUP(TMarkupType, EnumId)                                                        \
    class TMarkupType: public TProtobufMarkupBase<EnumId, TMarkupType, NRichTreeProtocol::TMarkupType> {    \
        using TBase = TProtobufMarkupBase<EnumId, TMarkupType, NRichTreeProtocol::TMarkupType>;             \
    public:                                                                                                 \
        TMarkupType(NRichTreeProtocol::TMarkupType* message = nullptr);                     \
        bool Serialize(NRichTreeProtocol::TMarkupDataBase& message, bool) const override;   \
        static NSearchQuery::TMarkupDataPtr Deserialize(                                    \
            const NRichTreeProtocol::TMarkupDataBase& message);                             \
    }


DECLARE_PROTOBUF_MARKUP(TGeoAddrMarkup,     NSearchQuery::MT_GEOADDR);
DECLARE_PROTOBUF_MARKUP(TDateMarkup,        NSearchQuery::MT_DATE);
DECLARE_PROTOBUF_MARKUP(TQSegmentsMarkup,   NSearchQuery::MT_QSEGMENTS);
DECLARE_PROTOBUF_MARKUP(TTransitMarkup,     NSearchQuery::MT_TRANSIT);
DECLARE_PROTOBUF_MARKUP(TUrlMarkup,         NSearchQuery::MT_URL);

#undef DECLARE_PROTOBUF_MARKUP


