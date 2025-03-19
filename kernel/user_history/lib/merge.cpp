#include "merge.h"
#include "embedding_tools.h"

#include <kernel/user_history/lib/embedding_tools.h>
#include <kernel/user_history/user_history.h>

#include <util/generic/algorithm.h>

namespace {
    constexpr NPersonalization::NProto::EModels DEFAULT_MODEL = NPersonalization::NProto::EModels::LogDwelltimeBigrams;

    [[nodiscard]] ::NPersonalization::NProto::TFilteredUserRecords Patch(
        const ::NPersonalization::NProto::TFilteredUserRecords& base,
        const ::NPersonalization::NProto::TFilteredUserRecords& patch,
        const NPersonalization::NProto::TUserHistoryPatch::EUserHistoryPatchLogic patchLogic
            ) {

        // locate oldest element in patch
        const auto getTimestamp = [&base](const auto& item) {
            if (base.GetDescription().HasSortOrderOnMirror()) {
                switch (base.GetDescription().GetSortOrderOnMirror()) {
                    case NPersonalization::NProto::SOM_SORT_BY_REQUEST_TIMESTAMP:
                        return item.GetRequestTimestamp();
                    default:
                        return item.GetTimestamp();
                }
            } else {
                return item.GetTimestamp();
            }
        };

        NPersonalization::TUserHistory userHistory;
        userHistory.LoadFromPBWithFilter(base, DEFAULT_MODEL);
        userHistory.Clear();
        if ((base.RecordsSize() == 0) || (patch.RecordsSize() == 0)) {
            // if at least one container is empty we simply copy all records from remaining one
            const auto& container = (base.RecordsSize() == 0) ? patch : base;
            for (const auto& record: container.GetRecords()) {
                auto userHistoryRecord = NPersonalization::TUserHistoryRecord::ConstructFromPB(record, DEFAULT_MODEL);
                userHistory.PushBack(userHistoryRecord);
            }
        } else {
            switch (patchLogic) {
                case NPersonalization::NProto::TUserHistoryPatch::PreferDataFromPatch: {
                    auto patchMaxTsIt = MaxElementBy(patch.GetRecords().begin(), patch.GetRecords().end(), getTimestamp);
                    for (const auto& record: base.GetRecords()) {
                        if (getTimestamp(record) <= getTimestamp(*patchMaxTsIt)) {
                            continue;
                        }
                        auto userHistoryRecord = NPersonalization::TUserHistoryRecord::ConstructFromPB(record, DEFAULT_MODEL);
                        userHistory.PushBack(userHistoryRecord);
                    }

                    for (const auto& record: patch.GetRecords()) {
                        auto userHistoryRecord = NPersonalization::TUserHistoryRecord::ConstructFromPB(record, DEFAULT_MODEL);
                        userHistory.PushBack(userHistoryRecord);
                    }
                    break;
                }
                case NPersonalization::NProto::TUserHistoryPatch::PreferDataFromBase: {
                    auto baseMinTsIt = MinElementBy(base.GetRecords().begin(), base.GetRecords().end(), getTimestamp);
                    for (const auto& record: patch.GetRecords()) {
                        if (getTimestamp(record) >= getTimestamp(*baseMinTsIt)) {
                            continue;
                        }
                        auto userHistoryRecord = NPersonalization::TUserHistoryRecord::ConstructFromPB(record, DEFAULT_MODEL);
                        userHistory.PushBack(userHistoryRecord);
                    }

                    for (const auto& record: base.GetRecords()) {
                        auto userHistoryRecord = NPersonalization::TUserHistoryRecord::ConstructFromPB(record, DEFAULT_MODEL);
                        userHistory.PushBack(userHistoryRecord);
                    }
                    break;
                }

            }
        }

        // regardless of data origin ensure we got rid of excessive data
        userHistory.SortByTimestamp();
        userHistory.Truncate();

        ::NPersonalization::NProto::TFilteredUserRecords result;
        // Will restore required sort order
        userHistory.SaveToMirror(result);
        return result;
    }

    [[nodiscard]] ::google::protobuf::RepeatedPtrField<::NPersonalization::NProto::TFilteredUserRecords> Patch(
        const ::google::protobuf::RepeatedPtrField<::NPersonalization::NProto::TFilteredUserRecords>& base,
        const ::google::protobuf::RepeatedPtrField<::NPersonalization::NProto::TFilteredUserRecords>& patch,
        const NPersonalization::NProto::TUserHistoryPatch::EUserHistoryPatchLogic patchLogic
    ) {
        ::google::protobuf::RepeatedPtrField<NPersonalization::NProto::TFilteredUserRecords> result;

        auto sanitizeFilteredUserRecords = [](const NPersonalization::NProto::TFilteredUserRecords filteredUserRecords) {
            NPersonalization::TUserHistory userHistory;
            userHistory.LoadFromPBWithFilter(filteredUserRecords, DEFAULT_MODEL);
            userHistory.SortByTimestamp();
            userHistory.Truncate();

            // Will restore required sort order
            NPersonalization::NProto::TFilteredUserRecords result;
            userHistory.SaveToMirror(result);
            return result;
        };

        for (auto& baseRecords: base) {
            // is there matched records in patch?
            auto matchedPatchIt = FindIf(patch.begin(), patch.end(),
                [&baseRecords](const auto& patchRecords) {
                    return NPersonalization::CompareUserRecordsDescriptions(baseRecords.GetDescription(), patchRecords.GetDescription());
                }
            );
            if (matchedPatchIt != patch.end()) {
                auto newRecords = result.Add();
                newRecords->CopyFrom(Patch(baseRecords, *matchedPatchIt, patchLogic));
            } else {
                auto newRecords = result.Add();
                newRecords->CopyFrom(sanitizeFilteredUserRecords(baseRecords));
            }
        }

        for (auto& patchRecords: patch) {
            // is there matched records in patch?
            auto matchedBaseIt = FindIf(base.begin(), base.end(),
                [&patchRecords](const auto& baseRecords) {
                    return NPersonalization::CompareUserRecordsDescriptions(baseRecords.GetDescription(), patchRecords.GetDescription());
                }
            );

            if (matchedBaseIt == base.end()) {
                auto newRecords = result.Add();
                newRecords->CopyFrom(sanitizeFilteredUserRecords(patchRecords));
            }
        }

        return result;
    }

    [[nodiscard]] ::google::protobuf::RepeatedPtrField<::NPersonalization::NProto::TUserHistoryRecord> Patch(
        const ::google::protobuf::RepeatedPtrField<::NPersonalization::NProto::TUserHistoryRecord>& base,
        const ::google::protobuf::RepeatedPtrField<::NPersonalization::NProto::TUserHistoryRecord>& patch,
        const NPersonalization::NProto::TUserHistoryPatch::EUserHistoryPatchLogic patchLogic
    ) {
        ::google::protobuf::RepeatedPtrField<NPersonalization::NProto::TUserHistoryRecord> result;
        const auto getTimestamp = [](const auto& item) {
            return item.GetTimestamp();
        };

        if (base.empty() || patch.empty()) {
            result.CopyFrom(base.empty() ? patch : base);
            SortBy(result, getTimestamp);
            return result;
        }

        using namespace NPersonalization;
        switch (patchLogic) {
            case NPersonalization::NProto::TUserHistoryPatch::PreferDataFromPatch: {
                // all elements from patch are always included
                result.CopyFrom(patch);

                // locate oldest element in patch
                auto patchMaxTsIt = MaxElementBy(patch.begin(), patch.end(), getTimestamp);

                for (const auto& item: base) {
                    // copy newest elements from base
                    if (item.GetTimestamp() > patchMaxTsIt->GetTimestamp()) {
                        auto newRecord = result.Add();
                        newRecord->CopyFrom(item);
                    }
                }
                break;
            }
            case NPersonalization::NProto::TUserHistoryPatch::PreferDataFromBase: {
                // locate newest element in base
                auto baseMinTsIt = MinElementBy(base.begin(), base.end(), getTimestamp);

                for (const auto& item: patch) {
                    // copy newest elements from base
                    if (item.GetTimestamp() < baseMinTsIt->GetTimestamp()) {
                        auto newRecord = result.Add();
                        newRecord->CopyFrom(item);
                    }
                }

                for (const auto& item: base) {
                    auto newRecord = result.Add();
                    newRecord->CopyFrom(item);
                }
                break;
            }
        }


        SortBy(result, getTimestamp);

        return result;
    }

}

namespace NPersonalization {

[[nodiscard]] ::google::protobuf::RepeatedPtrField<NProto::TFadingEmbedding> Patch(
    const ::google::protobuf::RepeatedPtrField<NProto::TFadingEmbedding>& base,
    const ::google::protobuf::RepeatedPtrField<NProto::TFadingEmbedding>& patch,
    const NProto::TUserHistoryPatch::EUserHistoryPatchLogic patchLogic
) {
    auto combineFadingEmbeddings = [](const auto& normal, const auto& preferred) {
        ::google::protobuf::RepeatedPtrField<NProto::TFadingEmbedding> result;

        // copy embeddings which are not present in preferred container
        for (const auto& emb: normal) {
            auto it = FindIf(
                preferred, [&emb](const auto& rhsEmb) {
                    return FadingEmbeddingsOfTheSameType(emb, rhsEmb);
                }
            );
            if (it == preferred.end()) {
                //copy from base
                auto addedEmbeddings = result.Add();
                addedEmbeddings->CopyFrom(emb);
            }
        }

        // copy embeddings from preferred container
        for (const auto& emb: preferred) {
            auto addedEmbeddings = result.Add();
            addedEmbeddings->CopyFrom(emb);
        }
        return result;
    };

    switch (patchLogic) {
        case NProto::TUserHistoryPatch::PreferDataFromPatch:
            return combineFadingEmbeddings(base, patch);

        case NProto::TUserHistoryPatch::PreferDataFromBase:
            return combineFadingEmbeddings(patch, base);
    }
}


NProto::TUserHistory Patch(const NPersonalization::NProto::TUserHistory& base, const NPersonalization::NProto::TUserHistory& patch, NPersonalization::NProto::TUserHistoryPatch::EUserHistoryPatchLogic patchLogic) {
    NPersonalization::NProto::TUserHistory result;

    (*result.MutableFadingEmbeddings()) = Patch(base.GetFadingEmbeddings(), patch.GetFadingEmbeddings(), patchLogic);
    (*result.MutableFilteredRecords()) = ::Patch(base.GetFilteredRecords(), patch.GetFilteredRecords(), patchLogic);
    (*result.MutableRecords()) = ::Patch(base.GetRecords(), patch.GetRecords(), patchLogic);

    return result;
}

NProto::TUserHistory Patch(const NPersonalization::NProto::TUserHistory& base, const NPersonalization::NProto::TUserHistoryPatch& patch) {
    return Patch(base, patch.GetPatch(), patch.GetPatchLogic());
}

}
