#include "compression_tools.h"
#include "descriptions_to_log.h"
#include "embedding_tools.h"

#include <library/cpp/blockcodecs/codecs.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <util/datetime/base.h>


namespace {
    const NBlockCodecs::ICodec* GetCodec(const NPersonalization::NProto::ECodecType type) {
        using namespace NPersonalization::NProto;

        switch (type) {
            case ECodecType::CT_NO_COMPRESSION:
                return NBlockCodecs::Codec("null");
                break;
            case ECodecType::CT_BROTLI_5:
                return NBlockCodecs::Codec("brotli_5");
                break;
        }
        Y_ENSURE(false, "Unable to determine codec");
        return nullptr;
    }
}

namespace NPersonalization {
    NProto::TUserHistory Decompress(const NProto::TCompressedUserHistory& compressedHistory) {
        const NBlockCodecs::ICodec* codecPtr = GetCodec(compressedHistory.GetCodecType());
        NProto::TUserHistory result;
        Y_ENSURE(codecPtr, "can't get codec for decompression");
        const TString decompressedHistory = codecPtr->Decode(compressedHistory.GetCompressedUserHistory());
        Y_ENSURE(result.ParseFromString(decompressedHistory));
        return result;
    }

    NProto::TCompressedUserHistory Compress(const NProto::TUserHistory& history, const NProto::ECodecType codecType) {
        NProto::TCompressedUserHistory result;
        const NBlockCodecs::ICodec* codecPtr = GetCodec(codecType);
        Y_ENSURE(codecPtr, "can't get codec for compression");
        result.SetCodecType(codecType);
        result.SetCompressedUserHistory(codecPtr->Encode(history.SerializeAsString()));
        return result;
    }

    TString GetCompressedUserHistory(const NProto::TUserHistory& history, const TVector<NProto::TUserRecordsDescription>& descriptionsToLog, bool isFiltered, const TMaybe<TInstant>& now) {
        NProto::TUserHistory protoMessageToLog;
        for (const auto& filteredRecords : history.GetFilteredRecords()) {
            for (const auto& descToLog : descriptionsToLog) {
                if (CompareUserRecordsDescriptions(filteredRecords.GetDescription(), descToLog)) {
                    if (now.Defined() && descToLog.HasRecordLifetime() && descToLog.GetRecordLifetime() > 0) {
                        auto recLifetime = descToLog.GetRecordLifetime();
                        auto& filteredRecordsToLog = *protoMessageToLog.AddFilteredRecords();
                        auto& desc = *filteredRecordsToLog.MutableDescription();
                        desc.CopyFrom(filteredRecords.GetDescription());
                        desc.SetRecordLifetime(recLifetime);
                        for (const auto& rec : filteredRecords.GetRecords()) {
                            if (rec.GetTimestamp() + recLifetime >= now->Seconds()) {
                                filteredRecordsToLog.AddRecords()->CopyFrom(rec);
                            }
                        }
                    } else {
                        protoMessageToLog.AddFilteredRecords()->CopyFrom(filteredRecords);
                    }
                }
            }
        }
        if (isFiltered) {
            protoMessageToLog.SetIsFiltered(isFiltered);
        }
        auto compressedHistory = Compress(protoMessageToLog, NProto::ECodecType::CT_BROTLI_5);
        return compressedHistory.SerializeAsString();
    }

    TVector<NProto::TUserRecordsDescription> GetDescsWithFreshUserHistory(TVector<NProto::TUserRecordsDescription> filteredDescriptionsToLog) {
        filteredDescriptionsToLog.push_back(ConstructUserBodyFreshRequestsDescription());
        filteredDescriptionsToLog.push_back(ConstructUserBodyFreshClicksDescription());
        return filteredDescriptionsToLog;
    }

    TString GetUserHistoryForLogging(const NProto::TUserHistory& history, bool isFiltered, const TMaybe<TInstant>& now, bool shouldLogFilteredFreshUserHistory) {
        static const TVector<NProto::TUserRecordsDescription> descriptionsToLog = {
            ConstructUserBodyRequestsDescription(),
            ConstructUserBodyClicksDescription(),
        };
        static const TVector<NProto::TUserRecordsDescription> filteredDescriptionsToLog = {}; // Rt-dssm descriptions where here but then got removed
        static const TVector<NProto::TUserRecordsDescription> filteredDescriptionsToLogWithFreshUserHistory = GetDescsWithFreshUserHistory(filteredDescriptionsToLog);
        return GetCompressedUserHistory(history, isFiltered ? (shouldLogFilteredFreshUserHistory ? filteredDescriptionsToLogWithFreshUserHistory : filteredDescriptionsToLog) : descriptionsToLog, isFiltered, now);
    }

    TString GetUserHistoryForBert(const NProto::TUserHistory& history, const TInstant& now) {
        static const TVector<NProto::TUserRecordsDescription> descriptionsToLog = {
            ConstructUserBodyRequestsDescription(),
            ConstructUserBodyClicksDescription(),
        };

        return GetCompressedUserHistory(history, descriptionsToLog, false, now);
    }

    TMaybe<NPersonalization::NProto::TUserHistory> GetUserHistoryProtoFromSearchPersBigRTProfile(const TStringBuf compressedProfile) {
        TString decompressedMessage;
        try {
            auto compressedMessage = Base64StrictDecode(compressedProfile);
            decompressedMessage = NBlockCodecs::Codec("brotli_5")->Decode(compressedMessage);
        } catch (const yexception& ex) {
            return Nothing();
        }
        yabs::proto::Profile profile;
        if (!profile.ParseFromString(decompressedMessage)) {
            return Nothing();
        }
        if (profile.search_pers_profiles().empty() || !profile.search_pers_profiles(0).has_profile()) {
            return Nothing();
        }
        return std::move(*profile.mutable_search_pers_profiles(0)->mutable_profile()->MutableUserHistoryState()->MutableUserHistory());
    }

    TString GetUserHistoryResponseFromSearchPersBigRTProfile(const TStringBuf compressedProfile) {
        if (auto uhProto = GetUserHistoryProtoFromSearchPersBigRTProfile(compressedProfile)) {
            return Base64Encode(GetUserHistoryForLogging(
                uhProto.GetRef(), false));
        } else {
            return TString();
        }
    }

    yabs::proto::Profile EmulateUserHistoryDelay(const yabs::proto::Profile& profile, const TInstant& now, ui64 userHistoryDelay) {
        yabs::proto::Profile result = profile;
        if (userHistoryDelay && !profile.search_pers_profiles().empty() && profile.search_pers_profiles(0).has_profile()) {
            ui64 maxAllowedTimestamp = now.Seconds() - userHistoryDelay;
            auto* filteredRecordsWithDescriptions = result.mutable_search_pers_profiles(0)->mutable_profile()->MutableUserHistoryState()->MutableUserHistory()->MutableFilteredRecords();
            filteredRecordsWithDescriptions->Clear();
            for (const auto& recordsWithDescription : profile.search_pers_profiles(0).profile().GetUserHistoryState().GetUserHistory().GetFilteredRecords()) {
                auto* filteredRecordsWithDescription = filteredRecordsWithDescriptions->Add();
                *filteredRecordsWithDescription->MutableDescription() = recordsWithDescription.GetDescription();
                for (const auto& record : recordsWithDescription.GetRecords()) {
                    if (Max<ui64>(record.GetTimestamp(), record.GetRequestTimestamp()) <= maxAllowedTimestamp) {
                        *filteredRecordsWithDescription->AddRecords() = record;
                    }
                }
            }
        }
        return result;
    }
}
