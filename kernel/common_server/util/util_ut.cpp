#include "bit_array_remap.h"
#include "objects_pool.h"

#include <library/cpp/yconf/patcher/unstrict_config.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/generic/buffer.h>
#include <util/generic/ymath.h>
#include <util/random/random.h>
#include <util/system/file.h>
#include <util/system/filemap.h>
#include <util/thread/pool.h>
#include <util/system/types.h>
#include <util/generic/ylimits.h>

struct TBitsChecker {
    TVector<TBitsReader::TFieldDescr> Fields;
    TVector<ui8> DataStorage;
    ui32 DocsCount;
    ui32 FactorsCount;

    ui32 GetStorageWidthBytes() {
        ui32 result = 0;
        for (ui32 i = 0; i < Fields.size(); ++i)
            result += Fields[i].Width;
        return result / 8 + (result % 8 ? 1 : 0);
    }

    TBitsChecker(ui32 docsCount, ui32 factorsCount) {
        DocsCount = docsCount;
        FactorsCount = factorsCount;
        Fields.resize(factorsCount);
    }

    void BuildData(float* data) {
        for (ui32 i = 0; i < Fields.size(); ++i) {
            if (Fields[i].Width == 1) {
                if (rand() % 2)
                    data[i] = 0;
                else
                    data[i] = 1;
            } else if (Fields[i].Type == TBitsReader::ftFloat) {
                data[i] = (float)(rand() % ((1ll << Fields[i].Width) - 1)) / (float)((1ll << Fields[i].Width) - 1);
            } else {
                if (Fields[i].Width == 32 || Fields[i].Width == 64)
                    data[i] = -1 * rand();
                else
                    data[i] = (1ll << (Fields[i].Width - 1));
            }
        }
    }

    void Check() {
        DataStorage.resize(GetStorageWidthBytes());
        TBitsRemapper br(Fields);
        TVector<float> dataStart(Fields.size());
        TVector<float> data(Fields.size());
        BuildData(&dataStart[0]);
        br.Pack(&dataStart[0], &DataStorage[0]);
        TInstant start0 = Now();
        for (ui32 j = 0; j < DocsCount; j++) {
            memcpy(&data[0], &dataStart[0], sizeof(float) * Fields.size());
        }
        TInstant start1 = Now();
        for (ui32 j = 0; j < DocsCount; j++) {
            for (ui32 copy = 0; copy < FactorsCount; ++copy)
                data[copy] = dataStart[copy];
        }
        TInstant start2 = Now();
        for (ui32 j = 0; j < DocsCount; j++) {
            br.UnPack(&data[0], &DataStorage[0]);
        }
        TInstant start3 = Now();
        for (ui32 i = 0; i < FactorsCount; ++i) {
            if (Abs(data[i] - dataStart[i]) > 0.0001)
                UNIT_ASSERT(false);
        }

        Cout << "memcpy: " << start1 - start0 << " vs copy: " << start2 - start1 << " vs bitsunpack: " << start3 - start2 << Endl;
    }

};

Y_UNIT_TEST_SUITE(TMemUtilTest) {

    struct TDestroyChecker {
        static TAtomic Counter;
        TAtomic CTest = 0;

        void Add() {
            AtomicIncrement(CTest);
        }

        void Remove() {
            AtomicDecrement(CTest);
        }

        ~TDestroyChecker() {
            AtomicIncrement(Counter);
        }
    };

    TAtomic TDestroyChecker::Counter = 0;

    class TPoolChecker: public IObjectInQueue {
    private:
        TObjectsPool<TDestroyChecker>& Pool;
    public:

        TPoolChecker(TObjectsPool<TDestroyChecker>& pool)
            : Pool(pool)
        {

        }

        virtual void Process(void* /*threadSpecificResource*/) {

            auto it = Pool.Get();
            if (!it) {
                it = Pool.Add(MakeAtomicShared<TDestroyChecker>());
            }

            it->Add();
            it->Remove();
        }

    };

    struct TIntCont {
        i32 Value = ::Max<i32>();

        TIntCont() = default;

        TIntCont(const i32 value)
            : Value(value)
        {

        }

        bool operator<(const TIntCont& item) const {
            return Value < item.Value;
        }

        bool operator==(const TIntCont& item) const {
            return Value == item.Value;
        }
    };

    Y_UNIT_TEST(TestObjectsPool) {
        DoInitGlobalLog("console", TLOG_DEBUG, false, false);
        TObjectsPool<TDestroyChecker> pool;
        {
            auto it = pool.Add(MakeAtomicShared<TDestroyChecker>());
            auto it3 = it;
        }
        {
            auto it1 = pool.Get();
            UNIT_ASSERT(!!it1);
            auto it2 = pool.Get();
            UNIT_ASSERT(!it2);
        }
        {
            auto it1 = pool.Get();
            UNIT_ASSERT(!!it1);
            auto it2 = pool.Get();
            UNIT_ASSERT(!it2);
        }

        TThreadPool queue;
        queue.Start(64);

        for (ui32 i = 0; i < 1000; ++i) {
            queue.SafeAddAndOwn(MakeHolder<TPoolChecker>(pool));
        }
        queue.Stop();

        TSet<ui64> pointers;
        TVector<TObjectsPool<TDestroyChecker>::TObjectGuard> storage;
        for (auto it = pool.Get(); !!it; it = pool.Get()) {
            storage.push_back(it);
            UNIT_ASSERT(pointers.insert((ui64)(*it).Get()).second);
        }

        UNIT_ASSERT_EQUAL(AtomicGet(TDestroyChecker::Counter), 0);
    }

    Y_UNIT_TEST(HashInvariant) {
        THashMap<TString, ui32> testHash;
        TVector<ui32*> pointers;
        for (ui32 i = 0; i < 1000000; ++i) {
            testHash[ToString(i)] = i;
            pointers.push_back(&testHash.find(ToString(i))->second);
        }

        for (ui32 i = 0; i < 1000000; ++i) {
            UNIT_ASSERT_EQUAL(pointers[i], &testHash.find(ToString(i))->second);
        }
    }

}

Y_UNIT_TEST_SUITE(TRtyUtilTest) {
    Y_UNIT_TEST(TestBitsReaderSimplePerformance) {
        TBitsChecker checker(100000, 100);

        for (ui32 i = 0; i < checker.Fields.size(); ++i) {
            checker.Fields[i].Width = 32;
            checker.Fields[i].IndexRemap = i;
            checker.Fields[i].Type = (TBitsReader::TFieldType)(rand() % 2);
        }

        checker.Check();
    }

    Y_UNIT_TEST(TestBitsReaderBitsPerformance1I) {
        TBitsChecker checker(100000, 320);

        for (ui32 i = 0; i < checker.Fields.size(); ++i) {
            checker.Fields[i].Width = 1;
            checker.Fields[i].IndexRemap = i;
            checker.Fields[i].Type = TBitsReader::ftInt;
        }

        checker.Check();
    }

    Y_UNIT_TEST(TestBitsReaderBytesPerformance8F) {
        TBitsChecker checker(100000, 320);

        for (ui32 i = 0; i < checker.Fields.size(); ++i) {
            checker.Fields[i].Width = 8;
            checker.Fields[i].IndexRemap = i;
            checker.Fields[i].Type = TBitsReader::ftFloat;
        }

        checker.Check();
    }

    Y_UNIT_TEST(TestBitsReaderBytesPerformance8I) {
        TBitsChecker checker(100000, 320);

        for (ui32 i = 0; i < checker.Fields.size(); ++i) {
            checker.Fields[i].Width = 8;
            checker.Fields[i].IndexRemap = i;
            checker.Fields[i].Type = TBitsReader::ftInt;
        }

        checker.Check();
    }

    Y_UNIT_TEST(TestBitsReaderBytesPerformance16I) {
        TBitsChecker checker(100000, 320);

        for (ui32 i = 0; i < checker.Fields.size(); ++i) {
            checker.Fields[i].Width = 16;
            checker.Fields[i].IndexRemap = i;
            checker.Fields[i].Type = TBitsReader::ftInt;
        }

        checker.Check();
    }

    Y_UNIT_TEST(TestUi16) {
        TVector<TBitsReader::TFieldDescr> fields;
        fields.resize(1);
        fields[0].Width = 16;
        fields[0].IndexRemap = 0;
        fields[0].Type = TBitsReader::ftInt;

        ui8 storage[16];
        TBitsRemapper br(fields);

        {
            float data = 0;
            data = 13;
            br.Pack(&data, storage);
        }
        {
            float data;
            br.UnPack(&data, storage);
            const ui32 res = data;
            UNIT_ASSERT_EQUAL(res, 13);
        }
    }

    Y_UNIT_TEST(TestBitsReader) {

        TBitsChecker bc(1, 11);

        bc.Fields[0].Width = 1;
        bc.Fields[1].Width = 2;
        bc.Fields[2].Width = 3;
        bc.Fields[3].Width = 2;
        bc.Fields[4].Width = 32;
        bc.Fields[5].Width = 32;
        bc.Fields[6].Width = 16;
        bc.Fields[7].Width = 8;
        bc.Fields[8].Width = 10;
        bc.Fields[9].Width = 6;
        bc.Fields[10].Width = 2;

        bc.Check();
    }

    Y_UNIT_TEST(TestUnstrictConfig) {
        const TString SampleConfig =
            "<Proxy>\n"
                "<HttpOptions>\n"
                    "Host : adfdf\n"
                    "Port : 10020\n"
                "</HttpOptions>\n"
            "</Proxy>";

        TUnstrictConfig config;
        UNIT_ASSERT(config.ParseMemory(SampleConfig));
        UNIT_ASSERT(config.GetValue("Proxy.HttpOptions.Host") == "adfdf");
        config.PatchEntry("Proxy.HttpOptions.Host", "localhost", "");
        config.PatchEntry("HttpOptions.Port", "12345", "Proxy.");
        TString patched = config.ToString();

        TUnstrictConfig patchedConfig;
        UNIT_ASSERT(patchedConfig.ParseMemory(patched));
        UNIT_ASSERT(patchedConfig.GetValue("Proxy.HttpOptions.Host") == "localhost");
        UNIT_ASSERT(patchedConfig.GetValue("Proxy.HttpOptions.Port") == "12345");
    }
};
