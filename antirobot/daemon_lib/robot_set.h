#pragma once

#include "cbb_id.h"
#include "stat.h"

#include <antirobot/lib/absl_flat_hash_map.h>
#include <antirobot/lib/absl_flat_hash_set.h>
#include <antirobot/lib/tuple_ops.h>

#include <library/cpp/iterator/functools.h>

#include <google/protobuf/repeated_field.h>

#include <util/generic/algorithm.h>
#include <util/generic/hash.h>
#include <util/generic/set.h>
#include <util/generic/variant.h>
#include <util/generic/ylimits.h>
#include <util/system/rwlock.h>

#include <array>
#include <compare>
#include <utility>


namespace NAntiRobot {


namespace NCacheSyncProto {
    class TBanAction;
    class TCbbBanEntry;
}

class TRequest;


struct TBanReasons {
    ui8 Matrixnet : 1;
    ui8 Yql : 1;
    ui8 Cbb : 1;

    TBanReasons() : TBanReasons(false, false, false) {}

    TBanReasons(bool matrixnet, bool yql, bool cbb)
        : Matrixnet(matrixnet)
        , Yql(yql)
        , Cbb(cbb)
    {}

    TBanReasons operator||(const TBanReasons& that) const {
        return {Matrixnet || that.Matrixnet, Yql || that.Yql, Cbb || that.Cbb};
    }
};


class TRobotSet {
public:
    struct TAllBans;

    enum class EHistogram {
        Size  /* "robot_set_size" */,
        Count
    };

public:
    TRobotSet() = default;

    explicit TRobotSet(IInputStream* input, TInstant now = TInstant::Zero());

    explicit TRobotSet(const TString& path, TInstant now = TInstant::Zero()) {
        TFileInput input(path);
        Load(&input, now);
    }

    void Load(IInputStream* input, TInstant now = TInstant::Zero());

    void Load(const TString& path, TInstant now = TInstant::Zero()) {
        TFileInput input(path);
        Load(&input, now);
    }

    void UnsafeInherit(EHostType child, const TVector<EHostType>& parents) {
        ServiceToParents[child] = parents;
    }

    void AddManual(
        EHostType service,
        const TUid& uid,
        TInstant expirationTime,
        bool matrixnet,
        const TVector<TCbbRuleId>& yqlRules,
        const TVector<TCbbRuleKey>& cbbRules
    );

    void Add(
        EHostType service,
        const TUid& uid,
        TInstant arrivalTime,
        bool matrixnet,
        const TVector<TCbbRuleId>& yqlRules,
        const TVector<TCbbRuleKey>& cbbRules
    );

    void Add(
        const TRequest& req,
        bool matrixnet,
        const TVector<TCbbRuleId>& yqlRules,
        const TVector<TCbbRuleKey>& cbbRules
    ) {
        Add(req.HostType, req.Uid, req.ArrivalTime, matrixnet, yqlRules, cbbRules);
    }

    bool UnsafeEquals(const TRobotSet& that) const {
        return AllOf(Zip(Buckets, that.Buckets), [] (const auto& bucket) {
            return std::get<0>(bucket).UnsafeEquals(std::get<1>(bucket));
        });
    }

    bool Contains(EHostType service, const TUid& uid) const;

    std::pair<TVector<TCbbRuleId>, TVector<TCbbRuleKey>> GetRules(
        EHostType service,
        const TUid& uid
    ) const;

    TInstant GetExpirationTime(EHostType service, const TUid& uid) const;
    TBanReasons GetBanReasons(EHostType service, const TUid& uid) const;

    void RemoveExpired(TInstant time = TInstant::Now());
    void Clear(EHostType service = HOST_NUMTYPES);

    void PrintStats(TStatsWriter* out) const {
        Histograms.Print(*out);
    }

    size_t Size() const {
        return Size_;
    }

    void WriteSyncResponse(
        const TUid& uid,
        const std::array<TAllBans, HOST_NUMTYPES>& serviceToReceivedBans,
        NProtoBuf::RepeatedPtrField<NCacheSyncProto::TBanAction>* actions
    ) const;

    void ApplySyncRequest(const NProtoBuf::RepeatedPtrField<NCacheSyncProto::TBanAction>& actions);

    void SerializeTo(IOutputStream* output) const;

    void LockedSave(const TString& path) const;

public:
    class TBanSet {
    public:
        class TEntry : public TTupleComparable<TEntry>, public TTupleHashable<TEntry> {
        private:
            template <typename F, typename G>
            using TCommonReturnType = std::common_type_t<
                std::invoke_result_t<F, TCbbRuleId>,
                std::invoke_result_t<G, TCbbRuleKey>
            >;

        public:
            explicit TEntry(TCbbRuleId ruleId)
                : GroupId(TCbbGroupId{::Max<ui32>()})
                , RuleId(ruleId)
            {}

            explicit TEntry(TCbbRuleKey ruleKey)
                : GroupId(ruleKey.Group)
                , RuleId(ruleKey.Rule)
            {}

            template <typename F, typename G>
            TCommonReturnType<F, G> Visit(const F& f, const G& g) const {
                return std::visit([&f, &g] (auto concreteRule) -> TCommonReturnType<F, G> {
                    if constexpr (std::is_same_v<decltype(concreteRule), TCbbRuleId>) {
                        return f(concreteRule);
                    } else if constexpr (std::is_same_v<decltype(concreteRule), TCbbRuleKey>) {
                        return g(concreteRule);
                    } else {
                        static_assert(TDependentFalse<decltype(concreteRule)>);
                    }
                }, Get());
            }

            std::variant<TCbbRuleId, TCbbRuleKey> Get() const {
                if (GroupId == TCbbGroupId{::Max<ui32>()}) {
                    return RuleId;
                } else {
                    return TCbbRuleKey(GroupId, RuleId);
                }
            }

            void SerializeTo(NCacheSyncProto::TCbbRule* pbRule) const;

            auto AsTuple() const {
                return std::tie(GroupId, RuleId);
            }

        private:
            TCbbGroupId GroupId;
            TCbbRuleId RuleId;
        };

        struct TAddResult {
            TInstant DeletedTime;
            bool Added = false;
        };

    public:
        TBanSet() = default;

        template <typename TRules>
        explicit TBanSet(TInstant expirationTime, const TRules& rules) {
            TimeToRules[expirationTime] = {rules.begin(), rules.end()};

            for (const auto rule : rules) {
                RuleToTime[rule] = expirationTime;
            }
        }

        explicit TBanSet(
            const NProtoBuf::RepeatedPtrField<NCacheSyncProto::TCbbBanEntry>& pbYqlBans,
            const NProtoBuf::RepeatedPtrField<NCacheSyncProto::TCbbBanEntry>& pbCbbBans,
            TInstant now = TInstant::Zero(),
            TInstant* outMaxTime = nullptr
        );

        bool operator==(const TBanSet& that) const = default;

        TAddResult Add(TEntry rule, TInstant time);

        void Remove(TInstant time);

        bool Contains(TInstant time) const {
            return TimeToRules.contains(time);
        }

        bool Contains(TEntry rule, TInstant time) const {
            if (
                const auto thisRuleTime = RuleToTime.find(rule);
                thisRuleTime != RuleToTime.end()
            ) {
                return time <= thisRuleTime->second;
            }

            return false;
        }

        bool Empty() const {
            return RuleToTime.empty();
        }

        const TAbslFlatHashMap<TEntry, TInstant, TEntry::THash>& GetRuleToTime() const {
            return RuleToTime;
        }

        const TAbslFlatHashMap<TInstant, TAbslFlatHashSet<TEntry, TEntry::THash>>& GetTimeToRules() const {
            return TimeToRules;
        }

        std::pair<TVector<TCbbRuleId>, TVector<TCbbRuleKey>> GetRules() const;

        TBanReasons GetBanReasons() const;

        bool WriteSyncResponse(
            const TBanSet& received,
            NProtoBuf::RepeatedPtrField<NCacheSyncProto::TCbbBanEntry>* pbYqlBans,
            NProtoBuf::RepeatedPtrField<NCacheSyncProto::TCbbBanEntry>* pbCbbBans
        ) const;

        void SerializeTo(
            NProtoBuf::RepeatedPtrField<NCacheSyncProto::TCbbBanEntry>* pbYqlBans,
            NProtoBuf::RepeatedPtrField<NCacheSyncProto::TCbbBanEntry>* pbCbbBans
        ) const;

    private:
        template <typename F>
        bool SerializeIf(
            const F& condition,
            NProtoBuf::RepeatedPtrField<NCacheSyncProto::TCbbBanEntry>* pbYqlBans,
            NProtoBuf::RepeatedPtrField<NCacheSyncProto::TCbbBanEntry>* pbCbbBans
        ) const;

    private:
        TInstant Max;
        TAbslFlatHashMap<TEntry, TInstant, TEntry::THash> RuleToTime;
        TAbslFlatHashMap<TInstant, TAbslFlatHashSet<TEntry, TEntry::THash>> TimeToRules;
    };

    struct TAllBans {
        TInstant Matrixnet;
        TBanSet Bans;

        TAllBans() = default;

        explicit TAllBans(
            const NCacheSyncProto::TBanAction& action,
            TInstant now = TInstant::Zero()
        );
    };

private:
    struct TUidRefKey : public TTupleComparable<TUidRefKey>, public TTupleHashable<TUidRefKey> {
        mutable const TUid* Uid = nullptr;

        explicit TUidRefKey(const TUid* uid) : Uid(uid) {}

        std::tuple<const TUid&> AsTuple() const {
            return *Uid;
        }
    };

    struct TBanInfo {
        TInstant Max;
        TBanSet Bans;

        TBanInfo() = default;

        TBanInfo(TInstant max, TBanSet bans)
            : Max(max)
            , Bans(std::move(bans))
        {}

        bool operator==(const TBanInfo& that) const = default;

        void Remove(TInstant time);
    };

    struct TExpirationQueueItem : public TTupleComparable<TExpirationQueueItem> {
        TInstant Time;
        TUid Uid;

        TExpirationQueueItem(TInstant time, TUid uid) : Time(time), Uid(uid) {}

        std::tuple<const TInstant&, const TUid&> AsTuple() const {
            return std::tie(Time, Uid);
        }
    };

    struct TServiceData {
        TAbslFlatHashMap<TUidRefKey, TInstant, THash<TTupleHashable<TUidRefKey>>> UidToMatrixnet;
        THashMap<TUidRefKey, TBanInfo, THash<TTupleHashable<TUidRefKey>>> UidToBanInfo;
        TSet<TExpirationQueueItem> ExpirationQueue;

        bool operator==(const TServiceData& that) const = default;

        bool Contains(const TUid& uid) const;

        TInstant GetExpirationTime(const TUid& uid) const;

        TBanReasons GetBanReasons(const TUid& uid) const;
    };

    struct TBucket {
        std::array<TServiceData, HOST_NUMTYPES> ServiceToData;
        mutable TRWMutex Mutex;

        bool UnsafeEquals(const TBucket& that) const {
            return ServiceToData == that.ServiceToData;
        }
    };

private:
    template <typename TEntries>
    void UnsafeAdd(
        const TUid& uid,
        TInstant expirationTime,
        bool matrixnet,
        const TEntries& entries,
        bool hasEntries,
        TServiceData* data
    );

    void UnsafeClearData(TServiceData* data);

    std::pair<TReadGuard, const TBucket&> ReadBucket(const TUid& uid) const {
        const auto& bucket = Buckets[BucketIndex(uid)];
        return {TReadGuard(bucket.Mutex), bucket};
    }

    std::pair<TWriteGuard, TBucket&> WriteBucket(const TUid& uid) {
        auto& bucket = Buckets[BucketIndex(uid)];
        return {TWriteGuard(bucket.Mutex), bucket};
    }

    std::pair<TReadGuard, const TServiceData&> ReadData(EHostType service, const TUid& uid) const {
        const auto& bucket = Buckets[BucketIndex(uid)];
        return {TReadGuard(bucket.Mutex), bucket.ServiceToData[service]};
    }

    std::pair<TWriteGuard, TServiceData&> WriteData(EHostType service, const TUid& uid) {
        auto& bucket = Buckets[BucketIndex(uid)];
        return {TWriteGuard(bucket.Mutex), bucket.ServiceToData[service]};
    }

    size_t BucketIndex(const TUid& uid) const {
        return THash<TUid>()(uid) % Buckets.size();
    }

private:
    std::array<TVector<EHostType>, HOST_NUMTYPES> ServiceToParents;
    TVector<TBucket> Buckets = TVector<TBucket>(1024);
    TCategorizedHistograms<std::atomic<size_t>, EHistogram, ESimpleUidType> Histograms;
    std::atomic<size_t> Size_{};
};

static_assert(sizeof(TRobotSet) < 10000, "TRobotSet is too large. It may cause stack overflow errors");


} // namespace NAntiRobot
