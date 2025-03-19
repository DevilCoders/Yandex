#pragma once
#include <kernel/embeds_bundle/protos/embeds_bundle_transport.pb.h>

#include <util/generic/vector.h>

namespace NSearchQuery {

struct TEmbedsBundle {
    TVector<TVector<float>> Embeds;
    TVector<float> Weights;
    ui32 LbExpansionTypeTag = 0;
};

TEmbedsBundleProto SerializeEmbedsBundle(const TEmbedsBundle& bundle, EEmbedsSerializtionAlgo algo);
TEmbedsBundle DeserializeEmbedsBundle(const TEmbedsBundleProto& transport);

TString SerializeEmbedsBundleTransport(const TEmbedsBundleTransport& transport);
TEmbedsBundleTransport DeserializeEmbedsBundle(TStringBuf data);

constexpr TStringBuf DefaultEmbedBundleRelevFieldName = "embeds_bundle";

}
