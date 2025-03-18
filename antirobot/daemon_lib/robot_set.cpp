#include "robot_set.h"

#include <antirobot/idl/cache_sync.pb.h>

#include <antirobot/lib/ar_utils.h>
#include <antirobot/lib/enum.h>

#include <google/protobuf/messagext.h>

#include <util/generic/cast.h>
#include <util/generic/maybe.h>
#include <util/stream/file.h>


namespace NAntiRobot {


namespace {
    TInstant ParseExpirationTime(ui64 timeRep, TInstant now) {
        auto time = TInstant::FromValue(timeRep);

        if (time <= now) {
            time = TInstant::Zero();
        }

        return time;
    }

    TDuration GetLifetime(TUid::ENameSpace ns) {
        static_assert(TUid::ENameSpace::Count == 13, "Possibly need to configure new namespace");

        switch (ns) {
        case TUid::ENameSpace::LCOOKIE:
            return ANTIROBOT_DAEMON_CONFIG.AmnestyLCookieInterval;
        case TUid::ENameSpace::ICOOKIE:
            return ANTIROBOT_DAEMON_CONFIG.AmnestyICookieInterval;
        case TUid::ENameSpace::FUID:
            return ANTIROBOT_DAEMON_CONFIG.AmnestyFuidInterval;
        case TUid::ENameSpace::SPRAVKA:
            return ANTIROBOT_DAEMON_CONFIG.SpravkaExpireInterval;
        case TUid::ENameSpace::IP6:
            return ANTIROBOT_DAEMON_CONFIG.AmnestyIpV6Interval;
        default:
            return ANTIROBOT_DAEMON_CONFIG.AmnestyIpInterval;
        }
    }

    template <typename TYqlRules, typename TCbbRules>
    auto RulesToEntries(const TYqlRules& yqlRules, const TCbbRules& cbbRules) {
        return Concatenate(
            MakeMappedRange(yqlRules, [] (TCbbRuleId rule) {
                return TRobotSet::TBanSet::TEntry(rule);
            }),
            MakeMappedRange(cbbRules, [] (TCbbRuleKey rule) {
                return TRobotSet::TBanSet::TEntry(rule);
            })
        );
    }
}


TRobotSet::TRobotSet(IInputStream* input, TInstant now) {
    Load(input, now);
}

void TRobotSet::Load(IInputStream* input, TInstant now) {
    Clear();

    NProtoBuf::io::TCopyingInputStreamAdaptor adaptor(input);

    NCacheSyncProto::TBanAction action;

    while (NProtoBuf::io::ParseFromZeroCopyStreamSeq(&action, &adaptor)) {
        const TUid uid(action.GetUid());
        const auto service = ParseHostType(action.GetHostType());
        const auto matrixnetTime = ParseExpirationTime(action.GetMatrixnetExpirationTime(), now);
        auto maxTime = matrixnetTime;
        TBanSet bans(action.GetYqlEntries(), action.GetCbbEntries(), now, &maxTime);

        auto& data = Buckets[BucketIndex(uid)].ServiceToData[service];

        if (data.Contains(uid)) {
            continue;
        }

        TInstant uidRefTime;
        const TUid* uidRef = nullptr;

        if (matrixnetTime != TInstant::Zero()) {
            const auto [item, _] = data.ExpirationQueue.insert({matrixnetTime, uid});
            uidRefTime = matrixnetTime;
            uidRef = &item->Uid;
        }

        for (const auto& [time, _] : bans.GetTimeToRules()) {
            const auto [item, _inserted] = data.ExpirationQueue.insert({time, uid});

            if (time > uidRefTime) {
                uidRefTime = time;
                uidRef = &item->Uid;
            }
        }

        if (matrixnetTime != TInstant::Zero()) {
            data.UidToMatrixnet[TUidRefKey(uidRef)] = matrixnetTime;
        }

        if (!bans.Empty()) {
            data.UidToBanInfo[TUidRefKey(uidRef)] = {maxTime, std::move(bans)};
        }

        ++Histograms.Get(UidNsToSimpleUidType(uid.GetNameSpace()), EHistogram::Size);
        ++Size_;

        action.Clear();
    }
}

void TRobotSet::AddManual(
    EHostType service,
    const TUid& uid,
    TInstant expirationTime,
    bool matrixnet,
    const TVector<TCbbRuleId>& yqlRules,
    const TVector<TCbbRuleKey>& cbbRules
) {
    if (!matrixnet && yqlRules.empty() && cbbRules.empty()) {
        return;
    }

    const auto [guard, data] = WriteData(service, uid);
    UnsafeAdd(
        uid,
        expirationTime,
        matrixnet,
        RulesToEntries(yqlRules, cbbRules),
        !yqlRules.empty() || !cbbRules.empty(),
        &data
    );
}

void TRobotSet::Add(
    EHostType service,
    const TUid& uid,
    TInstant arrivalTime,
    bool matrixnet,
    const TVector<TCbbRuleId>& yqlRules,
    const TVector<TCbbRuleKey>& cbbRules
) {
    const auto expirationTime = arrivalTime + GetLifetime(uid.GetNameSpace());
    AddManual(service, uid, expirationTime, matrixnet, yqlRules, cbbRules);
}

bool TRobotSet::Contains(EHostType service, const TUid& uid) const {
    const auto [guard, bucket] = ReadBucket(uid);

    if (bucket.ServiceToData[service].Contains(uid)) {
        return true;
    }

    for (const auto parent : ServiceToParents[service]) {
        if (bucket.ServiceToData[parent].Contains(uid)) {
            return true;
        }
    }

    return false;
}

std::pair<TVector<TCbbRuleId>, TVector<TCbbRuleKey>> TRobotSet::GetRules(
    EHostType service,
    const TUid& uid
) const {
    const auto [guard, data] = ReadData(service, uid);

    if (const auto* banInfo = data.UidToBanInfo.FindPtr(TUidRefKey(&uid))) {
        return banInfo->Bans.GetRules();
    }

    return {};
}

TInstant TRobotSet::GetExpirationTime(EHostType service, const TUid& uid) const {
    const auto [guard, bucket] = ReadBucket(uid);

    auto ret = bucket.ServiceToData[service].GetExpirationTime(uid);

    for (const auto parent : ServiceToParents[service]) {
        ret = Max(ret, bucket.ServiceToData[parent].GetExpirationTime(uid));
    }

    return ret;
}

TBanReasons TRobotSet::GetBanReasons(EHostType service, const TUid& uid) const {
    const auto [guard, bucket] = ReadBucket(uid);
    auto reasons = bucket.ServiceToData[service].GetBanReasons(uid);

    for (const auto parent : ServiceToParents[service]) {
        reasons = reasons || bucket.ServiceToData[parent].GetBanReasons(uid);
    }

    return reasons;
}

void TRobotSet::RemoveExpired(TInstant time) {
    for (auto& bucket : Buckets) {
        TWriteGuard guard(bucket.Mutex);

        for (auto& data : bucket.ServiceToData) {
            const TExpirationQueueItem firstNotExpired(time, TUid::Max());
            const auto expiredEnd = data.ExpirationQueue.upper_bound(firstNotExpired);

            for (auto queueItem = data.ExpirationQueue.begin(); queueItem != expiredEnd; ++queueItem) {
                const TUidRefKey uidKey(&queueItem->Uid);
                bool noBanInfo = false;

                if (
                    const auto uidMatrixnet = data.UidToMatrixnet.find(uidKey);
                    uidMatrixnet != data.UidToMatrixnet.end() &&
                    uidMatrixnet->second == queueItem->Time
                ) {
                    data.UidToMatrixnet.erase(uidKey);
                }

                if (
                    const auto uidBanInfo = data.UidToBanInfo.find(uidKey);
                    uidBanInfo != data.UidToBanInfo.end()
                ) {
                    uidBanInfo->second.Remove(queueItem->Time);

                    if (uidBanInfo->second.Bans.Empty()) {
                        noBanInfo = true;
                        data.UidToBanInfo.erase(uidBanInfo);
                    }
                } else {
                    noBanInfo = true;
                }

                if (noBanInfo) {
                    --Histograms.Get(UidNsToSimpleUidType(queueItem->Uid.GetNameSpace()), EHistogram::Size);
                    --Size_;
                }
            }

            data.ExpirationQueue.erase(data.ExpirationQueue.begin(), expiredEnd);
        }
    };
}

void TRobotSet::Clear(EHostType service) {
    if (service == HOST_NUMTYPES) {
        for (auto& bucket : Buckets) {
            TWriteGuard guard(bucket.Mutex);

            for (auto& data : bucket.ServiceToData) {
                UnsafeClearData(&data);
            }
        }
    } else {
        for (auto& bucket : Buckets) {
            TWriteGuard guard(bucket.Mutex);
            UnsafeClearData(&bucket.ServiceToData[service]);
        }
    }
}

void TRobotSet::WriteSyncResponse(
    const TUid& uid,
    const std::array<TAllBans, HOST_NUMTYPES>& serviceToReceivedBans,
    NProtoBuf::RepeatedPtrField<NCacheSyncProto::TBanAction>* actions
) const {
    const auto [guard, bucket] = ReadBucket(uid);

    for (const auto& [service, data] : Enumerate(bucket.ServiceToData)) {
        NCacheSyncProto::TBanAction action;
        bool added = false;

        if (
            const auto uidMatrixnet = data.UidToMatrixnet.find(TUidRefKey(&uid));
            uidMatrixnet != data.UidToMatrixnet.end() &&
            uidMatrixnet->second > serviceToReceivedBans[service].Matrixnet
        ) {
            action.SetMatrixnetExpirationTime(uidMatrixnet->second.GetValue());
            added = true;
        }

        if (const auto* banInfo = data.UidToBanInfo.FindPtr(TUidRefKey(&uid))) {
            added = added || banInfo->Bans.WriteSyncResponse(
                serviceToReceivedBans[service].Bans,
                action.MutableYqlEntries(),
                action.MutableCbbEntries()
            );
        }

        if (added) {
            uid.SerializeTo(action.MutableUid());
            action.SetHostType(service);

            *actions->Add() = std::move(action);
        }
    }
}

void TRobotSet::ApplySyncRequest(
    const NProtoBuf::RepeatedPtrField<NCacheSyncProto::TBanAction>& actions
) {
    const auto now = TInstant::Now();

    TBucket* bucket = nullptr;
    TMaybe<TWriteGuard> guard;

    TUid lastUid;

    for (const auto& action : actions) {
        const TUid uid(action.GetUid());

        if (uid == TUid()) {
            continue;
        }

        if (uid != lastUid) {
            lastUid = uid;
            bucket = &Buckets[BucketIndex(uid)];
            guard.ConstructInPlace(bucket->Mutex);
        }

        const auto service = ParseHostType(action.GetHostType());
        auto* data = &bucket->ServiceToData[service];

        const auto matrixnetTime = ParseExpirationTime(action.GetMatrixnetExpirationTime(), now);
        const TBanSet bans(action.GetYqlEntries(), action.GetCbbEntries(), now);

        if (matrixnetTime != TInstant::Zero()) {
            UnsafeAdd(uid, matrixnetTime, true, std::initializer_list<TBanSet::TEntry>{}, false, data);
        }

        for (const auto& [time, rules] : bans.GetTimeToRules()) {
            UnsafeAdd(uid, time, false, rules, true, data);
        }
    }
}

void TRobotSet::SerializeTo(IOutputStream* output) const {
    NCacheSyncProto::TBanAction action;

    NProtoBuf::io::TCopyingOutputStreamAdaptor adaptor(output);

    for (const auto& bucket : Buckets) {
        TReadGuard guard(bucket.Mutex);

        for (const auto& [service, data] : Enumerate(bucket.ServiceToData)) {
            for (const auto& [uid, banInfo] : data.UidToBanInfo) {
                uid.Uid->SerializeTo(action.MutableUid());
                action.SetHostType(service);

                if (
                    const auto uidMatrixnet = data.UidToMatrixnet.find(uid);
                    uidMatrixnet != data.UidToMatrixnet.end()
                ) {
                    action.SetMatrixnetExpirationTime(uidMatrixnet->second.GetValue());
                }

                banInfo.Bans.SerializeTo(action.MutableYqlEntries(), action.MutableCbbEntries());

                Y_ENSURE(
                    NProtoBuf::io::SerializeToZeroCopyStreamSeq(&action, &adaptor),
                    "Failed to serialize TRobotSet (full entry)"
                );

                action.Clear();
            }

            for (const auto& [uid, matrixnet] : data.UidToMatrixnet) {
                if (data.UidToBanInfo.contains(uid)) {
                    continue;
                }

                uid.Uid->SerializeTo(action.MutableUid());
                action.SetHostType(service);
                action.SetMatrixnetExpirationTime(matrixnet.GetValue());

                Y_ENSURE(
                    NProtoBuf::io::SerializeToZeroCopyStreamSeq(&action, &adaptor),
                    "Failed to serialize TRobotSet (mxnet entry)"
                );

                action.Clear();
            }
        }
    }
}

void TRobotSet::LockedSave(const TString& path) const {
    static TMutex mutex;

    if (auto guard = TTryGuard<TMutex>(mutex)) {
        TFileOutput output(path);
        SerializeTo(&output);
    }
}

template <typename TEntries>
void TRobotSet::UnsafeAdd(
    const TUid& uid,
    TInstant expirationTime,
    bool matrixnet,
    const TEntries& entries,
    bool hasEntries,
    TServiceData* data
) {
    bool added = false;

    const TUidRefKey* matrixnetKeytoBePatched = nullptr;
    TInstant currentMatrixnet;
    TInstant deletedMatrixnet;
    bool hadMatrixnet = false;

    if (matrixnet) {
        const auto [uidMatrixnet, inserted] =
            data->UidToMatrixnet.emplace(TUidRefKey(&uid), expirationTime);

        hadMatrixnet = !inserted;
        bool updated = expirationTime > uidMatrixnet->second;

        if (updated) {
            deletedMatrixnet = uidMatrixnet->second;
            uidMatrixnet->second = expirationTime;
        }

        added = inserted || updated;

        if (added) {
            matrixnetKeytoBePatched = &uidMatrixnet->first;
        }

        currentMatrixnet = uidMatrixnet->second;
    } else if (
        const auto uidMatrixnet = data->UidToMatrixnet.find(TUidRefKey(&uid));
        uidMatrixnet != data->UidToMatrixnet.end()
    ) {
        currentMatrixnet = uidMatrixnet->second;
        hadMatrixnet = true;
    }

    const TUidRefKey* banInfoKeyToBePatched = nullptr;
    TBanSet* currentBans = nullptr;
    bool hadBanInfo = false;

    if (hasEntries) {
        const auto [uidBanInfo, inserted] = data->UidToBanInfo.try_emplace(
            TUidRefKey(&uid),
            Max(expirationTime, currentMatrixnet),
            TBanSet()
        );

        currentBans = &uidBanInfo->second.Bans;
        hadBanInfo = !inserted;

        if (inserted) {
            banInfoKeyToBePatched = &uidBanInfo->first;
            *currentBans = TBanSet(expirationTime, entries);
            added = true;
        } else {
            if (expirationTime > uidBanInfo->second.Max) {
                banInfoKeyToBePatched = &uidBanInfo->first;
                uidBanInfo->second.Max = expirationTime;
            }

            for (const auto rule : entries) {
                const auto addResult = currentBans->Add(rule, expirationTime);

                if (
                    addResult.DeletedTime != TInstant::Zero() &&
                    addResult.DeletedTime != currentMatrixnet
                ) {
                    data->ExpirationQueue.erase({addResult.DeletedTime, uid});
                }

                if (addResult.Added && expirationTime != currentMatrixnet) {
                    added = true;
                }

                added = added || (addResult.Added && expirationTime != currentMatrixnet);
            }
        }
    } else if (const auto banInfo = data->UidToBanInfo.FindPtr(TUidRefKey(&uid))) {
        currentBans = &banInfo->Bans;
        hadBanInfo = true;
    }

    if (added) {
        if (
            deletedMatrixnet != TInstant::Zero() &&
            (!currentBans || !currentBans->Contains(deletedMatrixnet))
        ) {
            data->ExpirationQueue.erase({deletedMatrixnet, uid});
        }

        const auto [queueItem, _] = data->ExpirationQueue.emplace(expirationTime, uid);

        for (const auto keyToBePatched : {matrixnetKeytoBePatched, banInfoKeyToBePatched}) {
            if (keyToBePatched) {
                keyToBePatched->Uid = &queueItem->Uid;
            }
        }
    }

    if (!hadMatrixnet && !hadBanInfo) {
        ++Histograms.Get(UidNsToSimpleUidType(uid.GetNameSpace()), EHistogram::Size);
        ++Size_;
    }
}

void TRobotSet::UnsafeClearData(TServiceData* data) {
    size_t numRemoved = 0;

    for (const auto& [uid, _] : data->UidToBanInfo) {
        --Histograms.Get(UidNsToSimpleUidType(uid.Uid->GetNameSpace()), EHistogram::Size);
        ++numRemoved;
    }

    for (const auto& [uid, _] : data->UidToMatrixnet) {
        if (!data->UidToBanInfo.contains(uid)) {
            --Histograms.Get(UidNsToSimpleUidType(uid.Uid->GetNameSpace()), EHistogram::Size);
            ++numRemoved;
        }
    }

    Size_ -= numRemoved;

    data->UidToMatrixnet.clear();
    data->UidToBanInfo.clear();
    data->ExpirationQueue.clear();
}


TRobotSet::TBanSet::TBanSet(
    const NProtoBuf::RepeatedPtrField<NCacheSyncProto::TCbbBanEntry>& pbYqlBans,
    const NProtoBuf::RepeatedPtrField<NCacheSyncProto::TCbbBanEntry>& pbCbbBans,
    TInstant now,
    TInstant* outMaxTime
) {
    const auto totalBans = pbYqlBans.size() + pbCbbBans.size();
    TimeToRules.reserve(totalBans);
    RuleToTime.reserve(totalBans);

    TInstant maxTime;

    for (const auto& pbYqlBan : pbYqlBans) {
        const auto time = TInstant::FromValue(pbYqlBan.GetExpirationTime());

        if (time <= now) {
            continue;
        }

        for (const auto& pbRule : pbYqlBan.GetRules()) {
            Add(TEntry(SafeIntegerCast<TCbbRuleId>(pbRule.GetEntry())), time);
        }

        maxTime = ::Max(maxTime, time);
    }

    for (const auto& pbCbbBan : pbCbbBans) {
        const auto time = TInstant::FromValue(pbCbbBan.GetExpirationTime());

        if (time <= now) {
            continue;
        }

        for (const auto& pbRule : pbCbbBan.GetRules()) {
            Add(TEntry(TCbbRuleKey(pbRule)), time);
        }

        maxTime = ::Max(maxTime, time);
    }

    if (outMaxTime) {
        *outMaxTime = ::Max(*outMaxTime, maxTime);
    }
}

TRobotSet::TBanSet::TAddResult TRobotSet::TBanSet::Add(TEntry rule, TInstant time) {
    TAddResult result;

    if (const auto [oldRuleTime, inserted] = RuleToTime.emplace(rule, time); inserted) {
        result.Added = true;
        RuleToTime[rule] = time;
        TimeToRules[time].insert(rule);
    } else {
        auto& oldTime = oldRuleTime->second;

        if (time > oldTime) {
            result.Added = true;

            const auto oldTimeRules = TimeToRules.find(oldTime);
            oldTimeRules->second.erase(rule);

            if (oldTimeRules->second.empty()) {
                result.DeletedTime = oldTime;
                TimeToRules.erase(oldTimeRules);
            }

            TimeToRules[time].insert(rule);
            oldTime = time;
        }
    }

    return result;
}

void TRobotSet::TBanSet::Remove(TInstant time) {
    if (const auto timeRules = TimeToRules.find(time); timeRules != TimeToRules.end()) {
        for (const auto& rule : timeRules->second) {
            RuleToTime.erase(rule);
        }

        TimeToRules.erase(timeRules);
    }
}

std::pair<TVector<TCbbRuleId>, TVector<TCbbRuleKey>> TRobotSet::TBanSet::GetRules() const {
    std::pair<TVector<TCbbRuleId>, TVector<TCbbRuleKey>> rules;

    for (const auto& [rule, _] : RuleToTime) {
        rule.Visit(
            [&rules] (TCbbRuleId rule) { rules.first.push_back(rule); },
            [&rules] (TCbbRuleKey rule) { rules.second.push_back(rule); }
        );
    }

    return rules;
}

TBanReasons TRobotSet::TBanSet::GetBanReasons() const {
    TBanReasons ret;

    for (const auto& [rule, _] : RuleToTime) {
        rule.Visit(
            [&ret] (TCbbRuleId) { ret.Yql = true; },
            [&ret] (TCbbRuleKey) { ret.Cbb = true; }
        );
    }

    return ret;
}

bool TRobotSet::TBanSet::WriteSyncResponse(
    const TBanSet& received,
    NProtoBuf::RepeatedPtrField<NCacheSyncProto::TCbbBanEntry>* pbYqlBans,
    NProtoBuf::RepeatedPtrField<NCacheSyncProto::TCbbBanEntry>* pbCbbBans
) const {
    return SerializeIf(
        [&received] (TEntry rule, TInstant time) {
            return !received.Contains(rule, time);
        },
        pbYqlBans, pbCbbBans
    );
}

void TRobotSet::TBanSet::SerializeTo(
    NProtoBuf::RepeatedPtrField<NCacheSyncProto::TCbbBanEntry>* pbYqlBans,
    NProtoBuf::RepeatedPtrField<NCacheSyncProto::TCbbBanEntry>* pbCbbBans
) const {
    SerializeIf([] (TEntry, TInstant) { return true; }, pbYqlBans, pbCbbBans);
}

template <typename F>
bool TRobotSet::TBanSet::SerializeIf(
    const F& condition,
    NProtoBuf::RepeatedPtrField<NCacheSyncProto::TCbbBanEntry>* pbYqlBans,
    NProtoBuf::RepeatedPtrField<NCacheSyncProto::TCbbBanEntry>* pbCbbBans
) const {
    bool nonEmpty = false;

    for (const auto& [time, rules] : TimeToRules) {
        NCacheSyncProto::TCbbBanEntry* pbYqlEntry = nullptr;
        NCacheSyncProto::TCbbBanEntry* pbCbbEntry = nullptr;

        for (const auto rule : rules) {
            if (!condition(rule, time)) {
                continue;
            }

            const auto [pbEntries, pbEntry] = rule.Visit(
                [pbYqlBans, &pbYqlEntry] (TCbbRuleId) { return std::tuple(pbYqlBans, &pbYqlEntry); },
                [pbCbbBans, &pbCbbEntry] (TCbbRuleKey) { return std::tuple(pbCbbBans, &pbCbbEntry); }
            );

            if (!*pbEntry) {
                nonEmpty = true;
                *pbEntry = pbEntries->Add();
                (*pbEntry)->SetExpirationTime(time.GetValue());
            }

            rule.SerializeTo((*pbEntry)->AddRules());
        }
    }

    return nonEmpty;
}


void TRobotSet::TBanSet::TEntry::SerializeTo(NCacheSyncProto::TCbbRule* pbRule) const {
    Visit(
        [pbRule] (TCbbRuleId rule) { pbRule->SetEntry(EnumValue(rule)); },
        [pbRule] (TCbbRuleKey rule) { rule.SerializeTo(pbRule); }
    );
}


TRobotSet::TAllBans::TAllBans(
    const NCacheSyncProto::TBanAction& action,
    TInstant now
)
    : Matrixnet(ParseExpirationTime(action.GetMatrixnetExpirationTime(), now))
    , Bans(action.GetYqlEntries(), action.GetCbbEntries(), now)
{}


void TRobotSet::TBanInfo::Remove(TInstant time) {
    if (Max == time) {
        Max = TInstant::Zero();
    }

    Bans.Remove(time);
}


bool TRobotSet::TServiceData::Contains(const TUid& uid) const {
    const TUidRefKey key(&uid);
    return UidToMatrixnet.contains(key) || UidToBanInfo.contains(key);
}

TInstant TRobotSet::TServiceData::GetExpirationTime(const TUid& uid) const {
    const TUidRefKey key(&uid);
    TInstant ret;

    if (
        const auto uidMatrixnet = UidToMatrixnet.find(key);
        uidMatrixnet != UidToMatrixnet.end()
    ) {
        ret = uidMatrixnet->second;
    }

    if (const auto banInfo = UidToBanInfo.FindPtr(key)) {
        ret = Max(ret, banInfo->Max);
    }

    return ret;
}

TBanReasons TRobotSet::TServiceData::GetBanReasons(const TUid& uid) const {
    TBanReasons ret;

    if (const auto* banInfo = UidToBanInfo.FindPtr(TUidRefKey(&uid))) {
        ret = banInfo->Bans.GetBanReasons();
    }

    if (UidToMatrixnet.contains(TUidRefKey(&uid))) {
        ret.Matrixnet = true;
    }

    return ret;
}


} // namespace NAntiRobot
