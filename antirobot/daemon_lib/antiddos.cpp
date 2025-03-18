#include "antiddos.h"

#include "unified_agent_log.h"

#include <antirobot/idl/antirobot.ev.pb.h>
#include <antirobot/lib/alarmer.h>
#include <antirobot/lib/enum.h>

#include <util/generic/ylimits.h>
#include <util/stream/str.h>
#include <util/string/printf.h>
#include <util/system/yassert.h>

using namespace NAntirobotEvClass;

namespace NAntiRobot {
    namespace {

        struct TBlockCategoryParams {
            const EBlockCategory Category;
            const TRequest* Request;
            const TUid UidToBlock;
            const float RpsThreshold;
            const TCbbGroupId CbbFlag;
            const TDuration BlockPeriod;
            const EBlockStatus BlockStatus;
            const bool HandleRps;

            TBlockCategoryParams()
                : Category(BC_UNDEFINED)
                , Request(nullptr)
                , RpsThreshold(Max<float>())
                , CbbFlag(BlockCategoryToCbbGroup(BC_UNDEFINED))
                , BlockPeriod(TDuration())
                , BlockStatus(EBlockStatus::None)
                , HandleRps(false)
            {
            }

            TBlockCategoryParams(
                EBlockCategory category,
                const TRequest* request,
                const TUid& uidToBlock,
                float rpsThreshold,
                TCbbGroupId cbbFlag,
                TDuration blockPeriod,
                EBlockStatus blockStatus,
                bool handleRps
            )
                : Category(category)
                , Request(request)
                , UidToBlock(uidToBlock)
                , RpsThreshold(rpsThreshold)
                , CbbFlag(cbbFlag)
                , BlockPeriod(blockPeriod)
                , BlockStatus(blockStatus)
                , HandleRps(handleRps)
            {
            }

            TString MakeBlockDescription(float rps) {
                TString str;
                TStringOutput out(str);
                out << Sprintf("rps %.1f", rps) << ", uid " << UidToBlock << ", yandexuid " << (Request ? Request->YandexUid : TStringBuf());

                return str;
            }

            static TBlockCategoryParams Get(EBlockCategory category, const TRequest& req) {
                switch(category) {
                    case BC_UNDEFINED:
                    case BC_NUM:
                    case BC_SEARCH:
                        return TBlockCategoryParams();

                    case BC_NON_SEARCH:
                        return TBlockCategoryParams(BC_NON_SEARCH,
                                                    &req,
                                                    req.Uid,
                                                    ANTIROBOT_DAEMON_CONFIG.DDosRpsThreshold,
                                                    BlockCategoryToCbbGroup(BC_NON_SEARCH),
                                                    ANTIROBOT_DAEMON_CONFIG.DDosFlag1BlockPeriod,
                                                    ANTIROBOT_DAEMON_CONFIG.ConfByTld(req.Tld).DDosFlag1BlockEnabled ? EBlockStatus::Block : EBlockStatus::Mark,
                                                    true
                                                    );


                    case BC_SEARCH_WITH_SPRAVKA:
                        return TBlockCategoryParams(BC_SEARCH_WITH_SPRAVKA,
                                                    &req,
                                                    TUid::FromAddr(req.UserAddr),
                                                    ANTIROBOT_DAEMON_CONFIG.DDosRpsThreshold,
                                                    BlockCategoryToCbbGroup(BC_SEARCH_WITH_SPRAVKA),
                                                    ANTIROBOT_DAEMON_CONFIG.DDosFlag2BlockPeriod,
                                                    ANTIROBOT_DAEMON_CONFIG.ConfByTld(req.Tld).DDosFlag2BlockEnabled ? EBlockStatus::Block : EBlockStatus::Mark,
                                                    true
                                                    );

                    case BC_ANY_FROM_WHITELIST:
                        return TBlockCategoryParams(BC_ANY_FROM_WHITELIST,
                                                    &req,
                                                    TUid::FromAddr(req.UserAddr),
                                                    ANTIROBOT_DAEMON_CONFIG.DDosRpsThreshold,
                                                    BlockCategoryToCbbGroup(BC_ANY_FROM_WHITELIST),
                                                    ANTIROBOT_DAEMON_CONFIG.DDosAmnestyPeriod,
                                                    EBlockStatus::Mark,
                                                    true
                                                    );
                }
                return TBlockCategoryParams();
            }
        };

        void AddBlockToCbb(ICbbIO& cbbIO, TCbbGroupId cbbFlag, const TBlockRecord& block) {
            TAddr rangeStart, rangeEnd;

            if (block.Addr.GetFamily() == AF_INET6) {
                block.Addr.GetIntervalForMask(TUid::Ip6Bits, rangeStart, rangeEnd);
            } else {
                rangeStart = rangeEnd = block.Addr;
            }
            cbbIO.AddRangeBlock(cbbFlag, rangeStart, rangeEnd, block.ExpireTime, block.Description);
        }
    } // anonymous namespace

    class TAntiDDoS::TImpl {
    private:
        TBlocker& Blocker;
        ICbbIO* CbbIO;

        TAtomic NumDDosers;
        TAtomic NumRequestsFromDDosersTotal;
        TAtomic NumRequestsFromDDosers[BC_NUM];

    private:
        bool HandleRps(const TRequest& req, EBlockCategory category, const TRpsStat& rpsStat, TUserBase& userBase) {
            TBlockCategoryParams params = TBlockCategoryParams::Get(category, req);
            if (!params.HandleRps)
                return false;

            if (rpsStat.Rps < params.RpsThreshold)
                return false;

            AtomicIncrement(NumRequestsFromDDosers[category]);
            AtomicIncrement(NumRequestsFromDDosersTotal);

            TInstant expireTime = req.ArrivalTime + params.BlockPeriod;
            TString description = params.MakeBlockDescription(rpsStat.Rps);

            TBlockRecord blockRecord = {
                params.UidToBlock,
                params.Category,
                req.UserAddr,
                TString{req.YandexUid},
                params.BlockStatus,
                expireTime,
                description,
            };

            auto cbbFlag = params.CbbFlag;
            Blocker.AddBlock(blockRecord).Subscribe([=, &userBase](const TFuture<bool>& wasAdded) {
                if (wasAdded.GetValue()) {
                    if (CbbIO) {
                        AddBlockToCbb(*CbbIO, cbbFlag, blockRecord);
                    }
                    userBase.Get(blockRecord.Uid)->ClearRps(blockRecord.Category);

                    AtomicIncrement(NumDDosers);
                }
            });

            return true;
        }

        bool ProcessUser(const TRequest& req, TUserBase& userBase) {
            if (req.UserAddr.IsWhitelisted())
                return false;

            if (req.IsSearch)
                return false;

            TRpsStat rpsStat;
            {
                TUserBase::TValue tlUser = userBase.Get(req.Uid);
                tlUser->TouchDDosRps(BC_NON_SEARCH, req.ArrivalTime);
                tlUser->CurRpsStat(BC_NON_SEARCH, rpsStat);
            }

            HandleRps(req, BC_NON_SEARCH, rpsStat, userBase);

            return true;
        }

        void ProcessIp(const TRequest& req, TUserBase& userBase, bool isRobot) {
            if (!(req.IsSearch && req.Uid.Ns == TUid::SPRAVKA || req.UserAddr.IsWhitelisted()))
                return;

            if (isRobot)
                return;

            TRpsStat rpsStatAnyWL;
            TRpsStat rpsStatSearch;
            {
                TUserBase::TValue tlIp = userBase.Get(TUid::FromAddr(req.UserAddr));

                if (req.UserAddr.IsWhitelisted())
                    tlIp->TouchDDosRps(BC_ANY_FROM_WHITELIST, req.ArrivalTime);
                else
                    tlIp->TouchDDosRps(BC_SEARCH_WITH_SPRAVKA, req.ArrivalTime);

                tlIp->CurRpsStat(BC_ANY_FROM_WHITELIST, rpsStatAnyWL);
                tlIp->CurRpsStat(BC_SEARCH_WITH_SPRAVKA, rpsStatSearch);
            }

            HandleRps(req, BC_SEARCH_WITH_SPRAVKA, rpsStatSearch, userBase) || HandleRps(req, BC_ANY_FROM_WHITELIST, rpsStatAnyWL, userBase);
        }
    public:
        TImpl(TBlocker& blocker, ICbbIO* cbbIO)
            : Blocker(blocker)
            , CbbIO(cbbIO)
            , NumDDosers(0)
            , NumRequestsFromDDosersTotal(0)
        {
            for (int i = 0; i < BC_NUM; i++)
                NumRequestsFromDDosers[i] = 0;
        }

        void ProcessRequest(const TRequest& req, TUserBase& userBase, bool isRobot) {
            if (req.UserAddr.IsPrivileged())
                return;

            if (ProcessUser(req, userBase))
               return;

            ProcessIp(req, userBase, isRobot);

        }

        void PrintStats(TStatsWriter& out) const {
            out.WriteScalar("new_ddosers", AtomicGet(NumDDosers))
               .WriteScalar("ddos_num_requests", AtomicGet(NumRequestsFromDDosersTotal))
               .WriteScalar("ddos_num_requests1", AtomicGet(NumRequestsFromDDosers[BC_NON_SEARCH]))
               .WriteScalar("ddos_num_requests2", AtomicGet(NumRequestsFromDDosers[BC_SEARCH_WITH_SPRAVKA]))
               .WriteScalar("ddos_num_requests3", AtomicGet(NumRequestsFromDDosers[BC_ANY_FROM_WHITELIST]));
        }
    };

    TAntiDDoS:: TAntiDDoS(TBlocker& blocker, ICbbIO* cbbIO)
        : Impl(new TImpl(blocker, cbbIO))
    {
    }

    TAntiDDoS::~TAntiDDoS() = default;

    void TAntiDDoS::ProcessRequest(const TRequest& req, TUserBase& userBase, bool isRobot) {
        Impl->ProcessRequest(req, userBase, isRobot);
    }

    void TAntiDDoS::PrintStats(TStatsWriter& out) const {
        Impl->PrintStats(out);
    }

    TCbbGroupId BlockCategoryToCbbGroup(EBlockCategory category) {
        switch (category) {
            case BC_UNDEFINED:
            case BC_NUM:
            case BC_SEARCH:
                return TCbbGroupId{0};

            case BC_NON_SEARCH:
                return ANTIROBOT_DAEMON_CONFIG.CbbFlagDDos1;

            case BC_SEARCH_WITH_SPRAVKA:
                return ANTIROBOT_DAEMON_CONFIG.CbbFlagDDos2;

            case BC_ANY_FROM_WHITELIST:
                return ANTIROBOT_DAEMON_CONFIG.CbbFlagDDos3;
        }

        return TCbbGroupId{0};
    }
} // namespace NAntiRobot
