#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/codecs/float_huffman.h>

#include <util/generic/utility.h>

#include "factor_storage.h"

class TTestFactorStorage : public TTestBase {
    UNIT_TEST_SUITE(TTestFactorStorage);
        UNIT_TEST(TestFactorView);
        UNIT_TEST(TestFactorStorageCtors);
        UNIT_TEST(TestEmptyStorage);
        UNIT_TEST(TestCopyStorage);
        UNIT_TEST(TestCrossCopyStorage);
        UNIT_TEST(TestMoveStorage);
        UNIT_TEST(TestStorageVector);
        UNIT_TEST(TestFactorInfo);
        UNIT_TEST(TestSwapStorage);
        UNIT_TEST(TestResizeStorage);
        UNIT_TEST(TestFactorBuf);
        UNIT_TEST(TestSerialization);
        UNIT_TEST(TestSerializationMerge);
        UNIT_TEST(TestSerializationUnknownSlice);
        UNIT_TEST(TestBordersSerialization);
        UNIT_TEST(TestSerializationZeroFactors);
        UNIT_TEST(TestStorageSizeBound);
    UNIT_TEST_SUITE_END();

private:
    static void CheckViewsEqual(const TFactorView& viewX, const TFactorView& viewY) {
        UNIT_ASSERT_EQUAL(viewX.Size(), viewY.Size());
        for (size_t i = 0; i != viewX.Size(); ++i) {
            Cdbg << i << "\t" << viewX[i] << "\t" << viewY[i] << Endl;
            UNIT_ASSERT_EQUAL(viewX[i], viewY[i]);
        }
    }

    static void CopyStorage(const TFactorStorage& from, TFactorStorage& to) {
        for (auto iter = from.GetDomain().Begin(); iter.Valid(); iter.Next()) {
            if (to.GetDomain().HasIndex(iter)) {
                Cdbg << "COPY ONE FACTOR " << to.GetDomain().GetIndex(iter) << " <-- " << from.GetDomain().GetIndex(iter) << Endl;
                to[to.GetDomain().GetIndex(iter)] = from[from.GetDomain().GetIndex(iter)];
            }
        }
    }

    static void CopyStorageLeaves(const TFactorStorage& from, TFactorStorage& to) {
        for (auto iter = from.GetDomain().Begin(); iter.Valid(); iter.NextLeaf()) {
            size_t size = Min(iter.GetLeafSize(), to.GetDomain()[iter.GetLeaf()].Size());
            UNIT_ASSERT(to.factors + to.GetDomain().GetIndex(iter) + size <= to.factors + to.Size());
            UNIT_ASSERT(from.factors + iter.GetIndex() + size <= from.factors + from.Size());
            Cdbg << "COPY ONE LEAF " << iter.GetLeaf() << " " << size << Endl;
            memcpy(to.Ptr(iter), from.Ptr(iter), size * sizeof(float));
        }
    }

    static void CheckTRhrFactor(const TOneFactorInfo& info) {
        UNIT_ASSERT(info);
        UNIT_ASSERT_EQUAL(info.GetIndex(), 9);
        UNIT_ASSERT_EQUAL(TString(info.GetFactorName()), "TRhr");
    }

    static void CheckPtrInPool(const void* ptr, const TMemoryPool& pool) {
        bool inPool = false;
        auto op = [ptr, &inPool](const char* data, size_t size){
            inPool = inPool || (ptr >= data && ptr < data + size);
        };
        pool.Traverse(op);
        UNIT_ASSERT(inPool);
    }

    static void CheckPtrNotInPool(const void* ptr, const TMemoryPool& pool) {
        auto op = [ptr](const char* data, size_t size){
            UNIT_ASSERT(!(ptr >= data && ptr < data + size));
        };
        pool.Traverse(op);
    }

    struct TCongrandGen {
        unsigned int next() {
            state = state * 1664525u + 1013904223u;
            return state;
        }
        unsigned int state = 0;
    };

public:
    void TestFactorView() {
        TFactorBorders borders;
        borders[EFactorSlice::ALL] = TSliceOffsets(0, 1000);
        borders[EFactorSlice::WEB] = TSliceOffsets(0, 900);
        borders[EFactorSlice::WEB_PRODUCTION] = TSliceOffsets(0, 900);
        borders[EFactorSlice::LINGBOOST_BETA] = TSliceOffsets(900, 1000);

        TFactorStorage storage(borders[EFactorSlice::WEB].Size());
        storage.SetBorders(borders);

        UNIT_ASSERT_EQUAL(storage.Size(), 1000);

        for (size_t i = 0; i != storage.Size(); ++i) {
            storage[i] = i;
        }

        TFactorView webView = storage[EFactorSlice::WEB];
        TFactorView prodView = storage[EFactorSlice::WEB_PRODUCTION];
        TFactorView expView = storage[EFactorSlice::LINGBOOST_BETA];

        UNIT_ASSERT_EQUAL(webView.Size(), 900);
        UNIT_ASSERT_EQUAL(prodView.Size(), 900);
        UNIT_ASSERT_EQUAL(expView.Size(), 100);

        UNIT_ASSERT_EQUAL(webView[0], 0.f);
        UNIT_ASSERT_EQUAL(prodView[0], 0.f);
        UNIT_ASSERT_EQUAL(expView[0], 900.f);

        UNIT_ASSERT(prodView.GetDomain());

        UNIT_ASSERT(prodView.Has({EFactorSlice::WEB_PRODUCTION, 0}));
        UNIT_ASSERT(!prodView.Has({EFactorSlice::LINGBOOST_BETA, 0}));
        UNIT_ASSERT(prodView.Has({EFactorSlice::ALL, 0}));
        UNIT_ASSERT_VALUES_EQUAL(prodView[TFullFactorIndex(EFactorSlice::WEB_PRODUCTION, 0)], 0.f);
        UNIT_ASSERT_VALUES_EQUAL(prodView[TFullFactorIndex(EFactorSlice::ALL, 0)], 0.f);
        UNIT_ASSERT(!prodView.Has({EFactorSlice::WEB_PRODUCTION, 900}));
        UNIT_ASSERT(prodView.Has({EFactorSlice::WEB_PRODUCTION, 899}));
        UNIT_ASSERT_VALUES_EQUAL(prodView[TFullFactorIndex(EFactorSlice::WEB_PRODUCTION, 899)], 899.f);
        UNIT_ASSERT(expView.Has({EFactorSlice::LINGBOOST_BETA, 0}));
        UNIT_ASSERT(expView.Has({EFactorSlice::ALL, 900}));
        UNIT_ASSERT_VALUES_EQUAL(expView[TFullFactorIndex(EFactorSlice::LINGBOOST_BETA, 0)], 900.f);
        UNIT_ASSERT_VALUES_EQUAL(expView[TFullFactorIndex(EFactorSlice::ALL, 900)], 900.f);

        CheckViewsEqual(prodView, webView[EFactorSlice::WEB_PRODUCTION]);
        CheckViewsEqual(webView, webView[EFactorSlice::WEB]);

        auto mView = storage.CreateMultiViewFor(EFactorSlice::WEB_PRODUCTION, EFactorSlice::LINGBOOST_BETA, EFactorSlice::WEB_META);
        UNIT_ASSERT_EQUAL(mView.NumSlices(), 2);
        CheckViewsEqual(mView[EFactorSlice::WEB_PRODUCTION], prodView);
        CheckViewsEqual(mView[EFactorSlice::LINGBOOST_BETA], expView);
        UNIT_ASSERT_EQUAL(mView[EFactorSlice::WEB_META].Size(), 0);

        TVector<EFactorSlice> correctOrder = {EFactorSlice::WEB_PRODUCTION, EFactorSlice::LINGBOOST_BETA};
        auto correctOrderIter = correctOrder.begin();
        for (auto view : mView) {
            UNIT_ASSERT(view.Size() > 0);
            UNIT_ASSERT_EQUAL(view.GetSlice(), *correctOrderIter);
            ++correctOrderIter;
        }

        TFactorViewForSlice<EFactorSlice::WEB_PRODUCTION> viewForProd = storage.CreateViewFor(EFactorSlice::WEB_PRODUCTION);
        UNIT_ASSERT_EQUAL(viewForProd.Size(), prodView.Size());
        viewForProd = storage.CreateViewFor(EFactorSlice::WEB_PRODUCTION);
        UNIT_ASSERT_EQUAL(viewForProd.Size(), prodView.Size());
        viewForProd = storage.CreateViewFor(EFactorSlice::ALL);
        UNIT_ASSERT_EQUAL(viewForProd.Size(), prodView.Size());

        TFactorViewForRole<ESliceRole::MAIN> viewForMain = storage.CreateViewFor(EFactorSlice::WEB_PRODUCTION);
        UNIT_ASSERT_EQUAL(viewForMain.Size(), prodView.Size());
        viewForMain = storage.CreateViewFor(EFactorSlice::WEB_PRODUCTION);
        UNIT_ASSERT_EQUAL(viewForMain.Size(), prodView.Size());
        viewForMain = storage.CreateViewFor(EFactorSlice::ALL);
        UNIT_ASSERT_EQUAL(viewForMain.Size(), prodView.Size());
    }

    void TestFactorStorageCtors() {
        TMemoryPool pool(1024);
        TSlicesMetaInfo metaInfo = NFactorSlices::TGlobalSlicesMetaInfo::Instance();
        TFactorBorders borders;

        TFactorDomain domain;
        TFactorDomain domainWeb;

        TFactorStorage storage;
        TFactorStorage storageInPool;

        Y_UNUSED(domain);
        Y_UNUSED(domainWeb);
        Y_UNUSED(storage);
        Y_UNUSED(storageInPool);

        {
            domain = TFactorDomain(metaInfo);
            domainWeb = TFactorDomain(metaInfo, EFactorSlice::WEB);
        }

        {
            domain = TFactorDomain();
            domainWeb = TFactorDomain(EFactorSlice::WEB);
        }

        {
            storage = TFactorStorage(10);
            storageInPool = TFactorStorage(10, &pool);
        }

        {
            storage = TFactorStorage(domain);
            storageInPool = TFactorStorage(domain, &pool);
        }

        {
            storage = TFactorStorage(borders);
            storageInPool = TFactorStorage(borders, &pool);
        }
    }

    void TestEmptyStorage() {
        TFactorStorage storageX;
        TFactorStorage storageY = storageX;
        TFactorStorage storageZ(std::move(storageX));
        TFactorStorage storageW(std::move(storageX));
        storageX = storageY;
        ::DoSwap(storageZ, storageW);
        UNIT_ASSERT_EQUAL(storageX.Size(), 0);
        UNIT_ASSERT_EQUAL(storageY.Size(), 0);
        UNIT_ASSERT_EQUAL(storageZ.Size(), 0);
        UNIT_ASSERT_EQUAL(storageW.Size(), 0);

        UNIT_ASSERT_EQUAL(storageY.GetDomain(), storageX.GetDomain());
        UNIT_ASSERT_EQUAL(storageZ.GetDomain(), storageX.GetDomain());
        UNIT_ASSERT_EQUAL(storageW.GetDomain(), storageX.GetDomain());
    }

    void TestCopyStorage() {
        TFactorBorders bordersX;
        bordersX[EFactorSlice::FRESH] = TSliceOffsets(0, 10);
        bordersX[EFactorSlice::WEB_PRODUCTION] = TSliceOffsets(10, 20);
        bordersX[EFactorSlice::WEB_META] = TSliceOffsets(20, 30);
        UNIT_ASSERT(NFactorSlices::NDetail::ReConstruct(bordersX));

        TFactorBorders bordersY;
        bordersY[EFactorSlice::FRESH] = TSliceOffsets(0, 5);
        bordersY[EFactorSlice::WEB_PRODUCTION] = TSliceOffsets(5, 10);
        bordersY[EFactorSlice::WEB_META] = TSliceOffsets(10, 15);
        UNIT_ASSERT(NFactorSlices::NDetail::ReConstruct(bordersY));

        TFactorStorage storageX(bordersX);
        UNIT_ASSERT(storageX.GetDomain().IsNormal());

        TFactorStorage storageY(bordersY);
        UNIT_ASSERT(storageY.GetDomain().IsNormal());

        Cdbg << "INIT STORAGE" << Endl;
        for (size_t i = 0; i != storageY.Size(); ++i) {
            storageY[i] = (float) i;
            Cdbg << storageY[i] << " ";
        }
        Cdbg << Endl;

        Cdbg << "INIT STORAGE COPY" << Endl;
        CopyStorage(storageY, storageX);

        {
            Cdbg << "COPY STORAGE SIMPLE" << Endl;
            storageY.Clear();
            CopyStorage(storageX, storageY);

            for (size_t i = 0; i != storageY.Size(); ++i) {
                Cdbg << storageY[i] << " ";
                UNIT_ASSERT_EQUAL(storageY[i], (float) i);
            }
            Cdbg << Endl;
        }

        {
            Cdbg << "COPY STORAGE LEAVES" << Endl;
            storageY.Clear();
            CopyStorageLeaves(storageX, storageY);

            for (size_t i = 0; i != storageY.Size(); ++i) {
                Cdbg << storageY[i] << " ";
                UNIT_ASSERT_EQUAL(storageY[i], (float) i);
            }
            Cdbg << Endl;
        }

        {
            TFactorStorage storageX(100);
            TFactorStorage storageY(storageX);

            storageX[0] = 1.0;
            storageX[99] = 9.0;
            float* ptrX = storageX.Ptr(0);
            storageX = static_cast<TFactorStorage&>(storageX);
            UNIT_ASSERT_EQUAL(storageX.Size(), 100);
            UNIT_ASSERT_EQUAL(storageX[0], 1.0);
            UNIT_ASSERT_EQUAL(storageX[99], 9.0);
            UNIT_ASSERT_EQUAL(storageX.Ptr(0), ptrX);

            UNIT_ASSERT(!!storageY.Ptr(0));
            UNIT_ASSERT_EQUAL(storageX.Size(), storageY.Size());
            UNIT_ASSERT_UNEQUAL(storageX.Ptr(0), storageY.Ptr(0));

            storageY = TFactorStorage();
            UNIT_ASSERT_EQUAL(storageY.GetDomain().Size(), 0);

            storageY = storageX;
            UNIT_ASSERT(!!storageY.Ptr(0));
            UNIT_ASSERT_EQUAL(storageX.GetDomain().Size(), storageY.GetDomain().Size());
            UNIT_ASSERT_UNEQUAL(storageX.Ptr(0), storageY.Ptr(0));
        }
    }

    void TestCrossCopyStorage() {
        THolder<TFactorStorage> storageY;

        static const size_t count = 100;

        // Ensure that copy-ctor doesn't
        // allocate in pool
        {
            TMemoryPool pool(10 * count * sizeof(float));

            TFactorStorage storageX(count, &pool);
            for (size_t i : xrange(count)) {
                storageX[i] = i;
            }
            storageY.Reset(new TFactorStorage(storageX));
            CheckPtrNotInPool(storageY->factors, pool);
        }

        for (size_t i : xrange(count)) {
            UNIT_ASSERT_EQUAL((*storageY)[i], float(i));
        }

        {
            TMemoryPool pool(10 * count * sizeof(float));
            UNIT_ASSERT_EQUAL(pool.MemoryAllocated(), 0);
            TFactorStorage* storageZ = CreateFactorStorageInPool(&pool, *storageY);
            UNIT_ASSERT(pool.MemoryAllocated() > count * sizeof(float));
            Y_UNUSED(storageZ);
        }
    }

    void TestMoveStorage() {
        static const size_t count = 100;
        TMemoryPool pool(10 * count * sizeof(float));
        TMemoryPool pool2(10 * count * sizeof(float));

        {
            Cdbg << "TestMoveStorage: heap --> heap" << Endl;
            TFactorStorage storageX(100);
            float* factorsX = storageX.factors;
            TFactorStorage storageZ(std::move(storageX));
            CheckPtrNotInPool(storageZ.factors, pool);
            UNIT_ASSERT_EQUAL(storageZ.factors, factorsX);
            // Check that storageX is valid
            TFactorStorage storageW = storageX;
            UNIT_ASSERT_EQUAL(storageX.Size(), storageW.Size());
            Y_UNUSED(storageW);
            const TFactorDomain& domainX = storageX.GetDomain();
            Y_UNUSED(domainX);
            TFactorView viewX = storageX[EFactorSlice::WEB_PRODUCTION];
            Y_UNUSED(viewX);
        }

        {
            Cdbg << "TestMoveStorage: heap --> pool" << Endl;
            TFactorStorage storageX(100);
            TFactorStorage storageZ(std::move(storageX), &pool);
            CheckPtrInPool(storageZ.factors, pool);
        }

        {
            Cdbg << "TestMoveStorage: pool --> heap" << Endl;
            TFactorStorage storageY(100, &pool);
            TFactorStorage storageZ(std::move(storageY));
            CheckPtrNotInPool(storageZ.factors, pool);
        }

        {
            Cdbg << "TestMoveStorage: pool --> pool" << Endl;
            TFactorStorage storageY(100, &pool);
            float* factorsY = storageY.factors;
            TFactorStorage storageZ(std::move(storageY), &pool);
            CheckPtrInPool(storageZ.factors, pool);
            UNIT_ASSERT_EQUAL(storageZ.factors, factorsY);
            TFactorStorage* storageW = CreateFactorStorageInPool<TFactorStorage>(&pool, std::move(storageZ));
            Y_ASSERT(storageW);
            CheckPtrInPool(storageW->factors, pool);
            UNIT_ASSERT_EQUAL(storageW->factors, factorsY);
        }

        {
            Cdbg << "TestMoveStorage: pool --> pool2" << Endl;
            TFactorStorage storageY(100, &pool);
            TFactorStorage storageZ(std::move(storageY), &pool2);
            CheckPtrInPool(storageZ.factors, pool2);
        }
    }

    void TestStorageVector() {
        {
            TVector<TFactorStorage> storages;
            storages.resize(10, TFactorStorage(10));
            for (size_t i = 0; i != 100; ++i) {
                storages[i / 10][i % 10] = (float) i;
            }
            for (size_t i = 0; i != 100; ++i) {
                UNIT_ASSERT_EQUAL(storages[i / 10][i % 10], (float) i);
            }
        }

        {
            TVector<TGeneralFactorStorage<100>> storages;
            for (size_t i = 0; i != 100; ++i) {
                storages.push_back(TGeneralFactorStorage<100>());
            }
        }
    }

    void TestFactorInfo() {
        TFactorBorders borders;
        borders[EFactorSlice::FRESH] = TSliceOffsets(0, 10);
        borders[EFactorSlice::WEB_PRODUCTION] = TSliceOffsets(10, 20);
        borders[EFactorSlice::WEB_META] = TSliceOffsets(20, 30);
        UNIT_ASSERT(NFactorSlices::NDetail::ReConstruct(borders));

        TFactorStorage storage(borders);
        TFactorView viewWeb = storage[EFactorSlice::WEB];
        TFactorView viewAll = storage[EFactorSlice::ALL];

        {
            auto info = viewWeb.GetFactorInfo(9);
            CheckTRhrFactor(info);
        }

        {
            auto info = viewWeb[EFactorSlice::WEB_PRODUCTION].GetFactorInfo(9);
            CheckTRhrFactor(info);
        }

        {
            auto info = viewAll.GetFactorInfo(1);
            UNIT_ASSERT(!info); // not linked with search/fresh
            info = viewAll.GetFactorInfo(19);
            CheckTRhrFactor(info);
        }
    }

    void TestSwapStorage() {
        {
            TGeneralResizeableFactorStorage storageX(100);
            float* ptrX = storageX.factors;
            TGeneralResizeableFactorStorage storageY(200);
            float* ptrY = storageY.factors;
            DoSwap(storageX, storageY);
            UNIT_ASSERT_EQUAL(storageX.Size(), 200);
            UNIT_ASSERT_EQUAL(storageY.Size(), 100);
            UNIT_ASSERT_EQUAL(storageX.factors, ptrY);
            UNIT_ASSERT_EQUAL(storageY.factors, ptrX);
        }
        {
            TFactorStorage storageX(100);
            float* ptrX = storageX.Ptr(0);
            TFactorStorage storageY(200);
            float* ptrY = storageY.Ptr(0);
            DoSwap(storageX, storageY);
            UNIT_ASSERT_EQUAL(storageX.Size(), 200);
            UNIT_ASSERT_EQUAL(storageY.Size(), 100);
            UNIT_ASSERT_EQUAL(storageX.factors, ptrY);
            UNIT_ASSERT_EQUAL(storageY.factors, ptrX);
        }
    }

    void TestResizeStorage() {
        TMemoryPool pool{1UL << 10};
        TGeneralResizeableFactorStorage storage(0, &pool);

        for (size_t i : xrange(100)) {
            storage.Resize(i + 1);
            UNIT_ASSERT_EQUAL(storage.Size(), i + 1);
            storage[i] = float(i);
            for (auto j : xrange(storage.Size())) {
                UNIT_ASSERT_EQUAL(storage[j], float(j));
            }
        }

        Cdbg << "MEM_ALLOC_1 = " << pool.MemoryAllocated() << Endl;
        size_t memAlloc = pool.MemoryAllocated();

        for (size_t i : xrange(100)) {
            storage.Resize(100 - i - 1);
            UNIT_ASSERT_EQUAL(storage.Size(), 100 - i - 1);
            for (auto j : xrange(storage.Size())) {
                UNIT_ASSERT_EQUAL(storage[j], float(j));
            }
        }

        Cdbg << "MEM_ALLOC_2 = " << pool.MemoryAllocated() << Endl;
        UNIT_ASSERT_EQUAL(memAlloc, pool.MemoryAllocated());
    }

    void TestFactorBuf() {
        {
            TFactorBuf buf(0, nullptr);
            float data[100];
            buf = TFactorBuf(100, data);
            UNIT_ASSERT_EQUAL(buf.Size(), 100);
            UNIT_ASSERT_EQUAL(&buf[0], data);
        }
    }

    void TestSerialization() {
        {
            TFactorBorders borders;
            borders[EFactorSlice::WEB] = TSliceOffsets(0, 900);
            borders[EFactorSlice::WEB_PRODUCTION] = TSliceOffsets(0, 900);
            borders[EFactorSlice::LINGBOOST_BETA] = TSliceOffsets(900, 1000);

            NFactorSlices::TSlicesMetaInfo hostInfo;
            UNIT_ASSERT(NFactorSlices::NDetail::ReConstructMetaInfo(borders, hostInfo));
            TFactorDomain hostDomain(hostInfo);
            TFactorStorage storage(hostDomain);

            UNIT_ASSERT_EQUAL(storage.Size(), 1000);
            TCongrandGen gen;
            for (size_t i = 0; i != storage.Size(); ++i) {
                storage[i] = (gen.next() % 1000) / 1000.0;
            }
            TStringStream ss;
            NFSSaveLoad::Serialize(storage, &ss);
            THolder<TFactorStorage> loaded = NFSSaveLoad::Deserialize(&ss, hostInfo);
            UNIT_ASSERT_EQUAL(storage.Size(), loaded->Size());
            for (size_t i = 0; i < storage.Size(); ++i) {
                UNIT_ASSERT_EQUAL(storage[i], (*loaded)[i]);
            }
            UNIT_ASSERT_EQUAL(storage.GetDomain(), loaded->GetDomain());
        }
    }
    void TestSerializationMerge() {
        {
            TFactorBorders borders;
            borders[EFactorSlice::WEB] = TSliceOffsets(0, 1000);
            borders[EFactorSlice::WEB_PRODUCTION] = TSliceOffsets(0, 900);
            borders[EFactorSlice::WEB_META] = TSliceOffsets(900, 1000);

            NFactorSlices::TSlicesMetaInfo hostInfo;
            UNIT_ASSERT(NFactorSlices::NDetail::ReConstructMetaInfo(borders, hostInfo));
            TFactorDomain hostDomain(hostInfo);

            NFactorSlices::TSlicesMetaInfo guestInfo;
            borders[EFactorSlice::RAPID_CLICKS] = TSliceOffsets(1000, 1020);
            UNIT_ASSERT(NFactorSlices::NDetail::ReConstructMetaInfo(borders, guestInfo));
            TFactorDomain guestDomain(guestInfo);

            TFactorStorage storage(guestDomain);

            UNIT_ASSERT_EQUAL(storage.Size(), 1020);
            TCongrandGen gen;
            for (size_t i = 0; i != storage.Size(); ++i) {
                storage[i] = (gen.next() % 1000) / 1000.0;
            }
            TStringStream ss;
            NFSSaveLoad::Serialize(storage, &ss);
            THolder<TFactorStorage> loaded = NFSSaveLoad::Deserialize(&ss, hostInfo);
            UNIT_ASSERT(loaded.Get());
            UNIT_ASSERT_EQUAL(storage.Size(), loaded->Size());
            for (size_t i = 0; i < storage.Size(); ++i) {
                UNIT_ASSERT_EQUAL(storage[i], (*loaded)[i]);
            }
            UNIT_ASSERT_EQUAL(storage.GetDomain(), loaded->GetDomain());
        }
    }

    void TestBordersSerialization() {
        TFactorBorders borders;
        borders[EFactorSlice::WEB] = TSliceOffsets(0, 900);
        borders[EFactorSlice::WEB_PRODUCTION] = TSliceOffsets(0, 900);
        borders[EFactorSlice::LINGBOOST_BETA] = TSliceOffsets(900, 1000);
        TString bstr = SerializeFactorBorders(borders);
        UNIT_ASSERT_EQUAL(bstr, "web[0;900) web_production[0;900) lingboost_beta[900;1000)");
    }

    void TestSerializationUnknownSlice() {
        {
            TFactorBorders borders;
            borders[EFactorSlice::WEB] = TSliceOffsets(0, 900);
            borders[EFactorSlice::WEB_PRODUCTION] = TSliceOffsets(0, 900);
            borders[EFactorSlice::LINGBOOST_BETA] = TSliceOffsets(900, 1000);

            NFactorSlices::TSlicesMetaInfo senderInfo;
            UNIT_ASSERT(NFactorSlices::NDetail::ReConstructMetaInfo(borders, senderInfo));
            TFactorDomain senderDomain(senderInfo);
            TFactorStorage storage(senderDomain);

            NFactorSlices::TSlicesMetaInfo hostInfo; // empty

            UNIT_ASSERT_EQUAL(storage.Size(), 1000);
            TCongrandGen gen;
            for (size_t i = 0; i != storage.Size(); ++i) {
                storage[i] = (gen.next() % 1000) / 1000.0;
            }

            const TFactorBorders& sborders = storage.GetBorders();
            TString fb = SerializeFactorBorders(sborders);
            TString huffStr = NCodecs::NFloatHuff::Encode({storage.Ptr(0), storage.Size()});

            // Unknown hier
            {
                THolder<TFactorStorage> result;
                UNIT_ASSERT_NO_EXCEPTION(result = NFSSaveLoad::Deserialize(fb + " lalala[0;1000)", huffStr, hostInfo));
                UNIT_ASSERT_EQUAL(result->Size(), 1000);
                for (size_t i = 0; i != result->Size(); ++i) {
                    UNIT_ASSERT_EQUAL((*result)[i], storage[i]);
                }
            }

            // Missing hier
            {
                THolder<TFactorStorage> result;
                UNIT_ASSERT_NO_EXCEPTION(result = NFSSaveLoad::Deserialize("web_production[0;900) lingboost_beta[900;1000)", huffStr, hostInfo));
                UNIT_ASSERT_EQUAL(result->Size(), 1000);
                for (size_t i = 0; i != result->Size(); ++i) {
                    UNIT_ASSERT_EQUAL((*result)[i], storage[i]);
                }
            }

            // Unknown leaf
            {
                THolder<TFactorStorage> result;
                UNIT_ASSERT_NO_EXCEPTION(result = NFSSaveLoad::Deserialize("web[0;1000) web_production[0;900) lalala[900;1000)", huffStr, hostInfo));
                UNIT_ASSERT_EQUAL(result->Size(), 900);
                for (size_t i = 0; i != result->Size(); ++i) {
                    UNIT_ASSERT_EQUAL((*result)[i], storage[i]);
                }
            }

            // Unknown leaf + missing hier + unknown hier
            {
                THolder<TFactorStorage> result;
                UNIT_ASSERT_NO_EXCEPTION(result = NFSSaveLoad::Deserialize("dadada[900;1000) web_production[0;800) lalala[800;900) lingboost_beta[900;1000)", huffStr, hostInfo));
                UNIT_ASSERT_EQUAL(result->Size(), 900);
                for (size_t i = 0; i != result->Size(); ++i) {
                    UNIT_ASSERT_EQUAL((*result)[i], i < 800 ? storage[i] : storage[i + 100]);
                }
            }

            // Bad hier
            {
                THolder<TFactorStorage> result;
                UNIT_ASSERT_NO_EXCEPTION(result = NFSSaveLoad::Deserialize("all[0;1000) formula[100;900) fresh[0;100) web[100;900) web_production[100;900) mango[900;1000)", huffStr, hostInfo));
                UNIT_ASSERT_EQUAL(result->Size(), 1000);
                UNIT_ASSERT_EQUAL(result->CreateViewFor(EFactorSlice::FORMULA).Size(), 900);
            }
        }
    }

    void TestSerializationZeroFactors() {
        {
            TFactorBorders hostBorders;
            hostBorders[EFactorSlice::FRESH] = TSliceOffsets(0, 100);
            hostBorders[EFactorSlice::WEB] = TSliceOffsets(100, 1000);
            hostBorders[EFactorSlice::WEB_PRODUCTION] = TSliceOffsets(100, 1000);
            hostBorders[EFactorSlice::LINGBOOST_BETA] = TSliceOffsets(1000, 1100);
            NFactorSlices::TSlicesMetaInfo hostInfo;
            UNIT_ASSERT(NFactorSlices::NDetail::ReConstructMetaInfo(hostBorders, hostInfo));
            TFactorDomain hostDomain(hostInfo);

            TFactorBorders guestBorders;
            guestBorders[EFactorSlice::WEB] = TSliceOffsets(0, 900);
            guestBorders[EFactorSlice::WEB_PRODUCTION] = TSliceOffsets(0, 900);
            guestBorders[EFactorSlice::LINGBOOST_BETA] = TSliceOffsets(900, 1000);
            NFactorSlices::TSlicesMetaInfo guestInfo;
            UNIT_ASSERT(NFactorSlices::NDetail::ReConstructMetaInfo(guestBorders, guestInfo));
            TFactorDomain guestDomain(guestInfo);

            TFactorStorage storage(guestDomain);

            UNIT_ASSERT_EQUAL(storage.Size(), 1000);
            TCongrandGen gen;
            for (size_t i = 0; i != storage.Size(); ++i) {
                storage[i] = (gen.next() % 1000) / 1000.0;
            }
            TStringStream ss;
            NFSSaveLoad::Serialize(storage, &ss);
            THolder<TFactorStorage> loaded = NFSSaveLoad::Deserialize(&ss, hostInfo);
            UNIT_ASSERT(loaded.Get());
            UNIT_ASSERT_EQUAL(1100, loaded->Size());
            for (size_t i = 0; i < 100; ++i) {
                UNIT_ASSERT_EQUAL((*loaded)[i], 0.0);
            }
            for (size_t i = 100; i < storage.Size(); ++i) {
                UNIT_ASSERT_EQUAL(storage[i - 100], (*loaded)[i]);
            }
        }
    }

    void TestStorageSizeBound() {
        for (size_t count : xrange(100)) {
            {
                TMemoryPool pool{1 << 10};
                CreateFactorStorageInPool<TGeneralResizeableFactorStorage>(&pool, count);
                Cdbg << "POOL_ALLOC=" << pool.MemoryAllocated()
                    << ", BOUND=" << GetFactorStorageInPoolSizeBound<TGeneralResizeableFactorStorage>(count) << Endl;
                UNIT_ASSERT(pool.MemoryAllocated() <=
                    GetFactorStorageInPoolSizeBound<TGeneralResizeableFactorStorage>(count));
            }
            {
                TMemoryPool pool{1 << 10};
                CreateFactorStorageInPool<TFactorStorage>(&pool, count);
                Cdbg << "POOL_ALLOC=" << pool.MemoryAllocated()
                    << ", BOUND=" << GetFactorStorageInPoolSizeBound<TFactorStorage>(count) << Endl;
                UNIT_ASSERT(pool.MemoryAllocated() <=
                    GetFactorStorageInPoolSizeBound<TFactorStorage>(count));
            }
        }
    }
};

UNIT_TEST_SUITE_REGISTRATION(TTestFactorStorage);
