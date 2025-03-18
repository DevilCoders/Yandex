#pragma once

#include "block_set.h"
#include "cbb.h"
#include "cbb_id.h"
#include "exp_bin.h"
#include "stat.h"

#include <antirobot/lib/addr.h>

#include <library/cpp/threading/rcu/rcu.h>

#include <util/generic/ptr.h>

#include <atomic>
#include <functional>
#include <utility>

namespace NAntiRobot {
    struct TRequestContext;

    class TBlocker : private NThreading::TRcu<TBlockSet> {
    public:
        using TNonBlockingChecker = std::function<bool (const TUid& uid)>;

        template <typename ...Args>
        TBlocker(TNonBlockingChecker nonBlockingChecker, Args&&... args)
            : NThreading::TRcu<TBlockSet>(std::forward<Args>(args)...)
            , NonBlockingChecker(std::move(nonBlockingChecker))
        {
        }

        using NThreading::TRcu<TBlockSet>::GetCopy;

        TFuture<bool> AddBlock(const TBlockRecord& record);
        TFuture<bool> AddForcedBlock(const TBlockRecord& record);

        TFuture<TVector<TBlockRecord>> RemoveExpired();

        TVector<TBlockRecord> GetAllUidBlocks(const TUid& uid) const;

    private:
        TNonBlockingChecker NonBlockingChecker;
    };

    class TIsBlocked {
    private:
        class TImpl;
        THolder<TImpl> Impl;
    public:
        enum class EBlockSource {
            Unknown,
            Auto,
            Manual,
            Managed,
            Panic,
            Suspicious,
        };

        struct TBlockState {
            bool IsBlocked = false;
            EBlockSource Source = EBlockSource::Unknown;
            TInstant ExpireTime;
            TString Reason;
        };

    public:
        TIsBlocked(
            const TBlocker& blocker,
            TRefreshableAddrSet manualBlockList,
            TBlocker::TNonBlockingChecker nonBlockingChecker
        );

        ~TIsBlocked();

        TBlockState GetBlockState(const TRequestContext& rc) const;
    };

    class TBlockResponsesStats {
    public:
        enum class EServiceGroupCounter {
            UserMarkedWithLcookie           /* "block_responses_user_marked_with_Lcookie" */,
            WithLcookie                     /* "block_responses_with_Lcookie" */,
            Count
        };

        enum class EServiceNsGroupCounter {
            TotalTrustedUsers               /* "block_responses_total_trusted_users" */,

            MarkedTotal                     /* "block_responses_marked_total" */,
            MarkedWithCaptcha               /* "block_responses_marked_with_captcha" */,
            MarkedWithLcookie               /* "block_responses_marked_with_Lcookie" */,
            MarkedWithTrustedUsers          /* "block_responses_marked_with_trusted_users" */,
            MarkedWithDegradation           /* "block_responses_marked_with_degradation" */,
            MarkedAlreadyBanned             /* "block_responses_marked_already_banned" */,
            MarkedAlreadyBlocked            /* "block_responses_marked_already_blocked" */,

            UserMarkedTotal                 /* "block_responses_user_marked_total" */,
            UserMarkedTotalTrustedUsers     /* "block_responses_user_marked_total_trusted_users" */,
            UserMarkedWithCaptcha           /* "block_responses_user_marked_with_captcha" */,

            YandexBlockedRequests           /* "block_responses_from_yandex" */,
            WhitelistBlockedRequests        /* "block_responses_from_whitelist" */,

            Total                           /* "block_responses_total_req" */,

            Count
        };

        enum class EExpBinCounter {
            Total                           /* "block_responses_total" */,
            Count
        };

    private:
        TCategorizedStats<
            std::atomic<size_t>, EServiceGroupCounter,
            EHostType, EReqGroup
        > ServiceGroupCounters;

        TCategorizedStats<
            std::atomic<size_t>, EServiceNsGroupCounter,
            EHostType, ESimpleUidType, EReqGroup
        > ServiceNsGroupCounters;

        TCategorizedStats<
            std::atomic<size_t>, EExpBinCounter,
            EHostType, ESimpleUidType, EExpBin
        > ServiceNsExpBinCounters;

    public:
        explicit TBlockResponsesStats(std::array<size_t, HOST_NUMTYPES> groupCounts)
            : ServiceGroupCounters(groupCounts)
            , ServiceNsGroupCounters(groupCounts)
            , ServiceNsExpBinCounters()
        {}

        void Update(const TRequestContext& rc);
        void UpdateMarked(const TRequestContext& rc);
        void UpdateMarkedBanned(const TRequestContext& rc);
        void UpdateUserMarked(const TRequest& req);
        void PrintStats(const TRequestGroupClassifier& groupClassifier, TStatsWriter& out) const;
    };

    TString CbbReason(TCbbGroupId cbbGroup);

    template <typename T>
    TString CbbReason(const TVector<T>& groups) {
        TStringBuilder reason;
        reason << "CBB";

        for (const auto group : groups) {
            reason << '_' << group;
        }

        return std::move(reason);
    }
}
