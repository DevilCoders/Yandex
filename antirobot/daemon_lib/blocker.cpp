#include "blocker.h"

#include "antiddos.h"
#include "block_set.h"
#include "cbb.h"
#include "config.h"
#include "environment.h"
#include "eventlog_err.h"
#include "night_check.h"
#include "threat_level.h"
#include "unified_agent_log.h"

#include <antirobot/idl/antirobot.ev.pb.h>
#include <antirobot/lib/alarmer.h>
#include <antirobot/lib/enum.h>
#include <antirobot/lib/ip_list.h>

#include <library/cpp/threading/rcu/rcu_accessor.h>

#include <util/generic/deque.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/stream/tempbuf.h>
#include <util/string/builder.h>
#include <util/string/escape.h>
#include <util/string/join.h>
#include <util/system/event.h>
#include <util/system/mutex.h>
#include <util/system/rwlock.h>
#include <util/system/thread.h>

#include <utility>

using namespace NAntirobotEvClass;

namespace NAntiRobot {
    namespace {
        const TString MSG_BLOCK_TIME_EXPIRED = "Block time expired";
        const TIsBlocked::TBlockState NOT_BLOCKED {false, TIsBlocked::EBlockSource::Unknown, TInstant::Zero(), TString()};

        void LogBlockRecord(TUnifiedAgentLogBackend& eventLog, NAntirobotEvClass::TBlockType blockType,
                            const TBlockRecord& record, const TString& reason)
        {
            auto eventHeader = MakeEvlogHeader(TStringBuf(), record.Addr, record.Uid, record.YandexUid, TAddr(), TString());
            auto event = TBlockEvent(eventHeader, blockType, ui32(record.Category), reason);
            NAntirobotEvClass::TProtoseqRecord rec = CreateEventLogRecord(event);
            eventLog.WriteLogRecord(rec);
        }

        void LogBlock(TUnifiedAgentLogBackend& eventLog, const TBlockRecord& record) {
            LogBlockRecord(eventLog, NAntirobotEvClass::Block, record, record.Description);
        }

        void LogUnblock(TUnifiedAgentLogBackend& eventLog, const TBlockRecord& record, const TString& reason) {
            LogBlockRecord(eventLog, NAntirobotEvClass::Unblock, record, reason);
        }

        template <typename C>
        void LogUnblocks(TUnifiedAgentLogBackend& eventLog, const C& container, const TString& reason) {
            for (const auto& item : container) {
                LogUnblock(eventLog, item, reason);
            }
        }

        template <class Predicate>
        TFuture<TVector<TBlockRecord>> RemoveBlocksIf(NThreading::TRcu<TBlockSet>& blocks, Predicate pred) {
            return blocks.UpdateWithFunction([=](TBlockSet& blockSet) {
                return RemoveIf(blockSet, pred);
            });
        }
    }

    class TIsBlocked::TImpl {
    private:
        const TBlocker& Blocker;
        TRefreshableAddrSet ManualBlockList;
        TBlocker::TNonBlockingChecker NonBlockingChecker;

    private:
        TBlockState GetAutoBlockState(const TRequest& req) const {
            EBlockCategory category = req.BlockCategory;
            if (category == BC_NON_SEARCH) {
                auto recordPtr = FindBlockRecord(*Blocker.GetCopy(), req.Uid, category);
                if (recordPtr && recordPtr->Status == EBlockStatus::Block) {
                    return {true, EBlockSource::Auto, recordPtr->ExpireTime, CbbReason(BlockCategoryToCbbGroup(category))};
                }
            } else {
                TUid uid = TUid::FromAddrOrSubnet(req.UserAddr);

                auto recordPtr = FindBlockRecord(*Blocker.GetCopy(), uid, category);
                if (recordPtr && recordPtr->Status == EBlockStatus::Block) {
                    return {true, EBlockSource::Auto, recordPtr->ExpireTime, CbbReason(BlockCategoryToCbbGroup(category))};
                }
            }
            return NOT_BLOCKED;
        }

        TBlockState GetManualBlockState(const TRequest& req) const {
            if (ANTIROBOT_DAEMON_CONFIG.AllowBannedIpsAtNight && IsNightInMoscow(req.ArrivalTime))
                return NOT_BLOCKED;

            auto list = ManualBlockList->Get();

            if (list->ContainsActual(req.UserAddr) ||
                (req.SpravkaAddr != req.UserAddr && list->ContainsActual(req.SpravkaAddr))
               )
            {
                return {true, EBlockSource::Manual, TInstant::Max(), CbbReason(list->CbbGroups)};
            }

            return NOT_BLOCKED;
        }

        TBlockState GetManagedBlockState(const TRequestContext& rc) const {
            if (!rc.MatchedRules->ManagedBlock.empty()) {
                return {
                    true, EBlockSource::Managed, TInstant::Max(),
                    CbbReason(rc.MatchedRules->ManagedBlock)
                };
            }

            return NOT_BLOCKED;
        }

    public:
        TImpl(const TBlocker& blocker,
              TRefreshableAddrSet manualBlockList,
              TBlocker::TNonBlockingChecker nonBlockingChecker)
            : Blocker(blocker)
            , ManualBlockList(std::move(manualBlockList))
            , NonBlockingChecker(std::move(nonBlockingChecker))
        {
        }

        ~TImpl() = default;

        TBlockState GetBlockState(const TRequestContext& rc) const {
            const auto& req = *rc.Req;

            if (req.WillBlock()) {
                return {true, EBlockSource::Panic, TInstant::Max(), "PANIC"};
            }

            TBlockState state = GetManagedBlockState(rc);
            if (state.IsBlocked) {
                return state;
            }

            if (req.ReqType == REQ_XMLSEARCH) {
                return NOT_BLOCKED;
            }

            if (NonBlockingChecker(req.Uid)) {
                return NOT_BLOCKED;
            }

            state = GetAutoBlockState(req);
            if (state.IsBlocked) {
                return state;
            }

            state = GetManualBlockState(req);
            if (state.IsBlocked) {
                return state;
            }

            return NOT_BLOCKED;
        }

    };

    TIsBlocked::TIsBlocked(
        const TBlocker& blocker,
        TRefreshableAddrSet manualBlockList,
        TBlocker::TNonBlockingChecker nonBlockingChecker
    )
        : Impl(new TImpl(
            blocker,
            std::move(manualBlockList),
            std::move(nonBlockingChecker)
        ))
    {}

    TIsBlocked::~TIsBlocked() = default;

    TIsBlocked::TBlockState TIsBlocked::GetBlockState(const TRequestContext& rc) const {
        return Impl->GetBlockState(rc);
    }

    /* TBlockResponsesStats */

    void TBlockResponsesStats::Update(const TRequestContext& rc) {
        const TRequest& req = *rc.Req;

        ++ServiceNsExpBinCounters.Get(req, EExpBinCounter::Total);
        ++ServiceNsGroupCounters.Get(req, EServiceNsGroupCounter::Total);
        if (req.TrustedUser) {
            ++ServiceNsGroupCounters.Get(req, EServiceNsGroupCounter::TotalTrustedUsers);
        }

        if (req.HasValidLCookie) {
            ++ServiceGroupCounters.Get(req, EServiceGroupCounter::WithLcookie);

            rc.Env.UniqueLcookies->Add(
                req.HostType,
                TUid(TUid::LCOOKIE, req.LCookieUid),
                req.ArrivalTime, 
                /* matrixnet */ true, 
                /* yqlRules */ {}, 
                /* cbbRules */ {}
            );
        }

        if (req.UserAddr.IsYandex()){
            ++ServiceNsGroupCounters.Get(req, EServiceNsGroupCounter::YandexBlockedRequests);
        }

        if (req.UserAddr.IsWhitelisted()) {
            ++ServiceNsGroupCounters.Get(req, EServiceNsGroupCounter::WhitelistBlockedRequests);
        }
    }

    void TBlockResponsesStats::UpdateMarked(const TRequestContext& rc) {
        const TRequest& req = *rc.Req;
        TIsBlocked::TBlockState blockState = rc.Env.IsBlocked.GetByService(rc.Req->HostType)->GetBlockState(rc);

        ++ServiceNsGroupCounters.Get(req, EServiceNsGroupCounter::MarkedTotal);

        if (req.TrustedUser) {
            ++ServiceNsGroupCounters.Get(req, EServiceNsGroupCounter::MarkedWithTrustedUsers);
        }

        if (req.HasValidLCookie) {
            ++ServiceNsGroupCounters.Get(req, EServiceNsGroupCounter::MarkedWithLcookie);
        }

        if (req.CanShowCaptcha()) {
            ++ServiceNsGroupCounters.Get(req, EServiceNsGroupCounter::MarkedWithCaptcha);
        }

        if (rc.Degradation) {
            ++ServiceNsGroupCounters.Get(req, EServiceNsGroupCounter::MarkedWithDegradation);
        }

        if (blockState.IsBlocked) {
            ++ServiceNsGroupCounters.Get(req, EServiceNsGroupCounter::MarkedAlreadyBlocked);
        }
    }

    void TBlockResponsesStats::UpdateMarkedBanned(const TRequestContext& rc) {
        if (rc.RedirectType != ECaptchaRedirectType::NoRedirect) {
            ++ServiceNsGroupCounters.Get(*rc.Req, EServiceNsGroupCounter::MarkedAlreadyBanned);
        }
    }

    void TBlockResponsesStats::UpdateUserMarked(const TRequest& req) {
        ++ServiceNsGroupCounters.Get(req, EServiceNsGroupCounter::UserMarkedTotal);

        if (req.TrustedUser) {
            ++ServiceNsGroupCounters.Get(req, EServiceNsGroupCounter::UserMarkedTotalTrustedUsers);
        }

        if (req.HasValidLCookie) {
            ++ServiceGroupCounters.Get(req, EServiceGroupCounter::UserMarkedWithLcookie);
        }

        if (req.CanShowCaptcha()) {
            ++ServiceNsGroupCounters.Get(req, EServiceNsGroupCounter::UserMarkedWithCaptcha);
        }
    }

    void TBlockResponsesStats::PrintStats(
        const TRequestGroupClassifier& groupClassifier,
        TStatsWriter& out
    ) const {
        ServiceGroupCounters.Print(groupClassifier, out);
        ServiceNsGroupCounters.Print(groupClassifier, out);
        ServiceNsExpBinCounters.Print(out);
    }

    /* TBlocker */

    TFuture<bool> TBlocker::AddBlock(const TBlockRecord& record) {
        if (record.ExpireTime < TInstant::Now()
            || NonBlockingChecker(record.Uid)
            || IsIn(*GetCopy(), record))
        {
            return NThreading::MakeFuture(false);
        }
        return AddForcedBlock(record);
    }

    TFuture<bool> TBlocker::AddForcedBlock(const TBlockRecord& record) {
        auto result = UpdateWithFunction([record](TBlockSet& blockSet) -> bool {
            return blockSet.insert(record).second;
        });

        result.Subscribe([record](const TFuture<bool>& wasAdded) {
            if (wasAdded.GetValue()) {
                LogBlock(*ANTIROBOT_DAEMON_CONFIG.EventLog, record);
            }
        });

        return result;
    }

    TFuture<TVector<TBlockRecord>> TBlocker::RemoveExpired() {
        auto now = TInstant::Now();
        auto isExpired = [now](const TBlockRecord& record) {
            return record.ExpireTime < now;
        };
        auto result = RemoveBlocksIf(*this, isExpired);

        result.Subscribe([](const TFuture<TVector<TBlockRecord>>& removedItems) {
            LogUnblocks(*ANTIROBOT_DAEMON_CONFIG.EventLog, removedItems.GetValue(), MSG_BLOCK_TIME_EXPIRED);
        });

        return result;
    }

    TVector<TBlockRecord> TBlocker::GetAllUidBlocks(const TUid& uid) const {
        return GetAllItemsWithUid(*GetCopy(), uid);
    }

    TString CbbReason(TCbbGroupId cbbGroup) {
        return TStringBuilder() << "CBB_" << cbbGroup;
    }
}
