#include "serializer.h"

#include <kernel/prs_log/serializer/uniform_bound/proto/compressed.pb.h>
#include <kernel/prs_log/serializer/common/utils.h>
#include <kernel/dssm_applier/utils/utils.h>
#include <library/cpp/protobuf/util/pb_io.h>

#include <util/generic/algorithm.h>
#include <util/generic/array_ref.h>
#include <util/generic/hash.h>
#include <util/generic/xrange.h>
#include <util/generic/ymath.h>
#include <util/stream/output.h>

namespace {
    using namespace NPrsLog;
    using namespace UniformBoundCompressionProto;
    using namespace NDssmApplier::NUtils;
    /**
     * Reads the src (single document slice) and append it transposed to the dst (as a column).
     * After this function processes all the documents we have a matrix (n_features, n_documents).
     */
    void UpdateTransposedFeaturesStep(const TVector<float>& src, TVector<TVector<float>>& dst) {
        Y_ENSURE(src.size() == dst.size());

        for (size_t i : xrange(src.size())) {
            dst[i].push_back(src[i]);
        }
    }

    // Compression

    TCompressedFactor CompressFeatures(const TVector<float>& src) {
        Y_ENSURE(src);
        TString compressed;
        TSingleStatCompressionMeta meta;
        const float min = *MinElement(src.begin(), src.end());
        const float max = *MaxElement(src.begin(), src.end());
        meta.SetMinValue(min);

        if (!FuzzyEquals(min, max)) {
            meta.SetMaxValue(max);
            TVector<ui8> result = Compress(src, min, max);
            compressed = TString(reinterpret_cast<const char*>(result.data()), result.size());
        }

        TCompressedFactor factor;
        factor.MutableMeta()->CopyFrom(meta);
        if (compressed) {
            factor.SetCompressedFeatures(compressed);
        }
        return factor;
    }

    void UpdateCompressedMessage(const TVector<TVector<float>>& source, const TString& slice, TCompressedWebData& message) {
        for (const TVector<float>& features : source) {

            const auto* fieldDescriptor = message.GetDescriptor()->FindFieldByName(slice);
            Y_ENSURE(fieldDescriptor, slice + " not found in the TCompressedWebData");
            auto* factorsPtr = message.GetReflection()->MutableRepeatedPtrField<TCompressedFactor>(&message, fieldDescriptor);
            Y_ENSURE(factorsPtr, "MutableRepeatedPtrField not extracted for " + slice);

            auto* addedFactor = factorsPtr->Add();
            *addedFactor = CompressFeatures(features);
        }
    }

    // Decompression

    TVector<float> DecompressFactorForPRS(const TCompressedFactor& factor, const size_t numDocs) {
        TVector<float> features;
        TSingleStatCompressionMeta meta = factor.GetMeta();
        const float min = meta.GetMinValue();

        if (meta.HasMaxValue() && !FuzzyEquals(min, meta.GetMaxValue())) {
            TString compressed = factor.GetCompressedFeatures();
            Y_ENSURE(compressed.size() == numDocs);

            const float max = meta.GetMaxValue();
            TArrayRef<const ui8> compressedFeatures(reinterpret_cast<const ui8*>(compressed.data()), compressed.size());
            features = Decompress(compressedFeatures, GenerateSingleDecompression(min, max));
        } else {
            features.resize(numDocs, min);
        }
        return features;
    }

    /**
     * Return shape is (n_features, n_documents).
     * Call it in the same order as you've dumped the slices.
     */
    TVector<TVector<float>> GetDecompressedSlice(const TCompressedWebData& message,
        const TString& sliceName, const size_t numDocs) {

        const auto* fieldDescriptor = message.GetDescriptor()->FindFieldByName(sliceName);
        const auto& sliceFactors = message.GetReflection()->GetRepeatedPtrField<TCompressedFactor>(message, fieldDescriptor);

        TVector<TVector<float>> result(Reserve(sliceFactors.size()));
        for (int i = 0; i < sliceFactors.size(); ++i) {
            result.push_back(DecompressFactorForPRS(sliceFactors.Get(i), numDocs));
        }
        return result;
    }
}

namespace NPrsLog {
    TString TUniformBoundSerializer::Serialize(const TWebData& data) const {
        if (data.Documents.empty()) {
            return TString();
        }

        TVector<TVector<float>> statistics(1, TVector<float>(Reserve(data.Documents.size())));

        TVector<TVector<float>> webFeatures(
            data.Documents.front().WebFeatures.size(),
            TVector<float>(Reserve(data.Documents.size()))
        );
        TVector<TVector<float>> webMetaFeatures(
            data.Documents.front().WebMetaFeatures.size(),
            TVector<float>(Reserve(data.Documents.size()))
        );
        TVector<TVector<float>> rapidClicksFeatures(
            data.Documents.front().RapidClicksFeatures.size(),
            TVector<float>(Reserve(data.Documents.size()))
        );

        TVector<TString> docIds;
        TVector<TString> docOmniTitles;
        TVector<TString> docSerializedFactors;
        TCompressedWebData result;

        for (const TWebDocument& doc : data.Documents) {
            statistics[0].push_back(doc.L2Relevance);

            UpdateTransposedFeaturesStep(doc.WebFeatures, webFeatures);
            UpdateTransposedFeaturesStep(doc.WebMetaFeatures, webMetaFeatures);
            UpdateTransposedFeaturesStep(doc.RapidClicksFeatures, rapidClicksFeatures);

            docIds.push_back(doc.DocId);
            docOmniTitles.push_back(doc.OmniTitle);
            docSerializedFactors.push_back(doc.SerializedFactors);
            *result.AddUrls() = doc.Url;
        }

        UpdateCompressedMessage(statistics, "Statistics", result);
        UpdateCompressedMessage(webFeatures, "WebFactors", result);
        UpdateCompressedMessage(webMetaFeatures, "WebMetaFactors", result);
        UpdateCompressedMessage(rapidClicksFeatures, "RapidClicksFactors", result);

        for (const TString& docId : docIds) {
            *result.AddDocIds() = docId;
        }
        for (const TString& docOmniTitle : docOmniTitles) {
            *result.AddOmniTitles() = docOmniTitle;
        }
        for (const TString& docSerializedFactor : docSerializedFactors) {
            *result.AddSerializedFactors() = docSerializedFactor;
        }
        if (data.WebFeaturesIds) {
            result.SetWebFeaturesIds(CompressFeaturesIds(data.WebFeaturesIds));
        }
        if (data.WebMetaFeaturesIds) {
            result.SetWebMetaFeaturesIds(CompressFeaturesIds(data.WebMetaFeaturesIds));
        }
        if (data.RapidClicksFeaturesIds) {
            result.SetRapidClicksFeaturesIds(CompressFeaturesIds(data.RapidClicksFeaturesIds));
        }
        return result.SerializeAsString();
    }

    TWebData TUniformBoundSerializer::Deserialize(const TString& raw) const {
        if (raw.empty()) {
            return TWebData();
        }

        TCompressedWebData message;
        Y_ENSURE(message.ParseFromString(raw), "Cannot parse TCompressedWebData from TString");

        const size_t numDocs = message.DocIdsSize();
        Y_ENSURE(numDocs == message.UrlsSize());

        TVector<TVector<float>> statistics = GetDecompressedSlice(message, "Statistics", numDocs);
        TVector<TVector<float>> webFeatures = GetDecompressedSlice(message, "WebFactors", numDocs);
        TVector<TVector<float>> webMetaFeatures = GetDecompressedSlice(message, "WebMetaFactors", numDocs);
        TVector<TVector<float>> rapidClicksFeatures = GetDecompressedSlice(message, "RapidClicksFactors", numDocs);

        TWebData result;
        result.Documents.reserve(numDocs);

        for (size_t i = 0; i < numDocs; ++i) {
            TWebDocument document;

            document.DocId = message.GetDocIds(i);
            document.Url = message.GetUrls(i);
            document.OmniTitle = message.GetOmniTitles().empty() ? "" : message.GetOmniTitles(i);
            document.SerializedFactors = message.GetSerializedFactors().empty() ? "" : message.GetSerializedFactors(i);
            document.L2Relevance = statistics[0][i];

            document.WebFeatures.reserve(webFeatures.size());
            for (const auto& prsFeatures : webFeatures) {
                document.WebFeatures.push_back(prsFeatures[i]);
            }

            document.WebMetaFeatures.reserve(webMetaFeatures.size());
            for (const auto& prsFeatures : webMetaFeatures) {
                document.WebMetaFeatures.push_back(prsFeatures[i]);
            }
            document.RapidClicksFeatures.reserve(rapidClicksFeatures.size());
            for (const auto& prsFeatures : rapidClicksFeatures) {
                document.RapidClicksFeatures.push_back(prsFeatures[i]);
            }
            result.Documents.push_back(document);
        }
        if (message.HasWebFeaturesIds()) {
            result.WebFeaturesIds = DecompressFeaturesIds<NSliceWebProduction::EFactorId>(message.GetWebFeaturesIds());
        }
        if (message.HasWebMetaFeaturesIds()) {
            result.WebMetaFeaturesIds = DecompressFeaturesIds<NSliceWebMeta::EFactorId>(message.GetWebMetaFeaturesIds());
        }
        if (message.HasRapidClicksFeaturesIds()) {
            result.RapidClicksFeaturesIds = DecompressFeaturesIds<NRapidClicks::EFactorId>(message.GetRapidClicksFeaturesIds());
        }
        return result;
    }
} // NPrsLog
