#include <kernel/multipart_archive/common/hash_tester.h>
#include <kernel/multipart_archive/common/hash_storage.h>
#include <kernel/multipart_archive/hash/hash.h>
#include <kernel/multipart_archive/config/config.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/logger/global/global.h>

#include <util/ysaveload.h>

#include "test_utils.h"
#include "multipart.h"

using namespace NRTYArchive;

TBlob GetBlob(ui32 data) {
    return TBlob::FromString(ToString(data));
}

TMultipartHash::TPtr CreateHash(const TFsPath& path, ui32 buckets = 32, bool empty = true) {
    if (empty) {
        TArchiveOwner::Remove(path);
        TFsPath metaPath(path.GetPath() + ".meta");
        metaPath.DeleteIfExists();
    }

    TMultipartConfig config;
    return new TMultipartHash(path, config.CreateReadContext(), buckets);
}

Y_UNIT_TEST_SUITE(MultipartHashRepair) {

    ui64 Prepare(const TFsPath& path) {
        TMultipartHash::TPtr mHash = CreateHash(path, 32);

        for (ui32 i = 0; i < 100; ++i) {
            mHash->Insert(ToString(i), GetBlob(i));
        }

        for (ui32 i = 0; i < 50; i += 2) {
            mHash->Remove(ToString(i));
        }

        for (ui32 i = 0; i < 50; i += 2) {
            TBlob data;
            UNIT_ASSERT(mHash->Find(ToString(i + 1), data));
            UNIT_ASSERT_VALUES_EQUAL(GetString<TBlob>(data), ToString(i + 1));
        }
        return mHash->Size();
    }

    Y_UNIT_TEST(TestRepairCorrect) {
        InitLog();

        TFsPath path("test");
        ui64 size = Prepare(path);
        UNIT_ASSERT_VALUES_EQUAL(size, 75);
        UNIT_ASSERT(TMultipartHash::AllRight(path));

        {
            TMultipartHash::Repair(path, TMultipartConfig().CreateReadContext());
            UNIT_ASSERT(TMultipartHash::AllRight(path));
            auto mHash = CreateHash(path, 32, false);
            UNIT_ASSERT_VALUES_EQUAL(mHash->Size(), size);
            UNIT_ASSERT_VALUES_EQUAL(mHash->GetDocsCount(true), size);
            UNIT_ASSERT_VALUES_EQUAL(mHash->GetDocsCount(false), size);

            for (ui32 i = 0; i < 50; i += 2) {
                TBlob data;
                UNIT_ASSERT(mHash->Find(ToString(i + 1), data));
                UNIT_ASSERT_VALUES_EQUAL(GetString<TBlob>(data), ToString(i + 1));
            }
        }
    }

    Y_UNIT_TEST(TestRepairStatic) {
        class TVisitor {
        public:
            bool OnBucket(ui32 bucket) {
                INFO_LOG << "B:" << bucket << Endl;
                return true;
            }

            bool OnItem(ui64 hash, TBlob) {
                DocsCount++;
                INFO_LOG << "I:" << hash << Endl;
                return true;
            }

            ui32 DocsCount = 0;
        };

        InitLog();
        TFsPath dataPath = BinaryPath("kernel/multipart_archive/ut/data/accumulators/accumulators");

        TFsPath path = TFsPath::Cwd() / "data";
        dataPath.CopyTo(path, true);
        path = path / "accumulators";
        UNIT_ASSERT(!TMultipartHash::AllRight(path));

        {
            TMultipartHash::Repair(path, TMultipartConfig().CreateReadContext());
            UNIT_ASSERT(TMultipartHash::AllRight(path));
            auto mHash = CreateHash(path, 32, false);
            UNIT_ASSERT_VALUES_EQUAL(mHash->Size(), 968 + 701 - 649);
            TVisitor visitor;
            mHash->ScanHashLists(visitor);
            UNIT_ASSERT_VALUES_EQUAL(mHash->Size(), visitor.DocsCount);
        }
    }

    Y_UNIT_TEST(TestRepairBrokenBucketLinks) {
        InitLog();

        TFsPath path("test");
        ui64 size = Prepare(path);
        UNIT_ASSERT_VALUES_EQUAL(size, 75);

        TFsPath tmp("tmp");
        {
            TFileMappedArray<THashFAT::TMultipartHashLogic::TCell> hashData;
            hashData.Init(GetFatPath(path).c_str());

            TMappedHash<THashFAT::TMultipartHashLogic::TCell> newHash(tmp);
            newHash.Resize(hashData.Size());

            for (ui32 i = 0; i < hashData.Size(); ++i) {
                THashFAT::TMultipartHashLogic::TCell cell = hashData[i];
                if (i % 4 == 0 && i < /*80*/70) {
                    cell.SetNext(1234);
                }
                newHash.GetData(i) = cell;
            }
        }
        tmp.ForceRenameTo(GetFatPath(path));
        {
            TMultipartHash::TPartMetaSaver(GetPartMetaPath(path, 0))->SetStage(TPartMetaInfo::OPENED);
        }

        UNIT_ASSERT(!TMultipartHash::AllRight(path));

        {
            TMultipartHash::Repair(path, TMultipartConfig().CreateReadContext());
            UNIT_ASSERT(TMultipartHash::AllRight(path));
            auto mHash = CreateHash(path, 32, false);
  //          UNIT_ASSERT_VALUES_EQUAL(mHash->Size(), size);
            UNIT_ASSERT_VALUES_EQUAL(mHash->GetDocsCount(true), size);
            UNIT_ASSERT_VALUES_EQUAL(mHash->GetDocsCount(false), size);

            for (ui32 i = 0; i < 50; i += 2) {
                TBlob data;
                UNIT_ASSERT(mHash->Find(ToString(i + 1), data));
                UNIT_ASSERT_VALUES_EQUAL(GetString<TBlob>(data), ToString(i + 1));
            }
        }

    }

}

Y_UNIT_TEST_SUITE(MultipartHashListsTests) {

    class TVisitor {
    public:
        TVisitor(ui32 bucketsCount)
            : BucketsCount(bucketsCount)
        {}

        bool OnBucket(ui32 bucket) {
            CurBucket = bucket;
            INFO_LOG << "B:" << bucket << Endl;
            return true;
        }

        bool OnItem(ui64 hash, TBlob) {
            VERIFY_WITH_LOG((hash % BucketsCount == CurBucket), "Incorrect elements");
            INFO_LOG << "I:" << hash << Endl;
            return true;
        }

    private:
        ui32 CurBucket = 0;
        ui32 BucketsCount = 0;
    };

    Y_UNIT_TEST(TestIterators) {
        InitLog();

        TFsPath path("lists");
        TFsPath metaPath("lists.meta");
        metaPath.DeleteIfExists();
        TArchiveOwner::Remove(path);

        TMultipartConfig config;
        TMultipartHashLists lists(path, config.CreateReadContext(), 100);
        UNIT_ASSERT_VALUES_EQUAL(lists.GetBucketsCount(), 100);

        for (ui32 i = 0; i < 1000; ++i) {
            lists.PutDocument(TBlob::FromString(::ToString(i)), i);
        }

        TVisitor visitor(100);
        lists.ScanHashLists(visitor);
    }
}

Y_UNIT_TEST_SUITE(MultipartHashTests) {
    Y_UNIT_TEST(TestHashSimple) {
        InitLog();

        TMultipartHash::TPtr mHash = CreateHash("test", 32);
        THashTester<TMultipartHash, TBlob> hash(*mHash.Get());

        hash.Insert("key", GetBlob(1));
        hash.Remove("key");

        hash.Insert("key", GetBlob(2));
        hash.Insert("key", GetBlob(3));
        hash.Insert("key_second", GetBlob(10));

        for (ui32 i = 0; i < 100; ++i) {
            hash.Insert(ToString(i), GetBlob(i));
        }

        for (ui32 i = 0; i < 50; i+=2) {
            hash.Insert(ToString(i), GetBlob(i + 100));
        }
    }

    Y_UNIT_TEST(TestMetaFlush) {
        InitLog();
        TFsPath metaPath("hash.meta");
        metaPath.DeleteIfExists();
        TMultipartConfig config;
        TMultipartHash hash("hash", config.CreateReadContext(), TMaybe<ui32>());
        TFileInput inp(metaPath);
        UNIT_ASSERT(inp.ReadAll().size() != 0);
    }

    Y_UNIT_TEST(TestOneBucket) {
        InitLog();
        TMultipartHash::TPtr mHash = CreateHash("test", 1);
        THashTester<TMultipartHash, TBlob> hash(*mHash.Get());
        for (ui32 i = 0; i < 100; ++i) {
            hash.Insert(ToString(i), GetBlob(i));
        }

        for (ui32 i = 0; i < 50; i+=2) {
            hash.Insert(ToString(i), GetBlob(i + 100));
        }
    }

    Y_UNIT_TEST(TestSaveAndInit) {
        {
            TMultipartHash::TPtr mHash = CreateHash("test", 32);
            THashTester<TMultipartHash, TBlob> hash(*mHash.Get());
            for (ui32 i = 0; i < 100; ++i) {
                hash.Insert(ToString(i), GetBlob(i));
            }
        }

        TMultipartHash::TPtr mHash = CreateHash("test", 32, false);
        UNIT_ASSERT(mHash->Size() == 100);
        for (ui32 i = 0; i < 100; ++i) {
            TBlob data;
            UNIT_ASSERT(mHash->Find(ToString(i), data));
            UNIT_ASSERT_VALUES_EQUAL(GetString<TBlob>(data), ToString(i));
        }
    }
}
