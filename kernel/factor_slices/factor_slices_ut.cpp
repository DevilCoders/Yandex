#include "factor_slices.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/algorithm.h>

using namespace NFactorSlices;
using namespace NMLPool;

class TFactorSlicesTest : public TTestBase {
private:
    UNIT_TEST_SUITE(TFactorSlicesTest);
        UNIT_TEST(TestBorders);
        UNIT_TEST(TestSortedBorders);
        UNIT_TEST(TestStaticSliceInfo);
        UNIT_TEST(TestSlicesIterator);
        UNIT_TEST(TestToFromString);
        UNIT_TEST(TestBordersWithMetaInfo);
        UNIT_TEST(TestSerialize);
        UNIT_TEST(TestSerializeFromMetaInfo);
        UNIT_TEST(TestReConstructBorders);
        UNIT_TEST(TestFactorDomain);
        UNIT_TEST(TestFactorIterator);
        UNIT_TEST(TestMergeMetaInfo);
        UNIT_TEST(TestErase);
        UNIT_TEST(TestMinimal);
        UNIT_TEST(TestSaveLoad);
        UNIT_TEST(TestEnableSlices);
        UNIT_TEST(TestGetFactorBorders);
    UNIT_TEST_SUITE_END();

    template <typename Iter>
    TVector<EFactorSlice> GetIteratedSlices(Iter iter) {
        TVector<EFactorSlice> result;

        for(; iter.Valid(); iter.Next()) {
            Cdbg << "\t" "next = " << iter.Get() << Endl;
            result.push_back(iter.Get());
        }
        return result;
    }

public:
    void TestBorders() {
        TFactorBorders borders;
        UNIT_ASSERT(!borders[EFactorSlice::ALL].Contains(0));
        UNIT_ASSERT(!borders[EFactorSlice::ALL].ContainsRelative(0));

        borders[EFactorSlice::ALL] = TSliceOffsets(0, 1000);
        borders[EFactorSlice::FRESH] = TSliceOffsets(0, 100);
        borders[EFactorSlice::WEB] = TSliceOffsets(100, 900);
        borders[EFactorSlice::PERSONALIZATION] = TSliceOffsets(900, 1000);

        UNIT_ASSERT(!borders[EFactorSlice::WEB].Contains(0));
        UNIT_ASSERT(borders[EFactorSlice::WEB].Contains(100));
        UNIT_ASSERT(borders[EFactorSlice::WEB].Contains(800));
        UNIT_ASSERT(!borders[EFactorSlice::WEB].Contains(900));

        UNIT_ASSERT(borders[EFactorSlice::WEB].ContainsRelative(0));
        UNIT_ASSERT(borders[EFactorSlice::WEB].ContainsRelative(500));
        UNIT_ASSERT(borders[EFactorSlice::WEB].ContainsRelative(799));
        UNIT_ASSERT(!borders[EFactorSlice::WEB].ContainsRelative(800));
        UNIT_ASSERT(!borders[EFactorSlice::WEB].ContainsRelative(Max<TFactorIndex>()));

        UNIT_ASSERT_EQUAL(borders.SizeAll(), 1000);
        UNIT_ASSERT_EQUAL(borders[EFactorSlice::ALL].Size(), 1000);
        UNIT_ASSERT_EQUAL(borders[EFactorSlice::FRESH].Size(), 100);
        UNIT_ASSERT_EQUAL(borders[EFactorSlice::WEB].Size(), 800);
        UNIT_ASSERT_EQUAL(borders[EFactorSlice::PERSONALIZATION].Size(), 100);

        UNIT_ASSERT_EQUAL(borders.GetRelativeIndex(EFactorSlice::ALL, 0, EFactorSlice::ALL), 0);
        UNIT_ASSERT_EQUAL(borders.GetRelativeIndex(EFactorSlice::ALL, 500, EFactorSlice::ALL), 500);
        UNIT_ASSERT_EQUAL(borders.GetRelativeIndex(EFactorSlice::FRESH, 50, EFactorSlice::ALL), 50);
        UNIT_ASSERT_EQUAL(borders.GetRelativeIndex(EFactorSlice::ALL, 50, EFactorSlice::FRESH), 50);
        UNIT_ASSERT_EQUAL(borders.GetRelativeIndex(EFactorSlice::WEB, 0, EFactorSlice::WEB), 0);
        UNIT_ASSERT_EQUAL(borders.GetRelativeIndex(EFactorSlice::WEB, 500, EFactorSlice::ALL), 400);
        UNIT_ASSERT_EQUAL(borders.GetRelativeIndex(EFactorSlice::ALL, 400, EFactorSlice::WEB), 500);
    }

    void TestSortedBorders() {
        using namespace NFactorSlices::NDetail;

        TFactorBorders borders;
        UNIT_ASSERT(TSortedFactorBorders(borders).TryToValidate());

        borders[EFactorSlice::ALL] = TSliceOffsets(0, 10);
        borders[EFactorSlice::WEB] = TSliceOffsets(30, 40);
        UNIT_ASSERT(TSortedFactorBorders(borders).TryToValidate());

        borders[EFactorSlice::WEB] = TSliceOffsets(0, 10);
        UNIT_ASSERT(TSortedFactorBorders(borders).TryToValidate());

        borders[EFactorSlice::WEB] = TSliceOffsets(0, 40);
        UNIT_ASSERT(!TSortedFactorBorders(borders).TryToValidate());

        borders[EFactorSlice::FRESH] = TSliceOffsets(10, 40);
        UNIT_ASSERT(TSortedFactorBorders(borders).TryToValidate());

        TSortedFactorBorders sortedBorders(borders);

        TVector<EFactorSlice> slices;
        for (auto iter = sortedBorders.Begin(); iter.Valid(); iter.Next()) {
            Cdbg << "ORDER: " << iter.GetSlice() << " " << iter.GetOffsets().Begin << " " << iter.GetOffsets().End << Endl;
            if (!iter.GetOffsets().Empty()) {
                slices.push_back(iter.GetSlice());
            }
        }
        TVector<EFactorSlice> checkSlices = {EFactorSlice::WEB, EFactorSlice::ALL, EFactorSlice::FRESH};
        UNIT_ASSERT_EQUAL(slices, checkSlices);

        for (TFactorIndex i = 0; i != 40; ++i) {
            UNIT_ASSERT(sortedBorders.GetSliceByFactorIndex(i) != EFactorSlice::WEB);
        }

        UNIT_ASSERT_EQUAL(sortedBorders.GetSliceByFactorIndex(0), EFactorSlice::ALL);
        UNIT_ASSERT_EQUAL(sortedBorders.GetSliceByFactorIndex(9), EFactorSlice::ALL);
        UNIT_ASSERT_EQUAL(sortedBorders.GetSliceByFactorIndex(10), EFactorSlice::FRESH);
        UNIT_ASSERT_EQUAL(sortedBorders.GetSliceByFactorIndex(40), EFactorSlice::COUNT);
    }

    void TestStaticSliceInfo() {
        const TSlicesInfo* info = GetSlicesInfo();

        UNIT_ASSERT(info);
        UNIT_ASSERT(info->IsHierarchical(EFactorSlice::ALL));
        UNIT_ASSERT_EQUAL(info->GetName(EFactorSlice::ALL), TStringBuf("all"));
        UNIT_ASSERT_EQUAL(info->GetParent(EFactorSlice::ALL), EFactorSlice::COUNT);
        UNIT_ASSERT_UNEQUAL(info->GetParent(EFactorSlice::WEB), EFactorSlice::COUNT);

        UNIT_ASSERT(!info->IsChild(EFactorSlice::FRESH, EFactorSlice::WEB));
        UNIT_ASSERT(!info->IsChild(EFactorSlice::WEB, EFactorSlice::WEB));
        UNIT_ASSERT(info->IsChild(EFactorSlice::ALL, EFactorSlice::WEB_PRODUCTION));
        UNIT_ASSERT(!info->IsChild(EFactorSlice::WEB_PRODUCTION, EFactorSlice::ALL));
    }

    void TestSlicesIterator() {
        const TSlicesInfo* info = GetSlicesInfo();

        {
            Cdbg << "Iterate all slices" << Endl;
            const auto& iteratedSlices = GetIteratedSlices(TSliceIterator(EFactorSlice::ALL));
            UNIT_ASSERT(iteratedSlices.size() == GetAllFactorSlices().size() &&
                    std::equal(iteratedSlices.begin(), iteratedSlices.end(), GetAllFactorSlices().begin()));
        }

        {
            Cdbg << "Iterate all slices in " << EFactorSlice::WEB << Endl;
            auto slices = GetIteratedSlices(TSliceIterator(EFactorSlice::WEB));
            UNIT_ASSERT(slices.size() > 0);
            UNIT_ASSERT_EQUAL(slices[0], EFactorSlice::WEB);
            for (EFactorSlice slice : slices) {
                UNIT_ASSERT(slice == EFactorSlice::WEB || info->IsChild(EFactorSlice::WEB, slice));
            }
        }
        {
            Cdbg << "Iterate all leaf slices" << Endl;
            auto slices = GetIteratedSlices(TLeafIterator(EFactorSlice::ALL));
            UNIT_ASSERT_UNEQUAL(Find(slices.begin(), slices.end(), EFactorSlice::FRESH), slices.end());
            UNIT_ASSERT_UNEQUAL(Find(slices.begin(), slices.end(), EFactorSlice::WEB_PRODUCTION), slices.end());
            UNIT_ASSERT_UNEQUAL(Find(slices.begin(), slices.end(), EFactorSlice::PERSONALIZATION), slices.end());
        }
    }

    void TestToFromString() {
        UNIT_ASSERT_EQUAL(FromString<EFactorSlice>(ToString(EFactorSlice::ALL)), EFactorSlice::ALL);
    }

    void TestBordersWithMetaInfo() {
        TSlicesMetaInfo metaInfo;

        UNIT_ASSERT(!GetStaticSliceInfo(EFactorSlice::WEB_PRODUCTION).Hierarchical); // Assumption
        UNIT_ASSERT(!GetStaticSliceInfo(EFactorSlice::FRESH).Hierarchical); // Assumption

        metaInfo.SetNumFactors(EFactorSlice::WEB_PRODUCTION, 800);
        metaInfo.SetNumFactors(EFactorSlice::FRESH, 100);

        UNIT_ASSERT(metaInfo.IsSliceInitialized(EFactorSlice::WEB_PRODUCTION));
        UNIT_ASSERT(metaInfo.IsSliceInitialized(EFactorSlice::FRESH));

        UNIT_ASSERT_EQUAL(metaInfo.GetNumFactors(EFactorSlice::WEB_PRODUCTION), 800);
        UNIT_ASSERT_EQUAL(metaInfo.GetNumFactors(EFactorSlice::FRESH), 100);

        DisableSlices(metaInfo, TSliceIterator(EFactorSlice::ALL));

        metaInfo.SetSliceEnabled(EFactorSlice::FORMULA, true);

        UNIT_ASSERT(!metaInfo.IsSliceEnabled(EFactorSlice::ALL));
        UNIT_ASSERT(!metaInfo.IsSliceEnabled(EFactorSlice::WEB_PRODUCTION));
        UNIT_ASSERT(!metaInfo.IsSliceEnabled(EFactorSlice::FRESH));

        UNIT_ASSERT_EQUAL(GetStaticSliceInfo(EFactorSlice::WEB_PRODUCTION).Parent, EFactorSlice::WEB); // Assumption

        {
            TFactorBorders emptyBorders = metaInfo.GetBorders();
            UNIT_ASSERT_EQUAL(emptyBorders[EFactorSlice::ALL].Size(), 0);
            UNIT_ASSERT_EQUAL(emptyBorders[EFactorSlice::WEB].Size(), 0);
            UNIT_ASSERT_EQUAL(emptyBorders[EFactorSlice::WEB_PRODUCTION].Size(), 0);
            UNIT_ASSERT_EQUAL(emptyBorders[EFactorSlice::FRESH].Size(), 0);
        }

        TSlicesMetaInfo metaInfo2 = metaInfo;
        metaInfo2.SetSliceEnabled(EFactorSlice::WEB_PRODUCTION, true);

        {
            TFactorBorders emptyBorders = metaInfo2.GetBorders();
            UNIT_ASSERT_EQUAL(emptyBorders[EFactorSlice::ALL].Size(), 0);
            UNIT_ASSERT_EQUAL(emptyBorders[EFactorSlice::WEB].Size(), 0);
            UNIT_ASSERT_EQUAL(emptyBorders[EFactorSlice::WEB_PRODUCTION].Size(), 0);
            UNIT_ASSERT_EQUAL(emptyBorders[EFactorSlice::FRESH].Size(), 0);
        }

        TSlicesMetaInfo metaInfo3 = metaInfo2;

        metaInfo3.SetSliceEnabled(EFactorSlice::ALL, true);
        metaInfo3.SetSliceEnabled(EFactorSlice::WEB, true);
        metaInfo3.SetSliceEnabled(EFactorSlice::FRESH, true);

        UNIT_ASSERT(metaInfo3.IsSliceEnabled(EFactorSlice::ALL));
        UNIT_ASSERT(metaInfo3.IsSliceEnabled(EFactorSlice::WEB));
        UNIT_ASSERT(metaInfo3.IsSliceEnabled(EFactorSlice::WEB_PRODUCTION));
        UNIT_ASSERT(metaInfo3.IsSliceEnabled(EFactorSlice::FRESH));

        {
            TFactorBorders fullBorders = metaInfo3.GetBorders();

            Cdbg << "SLICE SIZES: " << fullBorders[EFactorSlice::ALL].Size()
                << " " << fullBorders[EFactorSlice::WEB].Size()
                << " " << fullBorders[EFactorSlice::WEB_PRODUCTION].Size()
                << " " << fullBorders[EFactorSlice::FRESH].Size()
                << Endl;

            UNIT_ASSERT_EQUAL(fullBorders[EFactorSlice::ALL].Size(), 900);
            UNIT_ASSERT_EQUAL(fullBorders[EFactorSlice::WEB].Size(), 800);
            UNIT_ASSERT_EQUAL(fullBorders[EFactorSlice::WEB_PRODUCTION].Size(), 800);
            UNIT_ASSERT_EQUAL(fullBorders[EFactorSlice::FRESH].Size(), 100);

            UNIT_ASSERT_EQUAL(fullBorders[EFactorSlice::ALL], TSliceOffsets(0, 900));
            UNIT_ASSERT_EQUAL(fullBorders[EFactorSlice::WEB], TSliceOffsets(100, 900));
            UNIT_ASSERT_EQUAL(fullBorders[EFactorSlice::WEB_PRODUCTION], TSliceOffsets(100, 900));
            UNIT_ASSERT_EQUAL(fullBorders[EFactorSlice::FRESH], TSliceOffsets(0, 100));
        }
    }

    void TestSerialize() {
        TFactorBorders borders;
        borders[EFactorSlice::ALL] = TSliceOffsets(0, 1000);
        borders[EFactorSlice::FRESH] = TSliceOffsets(0, 100);
        borders[EFactorSlice::WEB] = TSliceOffsets(100, 900);
        borders[EFactorSlice::WEB_PRODUCTION] = TSliceOffsets(100, 900);
        borders[EFactorSlice::PERSONALIZATION] = TSliceOffsets(900, 1000);

        const TString result = "all[0;1000) fresh[0;100) web[100;900) web_production[100;900) personalization[900;1000)";
        Cdbg << "ORIGINAL = " << result << Endl;
        Cdbg << "SERIALIZED = " << ToString(borders) << Endl;
        UNIT_ASSERT_EQUAL(ToString(borders), result);
        UNIT_ASSERT_EQUAL(SerializeFactorBorders(borders, ESerializationMode::LeafOnly), "fresh[0;100) web_production[100;900) personalization[900;1000)");

        TFactorBorders deserializedBorders;
        Cdbg << "ORIGINAL = " << result << Endl;
        UNIT_ASSERT(TryFromString(result, deserializedBorders));
        Cdbg << "DESERIALIZED = " << ToString(deserializedBorders) << Endl;
        UNIT_ASSERT_EQUAL(borders, deserializedBorders);

        {
            TFactorBorders borders;
            UNIT_ASSERT_NO_EXCEPTION(DeserializeFactorBorders("", borders));
            UNIT_ASSERT_EQUAL(borders, TFactorBorders());
        }

        {
            TString bordersStr = "all[0;10) XXX[0;1) web[1;5) YYY[5;10)";
            TFactorBorders borders;
            bool isException = false;
            try {
                DeserializeFactorBorders(bordersStr, borders);
            } catch (NFactorSlices::NDetail::TSliceNameError& e) {
                isException = true;
                UNIT_ASSERT_EQUAL(e.TotalNamesCount, 4);
                UNIT_ASSERT_EQUAL(e.UnknownSlices.size(), 2);
                UNIT_ASSERT_EQUAL(e.UnknownSlices["XXX"], TSliceOffsets(0, 1));
                UNIT_ASSERT_EQUAL(e.UnknownSlices["YYY"], TSliceOffsets(5, 10));
            }
            UNIT_ASSERT(isException);
            bool ok = false;
            UNIT_ASSERT_NO_EXCEPTION(ok = TryToDeserializeFactorBorders(bordersStr, borders));
            UNIT_ASSERT(!ok);
        }

        {
            TString bordersStr = "all[0;10) XXX[0;1) web[1;5) YYY[5;10)";
            TFeatureSlices sliceBorders;
            UNIT_ASSERT(TryToDeserializeFeatureSlices(bordersStr, true, sliceBorders));
            const TFeatureSlices expected = {
                {"all", 0, 10},
                {"XXX", 0, 1},
                {"web", 1, 5},
                {"YYY", 5, 10}
            };
            UNIT_ASSERT_VALUES_EQUAL(sliceBorders.size(), expected.size());
            for (size_t i = 0; i < expected.size(); ++i) {
                UNIT_ASSERT_VALUES_EQUAL(sliceBorders[i].Name, expected[i].Name);
                UNIT_ASSERT_VALUES_EQUAL(sliceBorders[i].Begin, expected[i].Begin);
                UNIT_ASSERT_VALUES_EQUAL(sliceBorders[i].End, expected[i].End);
            }
            UNIT_ASSERT(!TryToDeserializeFeatureSlices(bordersStr, false, sliceBorders));
        }

        {
            const TVector<TString> goodNames = {"all", "web"};
            const TVector<TString> unknownNames = {"XXX", "YYY"};
            const TVector<TString> allNames = {"all", "XXX", "web", "YYY"};
            TVector<TString> rejected;
            UNIT_ASSERT(ValidateBordersNames(goodNames, rejected));
            UNIT_ASSERT(rejected.empty());
            UNIT_ASSERT(!ValidateBordersNames(unknownNames, rejected));
            UNIT_ASSERT_VALUES_EQUAL(rejected, unknownNames);
            UNIT_ASSERT(!ValidateBordersNames(allNames, rejected));
            UNIT_ASSERT_VALUES_EQUAL(rejected, unknownNames);
        }

        {
            TString bordersStr = "all[0,10) web[0;5) personalization[5;10)";
            TFactorBorders borders;
            UNIT_ASSERT_EXCEPTION(DeserializeFactorBorders(bordersStr, borders), NFactorSlices::NDetail::TParseError);
            bool ok = false;
            UNIT_ASSERT_NO_EXCEPTION(ok = TryToDeserializeFactorBorders(bordersStr, borders));
            UNIT_ASSERT(!ok);
        }

        {
            TString bordersStr = "  web[10;15)    personalization[15;20) ";
            TFactorBorders borders;
            UNIT_ASSERT_NO_EXCEPTION(DeserializeFactorBorders(bordersStr, borders));
        }
    }

    void TestGetFactorBorders() {
        TFactorBorders borders;
        borders[EFactorSlice::ALL] = TSliceOffsets(0, 1000);
        borders[EFactorSlice::FRESH] = TSliceOffsets(0, 100);
        borders[EFactorSlice::WEB_PRODUCTION] = TSliceOffsets(100, 900);
        borders[EFactorSlice::PERSONALIZATION] = TSliceOffsets(900, 1000);

        {
            TVector<EFactorSlice> slices = GetFactorSlices(borders);
            UNIT_ASSERT_EQUAL(slices.size(), 3);
            UNIT_ASSERT_UNEQUAL(Find(slices.begin(), slices.end(), EFactorSlice::FRESH), slices.end());
            UNIT_ASSERT_UNEQUAL(Find(slices.begin(), slices.end(), EFactorSlice::WEB_PRODUCTION), slices.end());
            UNIT_ASSERT_UNEQUAL(Find(slices.begin(), slices.end(), EFactorSlice::PERSONALIZATION), slices.end());
            UNIT_ASSERT_EQUAL(Find(slices.begin(), slices.end(), EFactorSlice::ALL), slices.end());
            for (EFactorSlice slice : slices) {
                UNIT_ASSERT(!borders[slice].Empty());
            }
        }

        {
            TVector<EFactorSlice> slices = GetFactorSlices(borders, ESerializationMode::All);
            UNIT_ASSERT_EQUAL(slices.size(), 4);
            UNIT_ASSERT_UNEQUAL(Find(slices.begin(), slices.end(), EFactorSlice::FRESH), slices.end());
            UNIT_ASSERT_UNEQUAL(Find(slices.begin(), slices.end(), EFactorSlice::WEB_PRODUCTION), slices.end());
            UNIT_ASSERT_UNEQUAL(Find(slices.begin(), slices.end(), EFactorSlice::PERSONALIZATION), slices.end());
            UNIT_ASSERT_UNEQUAL(Find(slices.begin(), slices.end(), EFactorSlice::ALL), slices.end());
        }
    }

    void TestSerializeFromMetaInfo() {
        TSlicesMetaInfo metaInfo;
        metaInfo.SetNumFactors(EFactorSlice::WEB_PRODUCTION, 500);
        metaInfo.SetNumFactors(EFactorSlice::FRESH, 100);

        metaInfo.SetSliceEnabled(EFactorSlice::FORMULA, true);
        metaInfo.SetSliceEnabled(EFactorSlice::WEB_PRODUCTION, true);
        metaInfo.SetSliceEnabled(EFactorSlice::ALL, true);
        metaInfo.SetSliceEnabled(EFactorSlice::WEB, true);
        metaInfo.SetSliceEnabled(EFactorSlice::FRESH, true);

        TFactorBorders fullBorders = metaInfo.GetBorders();
        TFactorBorders deserializedBorders;
        DeserializeFactorBorders(SerializeFactorBorders(fullBorders), deserializedBorders);

        UNIT_ASSERT_EQUAL(fullBorders, deserializedBorders);
    }

    void TestReConstructBorders() {
        using namespace NFactorSlices::NDetail;

        // Empty
        {
            TFactorBorders borders;
            UNIT_ASSERT(borders.TryToValidate());
            UNIT_ASSERT(ReConstruct(borders));
            UNIT_ASSERT_EQUAL(borders[EFactorSlice::ALL].Size(), 0);
        }

        // One leaf
        {
            TFactorBorders borders;
            borders[EFactorSlice::WEB_PRODUCTION] = TSliceOffsets(0, 100);
            UNIT_ASSERT(!borders.TryToValidate());
            UNIT_ASSERT(ReConstruct(borders));
            UNIT_ASSERT_EQUAL(borders[EFactorSlice::ALL], TSliceOffsets(0, 100));
            UNIT_ASSERT_EQUAL(borders[EFactorSlice::WEB], TSliceOffsets(0, 100));
            UNIT_ASSERT(borders.TryToValidate());
        }

        // Two leaves
        {
            TFactorBorders borders;
            borders[EFactorSlice::WEB_PRODUCTION] = TSliceOffsets(0, 100);
            borders[EFactorSlice::WEB_META] = TSliceOffsets(100, 200);
            UNIT_ASSERT(!borders.TryToValidate());
            UNIT_ASSERT(ReConstruct(borders));
            UNIT_ASSERT_EQUAL(borders[EFactorSlice::WEB_PRODUCTION], TSliceOffsets(0, 100));
            UNIT_ASSERT_EQUAL(borders[EFactorSlice::WEB_META], TSliceOffsets(100, 200));
            UNIT_ASSERT_EQUAL(borders[EFactorSlice::WEB], TSliceOffsets(0, 200));
            UNIT_ASSERT_EQUAL(borders[EFactorSlice::ALL], TSliceOffsets(0, 200));
            UNIT_ASSERT(borders.TryToValidate());
        }

        // One hierarchical with >1 child
        {
            TFactorBorders borders;
            borders[EFactorSlice::WEB] = TSliceOffsets(0, 100);
            TFactorBorders bordersCopy = borders;
            UNIT_ASSERT(!borders.TryToValidate());
            UNIT_ASSERT(!ReConstruct(borders));
            UNIT_ASSERT_EQUAL(borders, bordersCopy);
        }

        // One leaf, wrong offsets
        {
            TFactorBorders borders;
            borders[EFactorSlice::WEB_PRODUCTION] = TSliceOffsets(1, 100);
            TFactorBorders bordersCopy = borders;
            UNIT_ASSERT(!borders.TryToValidate());
            UNIT_ASSERT(!ReConstruct(borders));
            UNIT_ASSERT_EQUAL(borders, bordersCopy);
        }

        // Two leaves, wrong order
        {
            TFactorBorders borders;
            borders[EFactorSlice::WEB_PRODUCTION] = TSliceOffsets(100, 200);
            borders[EFactorSlice::WEB_META] = TSliceOffsets(0, 100);
            TFactorBorders bordersCopy = borders;
            UNIT_ASSERT(!borders.TryToValidate());
            UNIT_ASSERT(!ReConstruct(borders));
            UNIT_ASSERT_EQUAL(borders, bordersCopy);
        }

        // Two leaves, overlap
        {
            TFactorBorders borders;
            borders[EFactorSlice::WEB_PRODUCTION] = TSliceOffsets(0, 120);
            borders[EFactorSlice::WEB_META] = TSliceOffsets(80, 200);
            TFactorBorders bordersCopy = borders;
            UNIT_ASSERT(!borders.TryToValidate());
            UNIT_ASSERT(!ReConstruct(borders));
            UNIT_ASSERT_EQUAL(borders, bordersCopy);
        }

        // Two leaves, one contains another
        {
            TFactorBorders borders;
            borders[EFactorSlice::WEB_PRODUCTION] = TSliceOffsets(0, 100);
            borders[EFactorSlice::WEB_META] = TSliceOffsets(0, 200);
            TFactorBorders bordersCopy = borders;
            UNIT_ASSERT(!borders.TryToValidate());
            UNIT_ASSERT(!ReConstruct(borders));
            UNIT_ASSERT_EQUAL(borders, bordersCopy);
        }

        // Bad hierarchical slice with IgnoreHierarchicalBorders=true
        {
            TFactorBorders borders;
            borders[EFactorSlice::FRESH] = TSliceOffsets(0, 100);
            borders[EFactorSlice::WEB_PRODUCTION] = TSliceOffsets(100, 1000);
            borders[EFactorSlice::FORMULA] = TSliceOffsets(100, 1000);

            TReConstructOptions options;
            options.IgnoreHierarchicalBorders = true;
            UNIT_ASSERT(!borders.TryToValidate());
            UNIT_ASSERT(ReConstruct(borders, options));
            UNIT_ASSERT_EQUAL(borders[EFactorSlice::FORMULA], TSliceOffsets(0, 1000));
        }
    }

    void TestFactorDomain() {
        {
            TFactorDomain domain;
            UNIT_ASSERT(domain.IsNormal());
            UNIT_ASSERT_EQUAL(domain.Size(), 0);
            UNIT_ASSERT(domain.GetBorders().TryToValidate());
        }

        {
            TFactorDomain domain(100);
            UNIT_ASSERT(domain.IsNormal());
            UNIT_ASSERT_EQUAL(domain.Size(), 100);
            UNIT_ASSERT(domain.GetBorders().TryToValidate());
        }

        {
            TFactorBorders borders;
            borders[EFactorSlice::FRESH] = TSliceOffsets(0, 10);
            borders[EFactorSlice::WEB_PRODUCTION] = TSliceOffsets(0, 10);
            TFactorDomain domain(borders);
            UNIT_ASSERT(!domain.IsNormal());
        }

        {
            TFactorDomain domain(2);
            UNIT_ASSERT(domain.IsNormal());
            TFactorIndex trIndex = 0;
            UNIT_ASSERT(domain.TryGetRelativeIndexByName(EFactorSlice::WEB_PRODUCTION, "TR", trIndex));
            UNIT_ASSERT_EQUAL(trIndex, 1);
        }

        {
            TFactorDomain srcDomain(2);
            {
                TFactorDomain domain = srcDomain.MakeDomainWithIncreasedSlice(EFactorSlice::WEB_META, 3);
                UNIT_ASSERT(domain.IsNormal());
                TFactorIndex trIndex = 0;

                UNIT_ASSERT(domain.TryGetRelativeIndexByName(EFactorSlice::WEB_PRODUCTION, "TR", trIndex));
                UNIT_ASSERT_EQUAL(trIndex, 1);
                UNIT_ASSERT_EQUAL(domain[EFactorSlice::WEB_META], TSliceOffsets(2, 5));
            }
            {
                TFactorDomain domain = srcDomain.MakeDomainWithIncreasedSlice(EFactorSlice::WEB_PRODUCTION, 3);
                UNIT_ASSERT(domain.IsNormal());
                TFactorIndex trIndex = 0;

                UNIT_ASSERT(domain.TryGetRelativeIndexByName(EFactorSlice::WEB_PRODUCTION, "TR", trIndex));
                UNIT_ASSERT_EQUAL(trIndex, 1);
                UNIT_ASSERT_EQUAL(domain[EFactorSlice::WEB_PRODUCTION], TSliceOffsets(0, 5));
            }
        }
    }

    void TestFactorIterator() {
        {
            TFactorDomain domain;
            UNIT_ASSERT(domain.IsNormal());
            auto iter = domain.Begin();
            UNIT_ASSERT(!iter.Valid());
            UNIT_ASSERT(!domain.End().Valid());
            UNIT_ASSERT_EQUAL(iter, domain.End());
            iter = domain.Begin(EFactorSlice::WEB_PRODUCTION);
            UNIT_ASSERT(!iter.Valid());
            UNIT_ASSERT_EQUAL(iter, domain.End());
        }

        {
            TFactorBorders borders;
            borders[EFactorSlice::WEB_PRODUCTION] = TSliceOffsets(0,1);
            UNIT_ASSERT(NFactorSlices::NDetail::ReConstruct(borders));
            TFactorDomain domain(borders);
            UNIT_ASSERT(domain.IsNormal());
            auto iter = domain.Begin();
            iter.NextLeaf();
            UNIT_ASSERT(!iter.Valid());
        }

        {
            TFactorBorders borders;
            borders[EFactorSlice::WEB_PRODUCTION] = TSliceOffsets(0, 10);
            borders[EFactorSlice::WEB_META] = TSliceOffsets(10, 20);
            UNIT_ASSERT(NFactorSlices::NDetail::ReConstruct(borders));
            TFactorDomain domain(borders);
            UNIT_ASSERT(domain.IsNormal());

            {
                auto iter = domain.Begin();
                UNIT_ASSERT(iter.Valid());
                for (TFactorIndex i = 0; i != 10; ++i) {
                    Cdbg << i << " " << iter.GetLeaf() << Endl;
                    UNIT_ASSERT_EQUAL(iter.GetLeaf(), EFactorSlice::WEB_PRODUCTION);
                    UNIT_ASSERT_EQUAL(TFactorIndex(iter.GetFactorInfo().GetIndex()), i);
                    UNIT_ASSERT_EQUAL(iter.GetIndexInLeaf(), i);
                    UNIT_ASSERT_EQUAL(iter.GetIndex(), i);
                    iter.Next();
                }
                UNIT_ASSERT(iter.Valid());
                for (TFactorIndex i = 0; i != 10; ++i) {
                    Cdbg << i << " " << iter.GetLeaf() << Endl;
                    UNIT_ASSERT_EQUAL(iter.GetLeaf(), EFactorSlice::WEB_META);
                    UNIT_ASSERT_EQUAL(TFactorIndex(iter.GetFactorInfo().GetIndex()), i);
                    UNIT_ASSERT_EQUAL(iter.GetIndexInLeaf(), i);
                    UNIT_ASSERT_EQUAL(iter.GetIndex(), 10 + i);
                    iter.Next();
                }
            }

            {
                auto iter = domain.Begin();
                UNIT_ASSERT_EQUAL(iter.GetLeafSize(), 10);
                iter.NextLeaf();
                UNIT_ASSERT_EQUAL(iter.GetIndex(), 10);
                UNIT_ASSERT_EQUAL(iter.GetLeafSize(), 10);
                iter.NextLeaf();
                UNIT_ASSERT_EQUAL(iter, domain.End());
            }

            {
                TFactorBorders borders2;
                borders2[EFactorSlice::WEB_PRODUCTION] = TSliceOffsets(0, 20);
                borders2[EFactorSlice::WEB_META] = TSliceOffsets(20, 25);
                borders2[EFactorSlice::PERSONALIZATION] = TSliceOffsets(25, 30);
                UNIT_ASSERT(NFactorSlices::NDetail::ReConstruct(borders2));
                TFactorDomain domain2(borders2);
                UNIT_ASSERT(domain2.IsNormal());
            }

            {
                TFactorBorders borders2;
                borders2[EFactorSlice::WEB_PRODUCTION] = TSliceOffsets(0, 10);
                UNIT_ASSERT(NFactorSlices::NDetail::ReConstruct(borders2));
                TFactorDomain domain2(borders2);
                UNIT_ASSERT(domain2.IsNormal());
            }

            {
                TFactorBorders borders2;
                borders2[EFactorSlice::WEB_PRODUCTION] = TSliceOffsets(0, 10);
                borders2[EFactorSlice::PERSONALIZATION] = TSliceOffsets(10, 20);
                UNIT_ASSERT(NFactorSlices::NDetail::ReConstruct(borders2));
                TFactorDomain domain2(borders2);
                UNIT_ASSERT(domain2.IsNormal());
            }
        }
    }

    TSlicesMetaInfo GetMerged(const TSlicesMetaInfo& infoX, const TSlicesMetaInfo& infoY) {
        TSlicesMetaInfo infoZ = infoX;
        infoZ.Merge(infoY);
        return infoZ;
    }

    void TestMergeMetaInfo() {
        TSlicesMetaInfo infoX;
        infoX.SetNumFactors(EFactorSlice::WEB_PRODUCTION, 50);
        infoX.SetNumFactors(EFactorSlice::FRESH, 100);
        EnableSlices(infoX, EFactorSlice::WEB_PRODUCTION);

        TSlicesMetaInfo infoY;
        infoY.SetNumFactors(EFactorSlice::WEB_PRODUCTION, 100);
        infoY.SetNumFactors(EFactorSlice::FRESH, 50);
        infoY.SetNumFactors(EFactorSlice::PERSONALIZATION, 30);
        EnableSlices(infoY, EFactorSlice::WEB_PRODUCTION, EFactorSlice::FRESH);

        TSlicesMetaInfo infoZ;
        infoZ.SetNumFactors(EFactorSlice::WEB_PRODUCTION, 100);
        infoZ.SetNumFactors(EFactorSlice::FRESH, 100);
        infoZ.SetNumFactors(EFactorSlice::PERSONALIZATION, 30);
        EnableSlices(infoZ, EFactorSlice::WEB_PRODUCTION, EFactorSlice::FRESH);

        TSlicesMetaInfo infoEmpty;

        UNIT_ASSERT_EQUAL(GetMerged(infoEmpty, infoEmpty), infoEmpty);
        UNIT_ASSERT_EQUAL(GetMerged(infoX, infoX), infoX);
        UNIT_ASSERT_EQUAL(GetMerged(infoY, infoY), infoY);

        UNIT_ASSERT_EQUAL(GetMerged(infoX, infoEmpty), infoX);
        UNIT_ASSERT_EQUAL(GetMerged(infoEmpty, infoX), infoX);

        UNIT_ASSERT_EQUAL(GetMerged(infoY, infoEmpty), infoY);
        UNIT_ASSERT_EQUAL(GetMerged(infoEmpty, infoY), infoY);

        UNIT_ASSERT_EQUAL(GetMerged(infoX, infoY), infoZ);
        UNIT_ASSERT_EQUAL(GetMerged(infoY, infoX), infoZ);

        UNIT_ASSERT_EQUAL(GetMerged(infoZ, infoX), infoZ);
        UNIT_ASSERT_EQUAL(GetMerged(infoY, infoZ), infoZ);
    }

    void TestErase() {
        TSliceOffsets offsets(10, 100);

        offsets.Erase(TSliceOffsets(0,0));
        UNIT_ASSERT_EQUAL(offsets, TSliceOffsets(10, 100));

        offsets.Erase(TSliceOffsets(100, 200));
        UNIT_ASSERT_EQUAL(offsets, TSliceOffsets(10, 100));

        offsets.Erase(TSliceOffsets(50, 60));
        UNIT_ASSERT_EQUAL(offsets, TSliceOffsets(10, 90));

        offsets.Erase(TSliceOffsets(80, 200));
        UNIT_ASSERT_EQUAL(offsets, TSliceOffsets(10, 80));

        offsets.Erase(TSliceOffsets(5, 15));
        UNIT_ASSERT_EQUAL(offsets, TSliceOffsets(5, 70));

        offsets.Erase(TSliceOffsets(0, 90));
        UNIT_ASSERT_EQUAL(offsets, TSliceOffsets(0, 0));
    }

    void TestMinimal() {
        TFactorBorders borders;
        DeserializeFactorBorders("web_production[0;500) web_meta[500;600) web[0;600)", borders);
        UNIT_ASSERT(borders.IsMinimal(TSliceOffsets(0, 0)));
        UNIT_ASSERT(borders.IsMinimal(TSliceOffsets(1, 1)));
        UNIT_ASSERT(borders.IsMinimal(TSliceOffsets(1000, 1000)));
        UNIT_ASSERT(borders.IsMinimal(TSliceOffsets(0, 300)));
        UNIT_ASSERT(!borders.IsMinimal(TSliceOffsets(0, 500)));
        UNIT_ASSERT(!borders.IsMinimal(TSliceOffsets(0, 550)));
        UNIT_ASSERT(borders.IsMinimal(TSliceOffsets(100, 550)));
        UNIT_ASSERT(!borders.IsMinimal(TSliceOffsets(0, 1000)));
        UNIT_ASSERT(!borders.IsMinimal(TSliceOffsets(50, 1000)));
    }

    template <typename T>
    void CheckSaveLoad(const T& obj) {
        TStringStream str;
        ::Save(&str, obj);
        T objLoaded;
        ::Load(&str, objLoaded);
        UNIT_ASSERT_EQUAL(obj, objLoaded);
    }

    void TestSaveLoad() {
        CheckSaveLoad(TSliceOffsets());
        CheckSaveLoad(TSliceOffsets(11, 1234));

        CheckSaveLoad(TFactorBorders());
        TFactorBorders borders;
        borders[EFactorSlice::ALL] = TSliceOffsets(0, 1000);
        borders[EFactorSlice::FRESH] = TSliceOffsets(0, 100);
        borders[EFactorSlice::WEB] = TSliceOffsets(100, 900);
        borders[EFactorSlice::PERSONALIZATION] = TSliceOffsets(900, 1000);
        CheckSaveLoad(borders);
        borders = TFactorBorders();
        borders[EFactorSlice::ALL] = TSliceOffsets(0, 1);
        borders[EFactorSlice::WEB_META] = TSliceOffsets(1, 2);
        CheckSaveLoad(borders);

        CheckSaveLoad(TFactorDomain());
        CheckSaveLoad(TFactorDomain(borders));
    }

    void TestEnableSlices() {
        TSlicesMetaInfo infoX;
        infoX.SetNumFactors(EFactorSlice::WEB_PRODUCTION, 100);
        infoX.SetNumFactors(EFactorSlice::FRESH, 100);

        EnableSlices(infoX, EFactorSlice::WEB_PRODUCTION);
        UNIT_ASSERT(TFactorBorders(infoX)[EFactorSlice::WEB_PRODUCTION].Size() > 0);
        EnableSlices(infoX, EFactorSlice::FRESH);
        UNIT_ASSERT(TFactorBorders(infoX)[EFactorSlice::FRESH].Size() > 0);
    }
};

UNIT_TEST_SUITE_REGISTRATION(TFactorSlicesTest);
