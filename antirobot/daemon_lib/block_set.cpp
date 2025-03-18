#include "block_set.h"

#include <antirobot/lib/ar_utils.h>

#include <util/stream/input.h>
#include <util/stream/output.h>
#include <util/string/subst.h>
#include <util/system/mutex.h>

namespace NAntiRobot {

bool TBlockRecordCompare::operator()(const TBlockRecord& first, const TBlockRecord& second) const {
    if (first.Uid.Ns != second.Uid.Ns) {
        return first.Uid.Ns < second.Uid.Ns;
    }
    if (first.Uid.AggrLevel != second.Uid.AggrLevel) {
        return first.Uid.AggrLevel < second.Uid.AggrLevel;
    }
    if (first.Uid.Id != second.Uid.Id) {
        return first.Uid.Id < second.Uid.Id;
    }
    if (first.Uid.IdHi != second.Uid.IdHi) {
        return first.Uid.IdHi < second.Uid.IdHi;
    }
    return first.Category < second.Category;
}

const TBlockRecord* FindBlockRecord(const TBlockSet& set, const TUid& uid, EBlockCategory category) {
    TBlockRecord record;
    record.Uid = uid;
    record.Category = category;
    auto iter = set.find(record);
    return iter != set.end() ? &*iter : nullptr;
}

TVector<TBlockRecord> GetAllItemsWithUid(const TBlockSet& set, const TUid& uid) {
    TBlockRecord record;
    record.Uid = uid;

    record.Category = BLOCK_CATEGORY_FIRST;
    auto begin = set.lower_bound(record);

    record.Category = BLOCK_CATEGORY_LAST;
    auto end = set.upper_bound(record);

    return {begin, end};
}

void SaveBlockSet(const TBlockSet& blockSet, const TFsPath& filename) {
    static TMutex BlockSetMutex;
    FlushToFileIfNotLocked(blockSet, filename, BlockSetMutex);
}

}

using NAntiRobot::TBlockSet;
using NAntiRobot::TBlockRecord;

template <>
void Out<TBlockSet>(IOutputStream& out, const TBlockSet& blockSet) {
    for (const auto& record : blockSet) {
        out << record << '\n';
    }
}

template <>
void In<TBlockSet>(IInputStream& in, TBlockSet& blockSet) {
    TBlockSet local;
    for (TString line; in.ReadLine(line); ) {
        TStringInput si(line);

        TBlockRecord record;
        si >> record;
        local.insert(record);
    }
    blockSet.swap(local);
}

template <>
void Out<TBlockRecord>(IOutputStream& out, const TBlockRecord& record) {
    const char delim = '\t';

    TString description = record.Description;
    SubstGlobal(description, ' ', '+');

    out << record.Uid
        << delim << record.Category
        << delim << record.Addr
        << delim << (record.YandexUid.empty() ? "-" : record.YandexUid)
        << delim << record.Status
        << delim << record.ExpireTime.GetValue()
        << delim << description
        ;
}

template <>
void In<TBlockRecord>(IInputStream& in, TBlockRecord& record) {
    TString blockCategory, addr, blockStatus;
    TInstant::TValue expireTime;

    in >> record.Uid
       >> blockCategory
       >> addr
       >> record.YandexUid
       >> blockStatus
       >> expireTime
       >> record.Description
       ;

    record.Category = FromString<NAntiRobot::EBlockCategory>(blockCategory);

    record.Addr.FromString(addr);
    Y_ENSURE(record.Addr != NAntiRobot::TAddr() || addr == "-", "Invalid addr " + addr);

    record.Status = FromString<NAntiRobot::EBlockStatus>(blockStatus);
    record.ExpireTime = TInstant::MicroSeconds(expireTime);

    if (record.YandexUid == "-") {
        record.YandexUid = "";
    }
    SubstGlobal(record.Description, '+', ' ');
}

