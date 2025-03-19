#pragma once

#include "types.h"
#include "item_type_map.h"

#include <library/cpp/containers/stack_vector/stack_vec.h>

#include <util/generic/array_ref.h>

namespace NDoom::NItemStorage::NPrivate {

inline constexpr size_t REQUEST_SIZE_HINT = 4;

struct TItemTypeLumpsRequest {
    TStackVec<TItemKey, REQUEST_SIZE_HINT> Items;
    TStackVec<size_t, REQUEST_SIZE_HINT> Lumps;
};

class TItemLumpsRequest {
public:
    void AddItem(TItemId item) {
        RequestsPerItemType_[item.ItemType].Items.push_back(item.ItemKey);
    }

    void AddItem(TItemType type, TItemKey key) {
        RequestsPerItemType_[type].Items.push_back(key);
    }

    void AddLumps(TItemType type, TConstArrayRef<size_t> lumps) {
        RequestsPerItemType_[type].Lumps.assign(lumps.begin(), lumps.end());
    }

    TConstArrayRef<TItemType> ItemTypes() const {
        return RequestsPerItemType_.ItemTypes();
    }

    TConstArrayRef<TItemTypeLumpsRequest> Requests() const {
        return RequestsPerItemType_.Values();
    }

    TItemTypeLumpsRequest& ItemTypeRequest(TItemType type) {
        return RequestsPerItemType_.at(type);
    }

    const TItemTypeLumpsRequest& ItemTypeRequest(TItemType type) const {
        return RequestsPerItemType_.at(type);
    }

    size_t NumItems() const {
        size_t num = 0;
        ForEachItem([&num](...) {
            ++num;
        });
        return num;
    }

    TStackVec<TItemId> Items() const {
        TStackVec<TItemId> res;
        ForEachItem([&res](size_t, TItemId item) {
            res.push_back(item);
        });
        return res;
    }

    template <typename F>
    void ForEachItem(F callback) const {
        size_t i = 0;
        for (TItemType type : RequestsPerItemType_.ItemTypes()) {
            for (TItemKey key : RequestsPerItemType_.at(type).Items) {
                callback(i++, TItemId{
                    .ItemType = type,
                    .ItemKey = key,
                });
            }
        }
    }

private:
    TItemTypeMap<TItemTypeLumpsRequest> RequestsPerItemType_;
};

} // namespace NDoom::NItemStorage::NPrivate
