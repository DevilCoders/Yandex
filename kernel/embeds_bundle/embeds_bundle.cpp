#include "embeds_bundle.h"

#include <library/cpp/string_utils/base64/base64.h>
#include <kernel/nn_ops/boosting_compression.h>
#include <kernel/lingboost/constants.h>
#include <util/generic/xrange.h>
#include <kernel/dssm_applier/begemot/production_data.h>
#include <kernel/dssm_applier/utils/utils.h>
#include <kernel/dssm_applier/decompression/decompression.h>

NSearchQuery::TEmbedsBundleProto NSearchQuery::SerializeEmbedsBundle(
    const TEmbedsBundle& bundle,
    EEmbedsSerializtionAlgo algo)
{
    Y_ENSURE(algo == EEmbedsSerializtionAlgo::PantherModelContinuesCommonRangeRenormDiscretization);
    Y_ENSURE(bundle.Weights.size() == bundle.Embeds.size());
    NSearchQuery::TEmbedsBundleProto result;
    result.SetSerializationAlgo(EEmbedsSerializtionAlgo::PantherModelContinuesCommonRangeRenormDiscretization);
    *result.MutableEmbedsWeights() = {bundle.Weights.begin(), bundle.Weights.end()};
    result.SetLbExpansionTypeTag(bundle.LbExpansionTypeTag);
    result.SetEmbedsCount(bundle.Embeds.size());

    const size_t expectedDim = NNeuralNetApplier::GetDefaultEmbeddingSize(NNeuralNetApplier::EDssmModel::PantherTerms);
    if (!bundle.Embeds.empty()) {
        size_t dim = bundle.Embeds.front().size();
        Y_ENSURE(dim == expectedDim); //this is true for NDssmApplier::EDssmModelType::PantherTerms
        result.SetEmbedsDimension(dim);

        TString buf;
        buf.resize(dim * bundle.Embeds.size());
        char* regionStart = buf.begin();
        for (auto i : xrange(bundle.Embeds.size())) {
            const auto& e = bundle.Embeds[i];
            Y_ENSURE(e.size() == dim);
            TVector<ui8> tmp = NDssmApplier::Compress(e, NDssmApplier::EDssmModelType::PantherTerms);
            Y_ASSERT(tmp.size() == dim);
            MemCopy(regionStart + i * dim, (char*)tmp.begin(), dim);
        }
        result.SetSeriazliedEmbeds(buf);
    }
    return result;
}

NSearchQuery::TEmbedsBundle NSearchQuery::DeserializeEmbedsBundle(
    const TEmbedsBundleProto& transport)
{
    Y_ENSURE(transport.GetSerializationAlgo() == EEmbedsSerializtionAlgo::PantherModelContinuesCommonRangeRenormDiscretization);
    NSearchQuery::TEmbedsBundle result;
    result.LbExpansionTypeTag = transport.GetLbExpansionTypeTag();

    TArrayRef<const char> region = transport.GetSeriazliedEmbeds();
    Y_ENSURE(transport.GetEmbedsCount() * transport.GetEmbedsDimension() == region.size());
    Y_ENSURE(transport.EmbedsWeightsSize() == transport.GetEmbedsCount());

    result.Embeds.reserve(transport.GetEmbedsCount());
    result.Weights.assign(transport.GetEmbedsWeights().begin(), transport.GetEmbedsWeights().end());

    // we don't write embed's dimension if whey are empty
    if (transport.GetEmbedsCount() == 0) {
        return result;
    }

    size_t dim = transport.GetEmbedsDimension();
    const size_t expectedDim = NNeuralNetApplier::GetDefaultEmbeddingSize(NNeuralNetApplier::EDssmModel::PantherTerms);
    Y_ENSURE(dim == expectedDim); //this is true for NDssmApplier::EDssmModelType::PantherTerms

    const auto& decompression = NDssmApplier::GetDecompression(NDssmApplier::EDssmModelType::PantherTerms);
    for (auto i : xrange(transport.GetEmbedsCount())) {
        result.Embeds.emplace_back(NDssmApplier::NUtils::Decompress(
            {
                (const ui8*) (region.begin() + i * dim),
                (const ui8*)(region.begin() + (i + 1) * dim)
            },
            decompression
        ));
    }
    return result;
}

TString NSearchQuery::SerializeEmbedsBundleTransport(const TEmbedsBundleTransport& transport) {
    TString tmp;
    Y_PROTOBUF_SUPPRESS_NODISCARD transport.SerializeToString(&tmp);
    return Base64EncodeUrl(tmp);
}

NSearchQuery::TEmbedsBundleTransport NSearchQuery::DeserializeEmbedsBundle(TStringBuf data) {
    NSearchQuery::TEmbedsBundleTransport result;
    Y_PROTOBUF_SUPPRESS_NODISCARD result.ParseFromString(Base64Decode(data));
    return result;
}
