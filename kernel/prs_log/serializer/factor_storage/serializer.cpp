#include "serializer.h"

#include <kernel/prs_log/serializer/factor_storage/proto/compressed.pb.h>
#include <kernel/prs_log/serializer/common/utils.h>
#include <kernel/factor_storage/factor_storage.h>
#include <library/cpp/protobuf/util/pb_io.h>

#include <util/generic/algorithm.h>
#include <util/generic/array_ref.h>
#include <util/generic/hash.h>
#include <util/generic/xrange.h>
#include <util/generic/ymath.h>
#include <util/stream/output.h>


namespace NPrsLog {
    const size_t N_SLICES = 2;
    const size_t N_STATS = 1;

    using namespace FactorStorageCompressionProto;

    TString TFactorStorageSerializer::Serialize(const TWebData& data) const {
        if (data.Documents.empty()) {
            return TString();
        }
        TCompressedWebData result;

        for (const TWebDocument& doc : data.Documents) {
            *result.AddDocIds() = doc.DocId;
            *result.AddUrls() = doc.Url;

            TFactorStorage storage(3 + doc.WebFeatures.size() + doc.WebMetaFeatures.size());
            storage[0] = doc.WebFeatures.size();
            storage[1] = doc.WebMetaFeatures.size();
            storage[N_SLICES] = doc.L2Relevance;

            for (size_t i = 0; i < doc.WebFeatures.size(); ++i) {
                storage[N_SLICES + N_STATS + i] = doc.WebFeatures[i];
            }
            for (size_t i = 0; i < doc.WebMetaFeatures.size(); ++i) {
                storage[N_SLICES + N_STATS + doc.WebFeatures.size() + i] = doc.WebMetaFeatures[i];
            }

            TStringStream out;
            NFSSaveLoad::Serialize(storage, &out);
            *result.AddFeatures() = out.Str();
        }

        if (data.WebFeaturesIds) {
            result.SetWebFeaturesIds(CompressFeaturesIds(data.WebFeaturesIds));
        }
        if (data.WebMetaFeaturesIds) {
            result.SetWebMetaFeaturesIds(CompressFeaturesIds(data.WebMetaFeaturesIds));
        }
        return result.SerializeAsString();
    }

    TWebData TFactorStorageSerializer::Deserialize(const TString& raw) const {
        if (raw.empty()) {
            return TWebData();
        }

        TCompressedWebData message;
        Y_ENSURE(message.ParseFromString(raw), "Cannot parse TCompressedWebData from TString");

        const size_t numDocs = message.DocIdsSize();
        Y_ENSURE(numDocs == message.UrlsSize());

        TWebData result;
        result.Documents.reserve(numDocs);

        for (size_t i = 0; i < numDocs; ++i) {
            TWebDocument document;

            document.DocId = message.GetDocIds(i);
            document.Url = message.GetUrls(i);

            TStringStream ss(message.GetFeatures(i));
            NFactorSlices::TSlicesMetaInfo someInfo;
            NFactorSlices::EnableSlices(someInfo, NFactorSlices::EFactorSlice::WEB_PRODUCTION);
            THolder<TFactorStorage> loaded = NFSSaveLoad::Deserialize(&ss, someInfo);

            const size_t nWeb = static_cast<size_t>((*loaded)[0]);
            const size_t nWebMeta = static_cast<size_t>((*loaded)[1]);
            Y_ENSURE(loaded->Size() == N_SLICES + N_STATS + nWeb + nWebMeta);

            document.L2Relevance = (*loaded)[N_SLICES];
            for (size_t i = 0; i < nWeb; ++i) {
                document.WebFeatures.push_back((*loaded)[N_SLICES + N_STATS + i]);
            }
            for (size_t i = 0; i < nWebMeta; ++i) {
                document.WebMetaFeatures.push_back((*loaded)[N_SLICES + N_STATS + nWeb + i]);
            }
            result.Documents.push_back(document);
        }
        if (message.HasWebFeaturesIds()) {
            result.WebFeaturesIds = DecompressFeaturesIds<NSliceWebProduction::EFactorId>(message.GetWebFeaturesIds());
        }
        if (message.HasWebMetaFeaturesIds()) {
            result.WebMetaFeaturesIds = DecompressFeaturesIds<NSliceWebMeta::EFactorId>(message.GetWebMetaFeaturesIds());
        }
        return result;
    }
} // NPrsLog
