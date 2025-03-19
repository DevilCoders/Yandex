#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/logger/global/global.h>

#include "hash.h"
#include "hash_tester.h"

#include <util/digest/fnv.h>


void InitLog(int level = 7) {
    if (!GlobalLogInitialized())
        DoInitGlobalLog("console", level, false, false);
}

template<class T>
class TVectorStorage : public IHashDataStorage<T> {
public:
    ui64 Size() const override {
        return Data.size();
    }

    void Resize(ui64 size) override {
        Data.resize(size);
    }

    T& GetData(ui64 index)  override {
        return Data[index];
    }

    const T& GetData(ui64 index) const override {
        return Data[index];
    }

    void Init(ui64 bucketsCount) override {
        Data.resize(bucketsCount, T());
    }

private:
    TVector<T> Data;
};


using THashType = THashLogic<ui64, ui64>;
using TTestStorage = TVectorStorage<TCell<ui64>>;


class THashTest {
public:
    THashTest(TTestStorage* storage, const THashHeader& header)
        : Hash(storage, header) {}

    bool Insert(const TString& key, ui64 data) {
        return Hash.Insert(GetHash(key), data);
    }

    bool Remove(const TString& key) {
        return Hash.Remove(GetHash(key));
    }

    bool Find(const TString& key, ui64& data) const {
        return Hash.Find(GetHash(key), data);
    }

    ui64 Size() const {
        return Hash.Size();
    }

private:
    ui64 GetHash(const TString& key) const {
        return FnvHash<ui64>(key);
    }

private:
    THashType Hash;
};


Y_UNIT_TEST_SUITE(PODHashTests) {
    Y_UNIT_TEST(TestHashSimple) {
        InitLog();
        THashTest dHash(new TTestStorage(), THashHeader(32));
        THashTester<THashTest> hash(dHash);
        hash.Insert("key", 1);
        hash.Remove("key");

        hash.Insert("key", 2);
        hash.Insert("key", 3);
        hash.Insert("key_second", 10);

        for (ui32 i = 0; i < 100; ++i) {
            hash.Insert(ToString(i), i);
        }

        for (ui32 i = 0; i < 50; i+=2) {
            hash.Insert(ToString(i), i + 100);
        }
    }

    Y_UNIT_TEST(TestOneBucket) {
        InitLog();
        THashTest dHash(new TTestStorage(), THashHeader(1));
        THashTester<THashTest> hash(dHash);
        for (ui32 i = 0; i < 100; ++i) {
            hash.Insert(ToString(i), i);
        }

        for (ui32 i = 0; i < 50; i+=2) {
            hash.Insert(ToString(i), i + 100);
        }
    }
}
