#pragma once

#include <kernel/user_history/proto/user_history.pb.h>

#include <yabs/proto/user_profile.pb.h>

#include <util/datetime/base.h>
#include <util/generic/maybe.h>

namespace NPersonalization {
    NProto::TUserHistory Decompress(const NProto::TCompressedUserHistory& compressedHistory);
    NProto::TCompressedUserHistory Compress(const NProto::TUserHistory& history, const NProto::ECodecType codecType);

    TString GetCompressedUserHistory(const NProto::TUserHistory& history, const TVector<NProto::TUserRecordsDescription>& descriptionsToLog, bool isFiltered = false, const TMaybe<TInstant>& now = Nothing());
    TString GetUserHistoryForBert(const NProto::TUserHistory& history, const TInstant& now);
    TString GetUserHistoryForLogging(const NProto::TUserHistory& history, bool isFiltered = false, const TMaybe<TInstant>& now = Nothing(), bool shouldLogFilteredFreshUserHistory = false);

    TMaybe<NPersonalization::NProto::TUserHistory> GetUserHistoryProtoFromSearchPersBigRTProfile(const TStringBuf compressedProfile);

    // This function allows to get data that must be identical to UserHistoryResponse from SearchPersBigRTProfile,
    // but it is heavy in calculation, so it is not recommended to use it if latency or cpu usage matters.
    TString GetUserHistoryResponseFromSearchPersBigRTProfile(const TStringBuf compressedProfile);

    yabs::proto::Profile EmulateUserHistoryDelay(const yabs::proto::Profile& profile, const TInstant& now, ui64 userHistoryDelay);
}
