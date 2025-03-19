#include "validate.h"
#include "generate.h"
#include "index.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/xrange.h>
#include <util/random/fast.h>
#include <util/random/shuffle.h>

namespace NDoom::NItemStorage::NTest {

void TestGlobalLumps(const TIndexData& data, const THolder<IItemStorage>& storage) {
    TReallyFastRng32 rng{1337};

    TVector<TLumpId> lumpsList = storage->GlobalLumps();
    TVector<TLumpId> expectedList;

    for (const TItemIndex& index : data.Items) {
        for (TStringBuf lump : index.GlobalLumpsList) {
            expectedList.push_back(TLumpId{ index.ItemType, TString{lump} });
        }
    }
    auto getLump = [](const TLumpId& lump) {
        return std::pair{lump.ItemType, lump.LumpName};
    };
    SortBy(lumpsList, getLump);
    SortBy(expectedList, getLump);
    UNIT_ASSERT_VALUES_EQUAL(lumpsList, expectedList);

    for (const TItemIndex& index : data.Items) {
        for (auto [name, data] : index.GlobalLumps) {
            UNIT_ASSERT(storage->HasGlobalLump(TLumpId{ index.ItemType, name }));
            TBlob blob = storage->LoadGlobalLump(TLumpId{ index.ItemType, name });
            TStringBuf lump{blob.AsCharPtr(), blob.Size()};
            UNIT_ASSERT_VALUES_EQUAL(data, lump);
        }

        for (ui32 i = 0; i < 10; ++i) {
            TString randomName = RandomString(40, rng());
            UNIT_ASSERT(!storage->HasGlobalLump(TLumpId{ index.ItemType, randomName }));
        }
    }

    for (ui32 i = 0; i < 10; ++i) {
        TString randomName = RandomString(40, rng());
        UNIT_ASSERT(!storage->HasGlobalLump(TLumpId{ static_cast<TItemType>(rng()), randomName }));
    }
}

void TestChunkGlobalLumps(const TIndexData& data, const THolder<IItemStorage>& storage) {
    for (const TItemIndex& index : data.Items) {
        for (const TChunk& chunk : index.Chunks) {
            for (auto&& [name, data] : chunk.ChunkGlobalLumps) {
                UNIT_ASSERT(storage->HasChunkGlobalLump(chunk.Id, TLumpId{ index.ItemType, name }));
                TBlob blob = storage->LoadChunkGlobalLump(chunk.Id, TLumpId{ index.ItemType, name });
                TStringBuf lump{blob.AsCharPtr(), blob.Size()};
                UNIT_ASSERT_VALUES_EQUAL(data, lump);
            }
        }
    }
}

class TReaskLimiter {
public:
    TReaskLimiter(float increment = 0.01f)
        : Budget_{0.0f}
        , Increment_{increment}
    {
    }

    bool AllowReask() {
        if (Budget_ > 1.0f) {
            Budget_ -= 1.0f;
            return true;
        }
        return false;
    }

    void Increment() {
        Budget_ += Increment_;
    }

private:
    float Budget_ = 0.0f;
    float Increment_ = 0.0f;
};

struct TItem {
    TItemId Id;
    TChunkId Chunk;
    const TLumps* Lumps = nullptr;
};

void TestItemLumps(const TIndexData& data, const THolder<IItemStorage>& storage) {
    TReallyFastRng32 rng{42};

    auto requester = storage->MakeRequester();

    TVector<TItem> items;
    THashSet<TItemId> presentIds;
    TVector<TItemId> unknownItems;

    THashMap<TItemType, TVector<TLumpId>> lumps;
    THashMap<TItemType, TLumpsSet> mapping;
    for (const TItemIndex& index : data.Items) {
        THashSet<TStringBuf> uniqLumps;
        for (const TChunk& chunk : index.Chunks) {
            uniqLumps.insert(chunk.ItemLumpsList.begin(), chunk.ItemLumpsList.end());
            for (auto&& [item, lumps] : chunk.ItemLumps) {
                TItemId id{
                    .ItemType = index.ItemType,
                    .ItemKey = item,
                };
                items.push_back(TItem{
                    .Id = id,
                    .Chunk = chunk.Id,
                    .Lumps = &lumps,
                });
                presentIds.insert(id);
            }
        }

        for (TStringBuf name : uniqLumps) {
            lumps[index.ItemType].push_back(TLumpId{ index.ItemType, TString{name} });
        }
        mapping[index.ItemType] = requester->MapItemLumps(lumps[index.ItemType]);

        for ([[maybe_unused]] ui32 _ : xrange(1000)) {
            TItemId item = GenItemId(index.ItemType, rng);
            if (!presentIds.contains(item)) {
                unknownItems.push_back(item);
            }
        }
    }
    ShuffleRange(items, rng);

    TVector<TItemId> req;
    TVector<bool> visited;
    size_t numItemsLoaded = 0;
    size_t numEmptyItems = 0;

    TReaskLimiter limiter{0.02f}; // Allow to reask 2% of requests

    for (ui32 i = 0; i < items.size(); ++i) {
        for ([[maybe_unused]] ui32 rep : xrange(4)) {
            limiter.Increment();

            ui32 j = Min<ui32>(i + 1 + rng() % 10, items.size());
            j = i + 1;

            req.clear();
            visited.clear();
            for (ui32 k = i; k < j; ++k) {
                req.push_back(items[k].Id);
                visited.push_back(false);
            }

            bool reask = true;
            while (std::exchange(reask, false)) {
                requester->LoadItemLumps(req, [&](size_t index, TStatusOr<IItemLumps*> res) {
                    const TItem& item = items[i + index];
                    UNIT_ASSERT(item.Id.ItemKey == req[index].ItemKey);
                    UNIT_ASSERT(!visited[index]);

                    if (res.StatusCode() == EStatusCode::Unavailable && limiter.AllowReask()) {
                        reask = true;
                        Cerr << "Going to reask unavailable response for item " << item.Id << ", status: " << res.Status() << Endl;
                        return;
                    }

                    visited[index] = true;
                    ++numItemsLoaded;

                    if (item.Lumps->empty()) {
                        ++numEmptyItems;
                        UNIT_ASSERT_VALUES_EQUAL(res.StatusCode(), EStatusCode::NotFound);
                        return;
                    }

                    if (!res) {
                        if (!item.Lumps->empty()) {
                            Cerr << "Failed to load item " << item.Id.ItemKey << ", expected lumps: " << Endl;
                            for (auto&& [lump, data] : *item.Lumps) {
                                Cerr << lump << ": " << data << Endl;
                            }
                        }
                        UNIT_FAIL("Failed to load item " << ToString(item.Id.ItemKey) << " at chunk " << item.Chunk << ", found " << res.Status());
                        return;
                    }
                    UNIT_ASSERT(res);
                    auto* loader = res.Unwrap();

                    UNIT_ASSERT(loader->ChunkId() == item.Chunk);

                    TItemType type = item.Id.ItemType;

                    TVector<TConstArrayRef<char>> regions(mapping[type].NumLumps());
                    loader->LoadLumps(mapping[type], regions);

                    for (ui32 i : xrange(regions.size())) {
                        const TLumpId& lump = lumps[type][i];
                        if (!regions[i]) {
                            auto* ptr = item.Lumps->FindPtr(lump.LumpName);
                            UNIT_ASSERT(!ptr || ptr->empty());
                            continue;
                        }

                        TStringBuf region{ regions[i].data(), regions[i].size() };
                        UNIT_ASSERT_VALUES_EQUAL(region, item.Lumps->at(lump.LumpName));
                    }
                });
            }

            for (ui32 i = 0; i < visited.size(); ++i) {
                UNIT_ASSERT_C(visited[i], ("Item " + ToString(i) + " was not loaded"));
            }
        }
    }

    for (ui32 i = 0; i < unknownItems.size(); ++i) {
        visited = { false };
        req = { unknownItems[i] };

        bool reask = true;
        while (std::exchange(reask, false)) {
            requester->LoadItemLumps(req, [&](size_t index, TStatusOr<IItemLumps*> res) {
                if (res.StatusCode() == EStatusCode::Unavailable && limiter.AllowReask()) {
                    reask = true;
                    return;
                }

                UNIT_ASSERT_VALUES_EQUAL(res.StatusCode(), EStatusCode::NotFound);

                const TItemId& item = unknownItems[i + index];
                UNIT_ASSERT_VALUES_EQUAL(item, req[index]);

                visited.at(index) = true;
            });
        }

        for (ui32 i = 0; i < visited.size(); ++i) {
            UNIT_ASSERT_C(visited[i], ("Item " + ToString(i) + " was not loaded"));
        }
    }

    for (ui32 _ : xrange(100)) {
        Y_UNUSED(_);

        TVector<TLumpId> lumps;
        for (auto&& item : data.Items) {
            size_t numInvalidLumps = rng() % 4;
            while (numInvalidLumps-- > 0) {
                lumps.push_back(TLumpId{
                    .ItemType = item.ItemType,
                    .LumpName = RandomString(38, rng()),
                });
            }
        }

        auto requester = storage->MakeRequester();
        auto lumpsSet = requester->MapItemLumps(lumps);
        for (ui32 rep : xrange(10)) {
            if (items.empty()) {
                continue;
            }
            Y_UNUSED(rep);

            size_t i = rng() % items.size();
            auto item = items[i];
            requester->LoadItemLumps({item.Id}, [&](size_t, TStatusOr<IItemLumps*> res) {
                if (res.StatusCode() == EStatusCode::NotFound) {
                    UNIT_ASSERT(item.Lumps->empty());
                    return;
                }
                UNIT_ASSERT_C(res.StatusCode() == EStatusCode::Ok || res.StatusCode() == EStatusCode::Unavailable, "Unexpected status" << res.Status());
                if (res.StatusCode() != EStatusCode::Ok) {
                    return;
                }

                TVector<TConstArrayRef<char>> regions(lumps.size());
                res.Unwrap()->LoadLumps(lumpsSet, regions);
                for (auto&& region : regions) {
                    UNIT_ASSERT_C(region.empty(), "Got region of size " << region.size());
                }
            });
        }
    }

    Cerr << "Loaded " << numItemsLoaded << " items" << Endl;
    Cerr << "Empty " << numEmptyItems << " items" << Endl;
}

void TestContainsItemCheck(const TIndexData& data, const THolder<IItemStorage>& storage) {
    TReallyFastRng32 rng{42};

    auto requester = storage->MakeRequester();

    TVector<TItem> items;
    THashSet<TItemId> presentIds;
    TVector<TItemId> unknownItems;

    for (const TItemIndex& index : data.Items) {
        for (const TChunk& chunk : index.Chunks) {
            for (auto&& [item, lumps] : chunk.ItemLumps) {
                TItemId id{
                    .ItemType = index.ItemType,
                    .ItemKey = item,
                };
                items.push_back(TItem{
                    .Id = id,
                    .Chunk = chunk.Id,
                    .Lumps = &lumps,
                });
                presentIds.insert(id);
            }
        }

        for ([[maybe_unused]] ui32 _ : xrange(1000)) {
            TItemId item = GenItemId(index.ItemType, rng);
            if (!presentIds.contains(item)) {
                unknownItems.push_back(item);
            }
        }
    }
    ShuffleRange(items, rng);

    TVector<TItemId> req;
    TVector<bool> visited;
    size_t numItemsLoaded = 0;
    size_t numEmptyItems = 0;

    TReaskLimiter limiter{0.02f};
    for (ui32 i = 0; i < items.size(); ++i) {
        for ([[maybe_unused]] ui32 rep : xrange(4)) {
            limiter.Increment();

            ui32 j = Min<ui32>(i + 1 + rng() % 10, items.size());
            j = i + 1;

            req.clear();
            visited.clear();
            for (ui32 k = i; k < j; ++k) {
                req.push_back(items[k].Id);
                visited.push_back(false);
            }

            bool reask = true;
            while (std::exchange(reask, false)) {
                requester->LoadItemLumps(req, [&](size_t index, TStatusOr<IItemLumps*> res) {
                    const TItem& item = items[i + index];
                    UNIT_ASSERT(item.Id.ItemKey == req[index].ItemKey);
                    UNIT_ASSERT(!visited[index]);

                    if (res.StatusCode() == EStatusCode::Unavailable && limiter.AllowReask()) {
                        reask = true;
                        Cerr << "Going to reask unavailable response for item " << item.Id << ", status: " << res.Status() << Endl;
                        return;
                    }

                    visited[index] = true;
                    ++numItemsLoaded;

                    if (item.Lumps->empty()) {
                        ++numEmptyItems;
                        UNIT_ASSERT_VALUES_EQUAL(res.StatusCode(), EStatusCode::NotFound);
                        return;
                    }

                    if (!res) {
                        UNIT_FAIL("Failed to load item " << item.Id << " at chunk " << item.Chunk << ", found " << res.Status() << ", requested " << req.size() << " keys");
                        return;
                    }
                    UNIT_ASSERT(res);
                    auto* loader = res.Unwrap();

                    UNIT_ASSERT(loader->ChunkId() == item.Chunk);
                });
            }

            for (ui32 i = 0; i < visited.size(); ++i) {
                UNIT_ASSERT_C(visited[i], "Item " << i << " was not loaded");
            }
        }
    }

    for (ui32 i = 0; i < unknownItems.size(); ++i) {
        visited = { false };
        req = { unknownItems[i] };

        bool reask = true;
        while (std::exchange(reask, false)) {
            requester->LoadItemLumps(req, [&](size_t index, TStatusOr<IItemLumps*> res) {
                if (res.StatusCode() == EStatusCode::Unavailable && limiter.AllowReask()) {
                    reask = true;
                    return;
                }

                UNIT_ASSERT_VALUES_EQUAL(res.StatusCode(), EStatusCode::NotFound);

                const TItemId& item = unknownItems[i + index];
                UNIT_ASSERT_VALUES_EQUAL(item, req[index]);

                visited.at(index) = true;
            });
        }

        for (ui32 i = 0; i < visited.size(); ++i) {
            UNIT_ASSERT_C(visited[i], ("Item " + ToString(i) + " was not loaded"));
        }
    }

    Cerr << "Loaded " << numItemsLoaded << " items" << Endl;
    Cerr << "Empty " << numEmptyItems << " items" << Endl;
}

} // namespace NDoom::NItemStorage::NTest
