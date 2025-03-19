#pragma once

#include <kernel/doom/search_fetcher/search_fetcher.h>

#include <util/generic/hash.h>
#include <util/generic/variant.h>


class TSearchDocDataHolder {
public:
    TSearchDocDataHolder() = default;

    bool Contains(ui32 docId) const {
        return DataHolder_.contains(docId);
    }

    void PreLoad(ui32 docId, NDoom::TSearchDocLoader&& loader) {
        DataHolder_[docId] = std::move(loader);
    }

    void PreLoad(ui32 docId, THolder<NDoom::IDocLumpLoader> loader) {
        DataHolder_[docId] = std::move(loader);
    }

    const NDoom::IDocLumpLoader& Get(ui32 docId) const {
        Y_ENSURE(Contains(docId));
        const auto& loader = DataHolder_.at(docId);

        if (std::holds_alternative<THolder<NDoom::IDocLumpLoader>>(loader)) {
            return *std::get<THolder<NDoom::IDocLumpLoader>>(loader);
        } else {
            return std::get<NDoom::TSearchDocLoader>(loader);
        }
    }

    void Remove(ui32 docId) {
        if (Contains(docId)) {
            DataHolder_.erase(docId);
        }
    }

private:
    THashMap<ui32, std::variant<NDoom::TSearchDocLoader, THolder<NDoom::IDocLumpLoader>>> DataHolder_;
};
