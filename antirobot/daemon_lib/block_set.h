#pragma once

#include "req_types.h"
#include "uid.h"

#include <antirobot/lib/addr.h>

#include <util/ysaveload.h>
#include <util/folder/path.h>
#include <util/generic/set.h>
#include <util/generic/vector.h>
#include <util/stream/file.h>

namespace NAntiRobot {

enum class EBlockStatus {
    None,
    Mark,
    Block,
};

struct TBlockRecord {
    TUid Uid;
    EBlockCategory Category;
    TAddr Addr;
    TString YandexUid;
    EBlockStatus Status;
    TInstant ExpireTime;
    TString Description;

    Y_SAVELOAD_DEFINE(Uid, Category, Addr, YandexUid, Status, ExpireTime, Description)
};

struct TBlockRecordCompare {
    bool operator()(const TBlockRecord& first, const TBlockRecord& second) const;
};

using TBlockSet = TSet<TBlockRecord, TBlockRecordCompare>;

const TBlockRecord* FindBlockRecord(const TBlockSet& set, const TUid& uid, EBlockCategory category);

TVector<TBlockRecord> GetAllItemsWithUid(const TBlockSet& set, const TUid& uid);

template <typename Predicate>
TVector<TBlockRecord> RemoveIf(TBlockSet& blockSet, Predicate pred) {
    TVector<TBlockRecord> result;

    for (auto i = blockSet.begin(); i != blockSet.end(); ) {
        if (pred(*i)) {
            result.push_back(*i);
            blockSet.erase(i++);
        } else {
            ++i;
        }
    }

    return result;
}

void SaveBlockSet(const TBlockSet& blockSet, const TFsPath& filename);

}

