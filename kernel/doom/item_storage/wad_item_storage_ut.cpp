#include <kernel/doom/item_storage/test/generate.h>
#include <kernel/doom/item_storage/test/write.h>
#include <kernel/doom/item_storage/test/validate.h>

#include <kernel/doom/item_storage/lumps_cache.h>
#include <kernel/doom/item_storage/wad_item_storage.h>
#include <kernel/doom/item_storage/wad_item_storage_writer.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/folder/tempdir.h>
#include <util/generic/xrange.h>
#include <util/random/fast.h>
#include <util/random/shuffle.h>

using namespace NDoom::NItemStorage;
using namespace NDoom::NItemStorage::NTest;

Y_UNIT_TEST_SUITE(TestItemStorage) {

    void WriteIndex(const TIndexData& index, const TFsPath& folder) {
        auto factory = NTest::MakeLocalWadWriterFactory(folder);
        return NTest::WriteIndex(index, factory.Get());
    }

    void TestIndexFiles(const TIndexData& data, const TFsPath& folder) {
        TVector<TString> children;
        folder.ListNames(children);

        THashSet<TString> paths(children.begin(), children.end());

        auto checkPath = [&folder, &paths](TString name) {
            TFsPath path = folder.Child(name);
            UNIT_ASSERT_C(path.Exists(), "File does " + name + " does not exist");
            UNIT_ASSERT_C(paths.contains(name), "Unknown file " + name);
            paths.erase(name);
        };

        for (const TItemIndex& index : data.Items) {
            const TString prefix = "item_" + ToString(index.ItemType);
            checkPath(prefix + ".global.wad");
            checkPath(prefix + ".mapping.wad");
            for (ui32 i : xrange(index.Chunks.size())) {
                checkPath(prefix + "." + ToString(i) + ".wad");
            }
        }

        if (!children.empty()) {
            for (TStringBuf child : paths) {
                Cerr << "Unexpected file " << child << " exists" << Endl;
            }
        }
        UNIT_ASSERT_C(paths.empty(), "Folder contains unknown files");
    }

    THolder<IItemStorage> BuildItemStorage(const TIndexData& data, const TFsPath& folder, bool enableCache) {
        TItemStorageBuilder builder;
        for (const TItemIndex& index : data.Items) {
            auto backend = MakeChunkedWadItemStorageBackend(index.ItemType, folder.GetPath() + "item_" + ToString(index.ItemType));
            if (enableCache) {
                THashSet<TString> lumpNames;
                for (auto&& chunk : index.Chunks) {
                    lumpNames.insert(chunk.ItemLumpsList.begin(), chunk.ItemLumpsList.end());
                }

                TTableCacheBuilder cacheBuilder;
                cacheBuilder
                    .SetItemType(index.ItemType)
                    .SetNumSlots(1000);

                for (TString lump : lumpNames) {
                    cacheBuilder.AddLump(TLumpCacheOptions{
                        .Lump = lump,
                        .NumPages = 1024,
                        .PageSize = 1024,
                        .MaxPagesPerLump = 16,
                        .ExpirationTime = TDuration::Seconds(2),
                    });
                }

                TVector<THolder<TTableCache>> caches;
                caches.push_back(cacheBuilder.Build());

                IItemLumpsStoragePtr cachedItemLumps = MakeIntrusive<TCachedLumpsStorage>(backend.ItemLumps(), std::move(caches));

                TItemStorageBackend cachedBackend = TItemStorageBackendBuilder{}
                    .SetItemType(index.ItemType)
                    .SetGlobalLumpsStorage(backend.GlobalLumps())
                    .SetChunkGlobalLumps(backend.ChunkGlobalLumps())
                    .SetItemLumps(cachedItemLumps)
                    .Build();

                backend = cachedBackend;
            }
            builder.AddBackend(backend);
        }
        return builder.Build();
    }

    void TestItemStorage(const TIndexData& data, const TFsPath& folder, bool enableCache) {
        auto storage = BuildItemStorage(data, folder, enableCache);

        TestGlobalLumps(data, storage);
        TestChunkGlobalLumps(data, storage);
        TestItemLumps(data, storage);
    }

    enum class ETestParam {
        CheckFiles = 0b001,
        CheckIndex = 0b010,
        CheckCache = 0b100,
        CheckEverything = CheckCache | CheckIndex | CheckFiles,
    };

    Y_DECLARE_FLAGS(ETestParams, ETestParam);
    Y_DECLARE_OPERATORS_FOR_FLAGS(ETestParams);

    void CheckItemStorage(const TIndexParams& params, ETestParams testParams = ETestParam::CheckIndex | ETestParam::CheckFiles) {
        TTempDir dir;
        TIndexData data = GenerateIndex(params);

        WriteIndex(data, dir.Path());
        if (testParams.HasFlags(ETestParam::CheckFiles)) {
            TestIndexFiles(data, dir.Path());
        }
        if (testParams.HasFlags(ETestParam::CheckIndex)) {
            TestItemStorage(data, dir.Path(), /*cache=*/ testParams.HasFlags(ETestParam::CheckCache));
        }
    }

    Y_UNIT_TEST(EmptyStorage) {
        CheckItemStorage(TIndexParams{});
    }

    Y_UNIT_TEST(ItemIdCollision) {
        UNIT_ASSERT_EXCEPTION(CheckItemStorage(TIndexParams{}
            .AddItems(TItemIndexParams{}
                .SetItemType(11)
                .SetGlobalLumps(2)
                .SetChunkLocalLumps(2)
                .SetItemLumps(4)
                .SetNumChunks(1)
                .SetNumItems(100)
            )
            .AddItems(TItemIndexParams{}
                .SetItemType(11)
                .SetGlobalLumps(2)
                .SetChunkLocalLumps(2)
                .SetItemLumps(4)
                .SetNumChunks(1)
                .SetNumItems(100)
            ),
            ETestParam::CheckIndex), NDoom::NItemStorage::TDuplicateItemTypeError);
    }

    Y_UNIT_TEST(OnlyGlobalStorage) {
        CheckItemStorage(TIndexParams{}
            .AddItems(TItemIndexParams{}
                .SetItemType(228)
                .SetGlobalLumps(2)
                .SetChunkLocalLumps(2)
                .SetItemLumps(4)
                .SetNumChunks(4)
                .SetNumItems(0)
            )
            .AddItems(TItemIndexParams{}
                .SetItemType(117)
                .SetGlobalLumps(0)
                .SetChunkLocalLumps(1)
                .SetItemLumps(1)
                .SetNumChunks(1)
                .SetNumItems(0)
            )
        );
    }

    Y_UNIT_TEST(OneChunkItemStorage) {
        CheckItemStorage(TIndexParams{}
            .AddItems(TItemIndexParams{}
                .SetItemType(0)
                .SetGlobalLumps(2)
                .SetChunkLocalLumps(2)
                .SetItemLumps(4)
                .SetNumChunks(1)
                .SetNumItems(100)
            )
        );
    }

    Y_UNIT_TEST(TinyItemStorage) {
        CheckItemStorage(TIndexParams{}
            .AddItems(TItemIndexParams{}
                .SetItemType(0)
                .SetGlobalLumps(2)
                .SetChunkLocalLumps(2)
                .SetItemLumps(4)
                .SetNumChunks(3)
                .SetNumItems(10)
            )
            .AddItems(TItemIndexParams{}
                .SetItemType(1)
                .SetGlobalLumps(0)
                .SetChunkLocalLumps(1)
                .SetItemLumps(1)
                .SetNumChunks(2)
                .SetNumItems(10)
            )
        );
    }

    Y_UNIT_TEST(BadMinhash) {
        CheckItemStorage(TIndexParams{}
            .AddItems(TItemIndexParams{}
                .SetItemType(15)
                .SetGlobalLumps(2)
                .SetChunkLocalLumps(2)
                .SetItemLumps(4)
                .SetNumChunks(2)
                .SetNumItems(400)
            )
        );
    }

    Y_UNIT_TEST(SmallItemStorage) {
        CheckItemStorage(TIndexParams{}
            .AddItems(TItemIndexParams{}
                .SetItemType(15)
                .SetGlobalLumps(2)
                .SetChunkLocalLumps(2)
                .SetItemLumps(4)
                .SetNumChunks(4)
                .SetNumItems(1000)
            )
            .AddItems(TItemIndexParams{}
                .SetItemType(0)
                .SetGlobalLumps(0)
                .SetChunkLocalLumps(1)
                .SetItemLumps(1)
                .SetNumChunks(1)
                .SetNumItems(5000)
            )
            .AddItems(TItemIndexParams{}
                .SetItemType(255)
                .SetGlobalLumps(4)
                .SetChunkLocalLumps(4)
                .SetItemLumps(0)
                .SetNumChunks(3)
                .SetNumItems(3000)
            )
        );
    }

    Y_UNIT_TEST(MediumItemStorage) {
        CheckItemStorage(TIndexParams{}
            .AddItems(TItemIndexParams{}
                .SetItemType(15)
                .SetGlobalLumps(2)
                .SetChunkLocalLumps(2)
                .SetItemLumps(4)
                .SetNumChunks(4)
                .SetNumItems(1000)
            )
            .AddItems(TItemIndexParams{}
                .SetItemType(0)
                .SetGlobalLumps(0)
                .SetChunkLocalLumps(1)
                .SetItemLumps(1)
                .SetNumChunks(2)
                .SetNumItems(5000)
            )
            .AddItems(TItemIndexParams{}
                .SetItemType(255)
                .SetGlobalLumps(4)
                .SetChunkLocalLumps(4)
                .SetItemLumps(3)
                .SetNumChunks(5)
                .SetNumItems(3000)
            )
            .AddItems(TItemIndexParams{}
                .SetItemType(13)
                .SetGlobalLumps(0)
                .SetChunkLocalLumps(1)
                .SetItemLumps(1)
                .SetNumChunks(2)
                .SetNumItems(5000)
            )
        );
    }

    Y_UNIT_TEST(Stress) {
        for (ui32 i : xrange(10)) {
            TReallyFastRng32 rng{NumericHash(i)};

            TIndexParams params;
            params.SetSeed(rng());

            ui32 numItems = rng.Uniform(0, 10);
            THashSet<ui32> items;
            while (items.size() < numItems) {
                ui32 itemType = rng.Uniform(1, 255);
                auto [it, inserted] = items.insert(itemType);
                if (!inserted) {
                    continue;
                }

                params.AddItems(TItemIndexParams{}
                    .SetItemType(itemType)
                    .SetGlobalLumps(rng.Uniform(0, 5))
                    .SetChunkLocalLumps(rng.Uniform(0, 5))
                    .SetItemLumps(rng.Uniform(0, 10))
                    .SetNumChunks(rng.Uniform(1, 10))
                    .SetNumItems(rng.Uniform(0, 10) * 100)
                );
            }

            CheckItemStorage(std::move(params));
        }
    }

    Y_UNIT_TEST(Cache) {
        CheckItemStorage(TIndexParams{}
            .AddItems(TItemIndexParams{}
                .SetItemType(15)
                .SetGlobalLumps(2)
                .SetChunkLocalLumps(2)
                .SetItemLumps(4)
                .SetNumChunks(4)
                .SetNumItems(1000)
            )
            .AddItems(TItemIndexParams{}
                .SetItemType(0)
                .SetGlobalLumps(0)
                .SetChunkLocalLumps(1)
                .SetItemLumps(1)
                .SetNumChunks(2)
                .SetNumItems(5000)
            )
            .AddItems(TItemIndexParams{}
                .SetItemType(255)
                .SetGlobalLumps(4)
                .SetChunkLocalLumps(4)
                .SetItemLumps(3)
                .SetNumChunks(5)
                .SetNumItems(3000)
            )
            .AddItems(TItemIndexParams{}
                .SetItemType(133)
                .SetGlobalLumps(0)
                .SetChunkLocalLumps(1)
                .SetItemLumps(1)
                .SetNumChunks(2)
                .SetNumItems(5000)
            )
        , ETestParam::CheckEverything);
    }
}
