#include "formula_storage.h"

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/resource/resource.h>

#include <util/folder/path.h>
#include <util/generic/is_in.h>
#include <util/generic/string.h>
#include <util/stream/file.h>

const size_t FLOAT_FORMULA_DATA_SIZE = 3;
const size_t FLOAT_FORMULA_FACTORS_COUNT = 101;
const float DATA[FLOAT_FORMULA_DATA_SIZE][FLOAT_FORMULA_FACTORS_COUNT] = {
        {2.21074e+08, 1.43609e+06, 33957, 0.649597, 37, 120000, 18.9875, 0.508095, 0.0479452, 2474, 0, 0.428815, 5.11, 0.0387097, 2, 1, 97101, 0, 0, 0, 0, 0, -230.259, 0.969231, 509208, 0, 1.08494e+06, 26743, 0.204, 0, 0, 0, 1343144064, -2.03244, 0, 948, 343, 132879, 42, 16, 0, 85, 1, 1343111552, 1343111552, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -0.310355, -1.12571, 0, 0, 13, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0.0777285, 361, -6.64386, 0, 0, 0, 140, 46, 24, 10, 0, 96, 1, 1343124864, 1343124864, 0.341, 0.0633271, 0.0340909, 0, 0.00225047468216, 0.00179685179685, 0.0002673002673, 0.0056811852512, 0.000222791578478, 0.000414651002073, 0.0, 0.0056811852512},
        {954636, 115393, 9601, 12.0876, 26, 96458, 39.9231, 0.349938, 0.160959, 1431, 0, 65.4699, -1, 0.0173077, 4, 0, 0, 0.0956522, 0, 0, 0, 0, -230.259, 0.840909, 46200, 1, 75392, 3856, 0.0277, 0.075, 0.8, 0.444949, 1344566912, 14.5119, 0, 625, 54, 0, 32, 6, 0, 1846, 1, 1344445696, 1344445696, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 42, 12.8984, 4.49869, 11.7552, 4.65881, 0, 0, 0, 0, 0, 174, 45, 28, 0.16092, 0.622222, 0.641667, 12, 4.92835, 0, 9, 0, 5, 3, 86, 3, 0, 40, 1, 1344508416, 1344508416, 0, 0.523759, 0.00424528, 0, 0.20320059546, 0.0299789621318, 0.00569775596073, 0.5, 0.0, 0.5, 4.0, 0.833333333333},
        {3.17168e+07, 107270, 1411, 0.338212, 34, 113100, 13.6048, 0.0791059, 0.0538178, 592, 0, 2.52863, -1, 0.0772926, 2, 0, 0, 0, 0, 0, 0, 0, -230.259, 0, 0, 0, 0, 0, 0, 0.112919, 0, 0, 1345626624, -2.97696, 0, 802, 165, 0, 209, 59, 0, 20, 1, 1345594240, 1345594240, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -3.11626, -1.91191, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -5.97361, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.00277900308778, 0.00149758779544, 0.000155458940703, 0.000589622641509, 0.0, 0.0, 0.0, 0.0}
};
const size_t FROM_FML_FACTORS_COUNT = 119;
const float FROM_FML_DATA[FLOAT_FORMULA_DATA_SIZE][FROM_FML_FACTORS_COUNT] = {
        {2.21074e+08, 1.43609e+06, 33957, 0.649597, 37, 120000, 18.9875, 0.508095, 0.0479452, 2474, 0, 0.428815, 5.11, 0.0387097, 2, 1, 97101, 0, 0, 0, 0, 0, -230.259, 0.969231, 509208, 0, 1.08494e+06, 26743, 0.204, 0, 0, 0, 1343144064, -2.03244, 0, 948, 343, 132879, 42, 16, 0, 85, 1, 1343111552, 1343111552, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -0.310355, -1.12571, 0, 0, 13, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0.0777285, 361, -6.64386, 0, 0, 0, 140, 46, 24, 10, 0, 96, 1, 1343124864, 1343124864, 0.341, 0.0633271, 0.0340909, 0, 0.00225047468216, 0.00179685179685, 0.0002673002673, 0.0056811852512, 0.000222791578478, 0.000414651002073, 0.0, 0.0056811852512, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
        {954636, 115393, 9601, 12.0876, 26, 96458, 39.9231, 0.349938, 0.160959, 1431, 0, 65.4699, -1, 0.0173077, 4, 0, 0, 0.0956522, 0, 0, 0, 0, -230.259, 0.840909, 46200, 1, 75392, 3856, 0.0277, 0.075, 0.8, 0.444949, 1344566912, 14.5119, 0, 625, 54, 0, 32, 6, 0, 1846, 1, 1344445696, 1344445696, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 42, 12.8984, 4.49869, 11.7552, 4.65881, 0, 0, 0, 0, 0, 174, 45, 28, 0.16092, 0.622222, 0.641667, 12, 4.92835, 0, 9, 0, 5, 3, 86, 3, 0, 40, 1, 1344508416, 1344508416, 0, 0.523759, 0.00424528, 0, 0.20320059546, 0.0299789621318, 0.00569775596073, 0.5, 0.0, 0.5, 4.0, 0.833333333333, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
        {3.17168e+07, 107270, 1411, 0.338212, 34, 113100, 13.6048, 0.0791059, 0.0538178, 592, 0, 2.52863, -1, 0.0772926, 2, 0, 0, 0, 0, 0, 0, 0, -230.259, 0, 0, 0, 0, 0, 0, 0.112919, 0, 0, 1345626624, -2.97696, 0, 802, 165, 0, 209, 59, 0, 20, 1, 1345594240, 1345594240, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -3.11626, -1.91191, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -5.97361, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.00277900308778, 0.00149758779544, 0.000155458940703, 0.000589622641509, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}
};
const TVector<double> INFO_VALUES = {0.1113263824, 0.583931613, 0.05911373111};
const TVector<double> MNMC_VALUES = {0.3194759758, 0.4648976219, 0.2556718554};
const TVector<double> REGTREE_VALUES = {23.57784462, 23.57747078, 11.72181702};
const TVector<double> FROM_FML_VALUES = {0.26111505, 0.5270061994, 0.23959038};

const double EPS = 1e-6;

const size_t CATEGS_NUMBER = 11;
const double CATEGS[CATEGS_NUMBER] = {0.09677932417, 0.1280059047, 0.2179983362, 0.04482820069, 0.06405254919, 0.1989275335, 0.07398405226, 0.04538809479, 0.04357800935, 0.04305532486, 0.04340267035};
const double CATEGS_VALUES[CATEGS_NUMBER] = {0, 0.125, 0.3000000119, 0.474999994, 0.25, 0.349999994, 0.174999997, 0.6000000238, 0.75, 0.8999999762, 0.4499999881};
const double MAX_CATEG_VALUE = 0.3000000119;

class TFormulasStorageTest: public TTestBase {
private:
    UNIT_TEST_SUITE(TFormulasStorageTest);
        UNIT_TEST(TestAppend);
        UNIT_TEST(TestXtdAppend);
        UNIT_TEST(TestRemove);
        UNIT_TEST(TestSimpleCalculations);
        UNIT_TEST(TestXtdCalculations);
        UNIT_TEST(TestRegTreeCalculations);
        UNIT_TEST(TestCatboostCalculations);
        UNIT_TEST(TestMultiCategCalculations);
        UNIT_TEST(TestFormulaNames);
        UNIT_TEST(TestMultipleReturnValues);
        UNIT_TEST(TestMultiCategCategsCalculations);
        UNIT_TEST(TestArchive);
        UNIT_TEST(TestMnMCArchive);
        UNIT_TEST(TestErrorLogCompletion);
        UNIT_TEST(TestErrorLogException);
        UNIT_TEST(TestMd5);
        UNIT_TEST(TestFmlIDStorage);
    UNIT_TEST_SUITE_END();

    float Data[FLOAT_FORMULA_DATA_SIZE][FLOAT_FORMULA_FACTORS_COUNT];
    float FromFmlData[FLOAT_FORMULA_DATA_SIZE][FROM_FML_FACTORS_COUNT];

public:
    TFormulasStorageTest() {
        static_assert(sizeof(DATA) == sizeof(Data), "expect sizeof(DATA) == sizeof(Data)");
        memcpy(Data, DATA, sizeof(DATA));
        static_assert(sizeof(FROM_FML_DATA) == sizeof(FromFmlData), "expect sizeof(FROM_FML_DATA) == sizeof(FromFmlData)");
        memcpy(FromFmlData, FROM_FML_DATA, sizeof(FROM_FML_DATA));
    }

    void SetUp() override {
        TFsPath path("formulas");
        path.MkDir();
        TFsPath mnmcPath("formulas/mnmc");
        mnmcPath.MkDir();
        TFsPath regtreePath("formulas/regtree");
        regtreePath.MkDir();
        TFsPath archivePath("formulas/archive");
        archivePath.MkDir();
        TFsPath fromFMLPath("formulas/from_fml");
        fromFMLPath.MkDir();

        TString dataInfo = NResource::Find("info");
        TString dataMnmc = NResource::Find("mnmc");
        TString dataRegtree = NResource::Find("regtree");
        TString dataArch = NResource::Find("models");
        TString dataMnMCArch = NResource::Find("archive_with_mnmc");
        TString dataFromFML = NResource::Find("from_fml");
        TString dataXtd = NResource::Find("xtd");

        TString dataCatboostInfo = NResource::Find("catboost_info");

        TFileOutput outInfo("formulas/info.info");
        outInfo.Write(dataInfo);
        outInfo.Finish();

        TFileOutput outMnmc("formulas/mnmc/mnmc.mnmc");
        outMnmc.Write(dataMnmc);
        outMnmc.Finish();

        TFileOutput outRegtree("formulas/regtree/regtree.regtree");
        outRegtree.Write(dataRegtree);
        outRegtree.Finish();

        TFileOutput outArch("formulas/archive/models.archive");
        outArch.Write(dataArch);
        outArch.Finish();

        TFileOutput outMnMCArch("formulas/archive/with_mnmc.archive");
        outMnMCArch.Write(dataMnMCArch);
        outMnMCArch.Finish();

        TFileOutput outFromFML("formulas/from_fml/from_fml.info");
        outFromFML.Write(dataFromFML);
        outFromFML.Finish();

        TFileOutput outXtd("formulas/xtd.xtd");
        outXtd.Write(dataXtd);
        outXtd.Finish();

        {
            TFileOutput outCatboostInfo("formulas/catboost.cbm");
            outCatboostInfo.Write(dataCatboostInfo);
        }
    }

    void TearDown() override {
        TFsPath path("formulas");
        path.ForceDelete();
    }

    void TestAppend() {
        TFormulasStorage storage;

        UNIT_ASSERT_VALUES_EQUAL(storage.Size(), 0);
        UNIT_ASSERT(storage.Empty());

        storage.AddFormula("f1", "formulas/info.info");
        UNIT_ASSERT_VALUES_EQUAL(storage.Size(), 1);
        UNIT_ASSERT(!storage.Empty());

        bool check = storage.AddFormula("f1", "formulas/info.info");
        UNIT_ASSERT(!check);
        UNIT_ASSERT_VALUES_EQUAL(storage.Size(), 1);
        UNIT_ASSERT(!storage.Empty());

        check = storage.AddFormula("infoFormula", "formulas/info.info");
        UNIT_ASSERT(check);
        UNIT_ASSERT_VALUES_EQUAL(storage.Size(), 2);

        check = storage.AddFormula("f2", "formulas/mnmc/mnmc.mnmc");
        UNIT_ASSERT(check);
        UNIT_ASSERT_VALUES_EQUAL(storage.Size(), 3);

        check = storage.AddFormula("f3", "formulas/regtree/regtree.regtree");
        UNIT_ASSERT(check);
        UNIT_ASSERT_VALUES_EQUAL(storage.Size(), 4);

        storage.AddFormulasFromDirectory("formulas");
        UNIT_ASSERT_VALUES_EQUAL(storage.Size(), 7);
        storage.Finalize();
        UNIT_ASSERT_VALUES_EQUAL(storage.Size(), 6);

        check = storage.AddFormula("formulas/mnmc/mnmc.mnmc");
        UNIT_ASSERT(check);
        UNIT_ASSERT_VALUES_EQUAL(storage.Size(), 7);

        check = storage.AddFormula("formulas/info.info");
        UNIT_ASSERT(!check);
        UNIT_ASSERT_VALUES_EQUAL(storage.Size(), 7);

        storage.AddFormulasFromDirectoryRecursive("formulas");
        storage.Finalize();

        UNIT_ASSERT_VALUES_EQUAL(storage.Size(), 10);
        UNIT_ASSERT(!storage.Empty());

        storage.Clear();
        UNIT_ASSERT(storage.Empty());
        storage.AddValidFormulasFromDirectory("formulas");
        storage.Finalize();
        UNIT_ASSERT(!storage.Empty());
        UNIT_ASSERT_VALUES_EQUAL(storage.Size(), 2);

        storage.Clear();
        UNIT_ASSERT(storage.Empty());
        storage.AddValidFormulasFromDirectoryRecursive("formulas");
        storage.Finalize();
        UNIT_ASSERT(!storage.Empty());
        UNIT_ASSERT_VALUES_EQUAL(storage.Size(), 6);
    }

    void TestXtdAppend() {
        NFormulaStorageImpl::TFormulasStorage<NExtendedMx::TExtendedRelevCalcer> storage(true);

        storage.AddFormulasFromDirectory("formulas");
        UNIT_ASSERT(!storage.Empty());
        UNIT_ASSERT_VALUES_EQUAL(storage.Size(), 3);

        storage.Finalize();
        UNIT_ASSERT(!storage.Empty());
        UNIT_ASSERT_VALUES_EQUAL(storage.Size(), 3);
        UNIT_ASSERT(storage.GetErrorLog().Empty());
    }

    void TestRemove() {
        TFormulasStorage storage;

        UNIT_ASSERT(storage.Empty());

        storage.AddFormulasFromDirectoryRecursive("formulas");
        storage.Finalize();
        UNIT_ASSERT_VALUES_EQUAL(storage.Size(), 6);
        UNIT_ASSERT(!storage.Empty());

        bool check = storage.RemoveFormula("info");
        UNIT_ASSERT(check);
        UNIT_ASSERT_VALUES_EQUAL(storage.Size(), 5);

        check = storage.RemoveFormula("unused_name");
        UNIT_ASSERT(!check);
        UNIT_ASSERT_VALUES_EQUAL(storage.Size(), 5);

        check = storage.AddFormula("info", "formulas/info.info");
        UNIT_ASSERT(check);
        UNIT_ASSERT_VALUES_EQUAL(storage.Size(), 6);

        storage.Clear();
        UNIT_ASSERT_VALUES_EQUAL(storage.Size(), 0);
        UNIT_ASSERT(storage.Empty());

        check = storage.AddFormula("info", "formulas/mnmc/mnmc.mnmc");
        UNIT_ASSERT(check);
        UNIT_ASSERT_VALUES_EQUAL(storage.Size(), 1);
        UNIT_ASSERT(!storage.Empty());
    }

    void TestSimpleCalculations() {
        TFormulasStorage storage;
        storage.AddFormula("info", "formulas/info.info");

        for (size_t idx = 0; idx < FLOAT_FORMULA_DATA_SIZE; ++idx) {
            double value = storage.GetFormulaValue("info", Data[idx]);
            UNIT_ASSERT_DOUBLES_EQUAL(value, INFO_VALUES[idx], EPS);
        }
    }

    void TestXtdCalculations() {
        NFormulaStorageImpl::TFormulasStorage<NExtendedMx::TExtendedRelevCalcer> storage;
        storage.AddFormula("info", "formulas/info.info");
        storage.AddFormula("xtd", "formulas/xtd.xtd");
        storage.Finalize();

        UNIT_ASSERT_VALUES_EQUAL(storage.Size(), 2);

        for (size_t idx = 0; idx < FLOAT_FORMULA_DATA_SIZE; ++idx) {
            double value = storage.GetFormulaValue("xtd", Data[idx]);
            UNIT_ASSERT_DOUBLES_EQUAL(value, 0.0, EPS);
        }

        NExtendedMx::TCalcContext ctx;
        for (size_t idx = 0; idx < FLOAT_FORMULA_DATA_SIZE; ++idx) {
            UNIT_ASSERT(storage.GetFormula("xtd") != nullptr);
            double value = storage.GetFormula("xtd")->DoCalcRelevExtended(Data[idx], FLOAT_FORMULA_FACTORS_COUNT, ctx);
            UNIT_ASSERT_DOUBLES_EQUAL(value, INFO_VALUES[idx], EPS);
        }
    }

    void TestRegTreeCalculations() {
        TFormulasStorage storage;
        storage.AddFormula("regtree", "formulas/regtree/regtree.regtree");

        for (size_t idx = 0; idx < FLOAT_FORMULA_DATA_SIZE; ++idx) {
            double value = storage.GetFormulaValue("regtree", Data[idx]);
            UNIT_ASSERT_DOUBLES_EQUAL(value, REGTREE_VALUES[idx], EPS);
        }
    }

    void TestCatboostCalculations() {
        TFormulasStorage storage;
        storage.AddFormula("info_cbm", "formulas/catboost.cbm");

        for (size_t idx = 0; idx < FLOAT_FORMULA_DATA_SIZE; ++idx) {
            double value = storage.GetFormulaValue("info_cbm", Data[idx]);
            UNIT_ASSERT_DOUBLES_EQUAL(value, INFO_VALUES[idx], EPS);
        }
    }

    void TestMultiCategCalculations() {
        TFormulasStorage storage;
        storage.AddFormula("mnmc", "formulas/mnmc/mnmc.mnmc");

        for (size_t idx = 0; idx < FLOAT_FORMULA_DATA_SIZE; ++idx) {
            double value = storage.GetFormulaValue("mnmc", Data[idx]);
            UNIT_ASSERT_DOUBLES_EQUAL(value, MNMC_VALUES[idx], EPS);
        }
    }

    void TestFormulaNames() {
        TVector<TString> names;
        TFormulasStorage storage;
        storage.AddFormula("f1", "formulas/info.info");
        storage.AddFormula("m1", "formulas/mnmc/mnmc.mnmc");
        storage.AddFormula("regtreeFml", "formulas/regtree/regtree.regtree");
        storage.GetDefinedFormulaNames(names, true);

        TVector<TString> namesTest = {"f1", "m1", "regtreeFml"};
        UNIT_ASSERT(names == namesTest);

        storage.AddFormula("a1", "formulas/info.info");
        storage.GetDefinedFormulaNames(names, true);
        namesTest.insert(namesTest.begin(), "a1");
        UNIT_ASSERT(names == namesTest);

        storage.AddFormula("l1", "formulas/mnmc/mnmc.mnmc");
        storage.GetDefinedFormulaNames(names, true);
        namesTest.insert(namesTest.begin() + 2, "l1");
        UNIT_ASSERT(names == namesTest);

        storage.AddFormula("z1", "formulas/regtree/regtree.regtree");
        storage.GetDefinedFormulaNames(names, true);
        namesTest.insert(namesTest.begin() + 5, "z1");
        UNIT_ASSERT(names == namesTest);

        storage.GetDefinedFormulaNames(names);
        UNIT_ASSERT_VALUES_EQUAL(names.size(), namesTest.size());
        for (size_t idx = 0; idx < names.size(); ++idx) {
            UNIT_ASSERT(IsIn(namesTest, names[idx]));
        }
    }

    void TestMultipleReturnValues() {
        TFormulasStorage storage;
        storage.AddFormula("info", "formulas/info.info");
        storage.AddFormula("mnmc", "formulas/mnmc/mnmc.mnmc");
        storage.AddFormula("regtree", "formulas/regtree/regtree.regtree");

        TVector<const float*> data;
        for (size_t idx = 0; idx < FLOAT_FORMULA_DATA_SIZE; ++idx) {
            data.push_back(Data[idx]);
        }

        TVector<double> res;
        storage.GetFormulaValues("info", data, res);
        UNIT_ASSERT_VALUES_EQUAL(res.size(), FLOAT_FORMULA_DATA_SIZE);
        for (size_t idx = 0; idx < FLOAT_FORMULA_DATA_SIZE; ++idx) {
            UNIT_ASSERT_DOUBLES_EQUAL(res[idx], INFO_VALUES[idx], EPS);
        }

        TVector<double> regTreeRes;
        storage.GetFormulaValues("regtree", data, regTreeRes);
        UNIT_ASSERT_VALUES_EQUAL(regTreeRes.size(), FLOAT_FORMULA_DATA_SIZE);
        for (size_t idx = 0; idx < FLOAT_FORMULA_DATA_SIZE; ++idx)
            UNIT_ASSERT_DOUBLES_EQUAL(regTreeRes[idx], REGTREE_VALUES[idx], EPS);

        storage.GetFormulaValues("mnmc", data, res);
        UNIT_ASSERT_VALUES_EQUAL(res.size(), FLOAT_FORMULA_DATA_SIZE);
        for (size_t idx = 0; idx < FLOAT_FORMULA_DATA_SIZE; ++idx) {
            UNIT_ASSERT_DOUBLES_EQUAL(res[idx], MNMC_VALUES[idx], EPS);
        }
    }

    void TestMultiCategCategsCalculations() {
        TFormulasStorage storage;
        storage.AddFormula("mnmc", "formulas/mnmc/mnmc.mnmc");

        TVector<double> categs;
        TVector<double> categValues;
        TVector<float> factors(Data[0], Data[0] + FLOAT_FORMULA_FACTORS_COUNT);

        storage.GetFormulaCategsByName("mnmc", factors, categs, categValues);
        UNIT_ASSERT_VALUES_EQUAL(categs.size(), CATEGS_NUMBER);
        UNIT_ASSERT_VALUES_EQUAL(categValues.size(), CATEGS_NUMBER);
        for (size_t idx = 0; idx < categs.size(); ++idx) {
            UNIT_ASSERT_DOUBLES_EQUAL(categs[idx], CATEGS[idx], EPS);
            UNIT_ASSERT_DOUBLES_EQUAL(categValues[idx], CATEGS_VALUES[idx], EPS);
        }

        double maxCategVal;
        storage.GetMaxCategValueByName("mnmc", factors, maxCategVal);
        UNIT_ASSERT_DOUBLES_EQUAL(maxCategVal, MAX_CATEG_VALUE, EPS);
    }

    void TestArchive() {
        TFormulasStorage storage;
        bool check = storage.AddFormulasFromArchive("formulas/archive/models.archive");
        UNIT_ASSERT(check);

        UNIT_ASSERT_EQUAL(storage.Size(), 2);
        UNIT_ASSERT(storage.CheckFormulaUsed("info"));
        UNIT_ASSERT(storage.CheckFormulaUsed("info_copy"));
        UNIT_ASSERT(!storage.CheckFormulaUsed("info_unused"));
        UNIT_ASSERT(storage.GetFormulaByName("info_copy") != nullptr);
        UNIT_ASSERT(storage.GetFormulaByName("info_unused") == nullptr);

        double value = storage.GetFormulaValue("info", Data[0]);
        UNIT_ASSERT_DOUBLES_EQUAL(value, INFO_VALUES[0], EPS);

        value = storage.GetFormulaValue("info_copy", Data[1]);
        UNIT_ASSERT_DOUBLES_EQUAL(value, INFO_VALUES[1], EPS);

        value = storage.GetFormulaValue("info", Data[2]);
        UNIT_ASSERT_DOUBLES_EQUAL(value, INFO_VALUES[2], EPS);

        double value2 = storage.GetFormulaValue("info_copy", Data[2]);
        UNIT_ASSERT_DOUBLES_EQUAL(value, value2, EPS);
    }

    void TestMnMCArchive() {
        TFormulasStorage storage;
        bool check = storage.AddFormulasFromArchive("formulas/archive/with_mnmc.archive");
        UNIT_ASSERT(check);

        UNIT_ASSERT_EQUAL(storage.Size(), 2);

        UNIT_ASSERT(storage.CheckFormulaUsed("mnmc"));

        TVector<float> factors(Data[0], Data[0] + FLOAT_FORMULA_FACTORS_COUNT);

        double maxCategVal(-1.);
        storage.GetMaxCategValueByName("mnmc", factors, maxCategVal);
        UNIT_ASSERT_DOUBLES_EQUAL(maxCategVal, MAX_CATEG_VALUE, EPS);
    }

    void TestErrorLogCompletion() {
        TFormulasStorage storage(true);
        storage.AddFormula("info", "formulas/info.info");
        storage.AddFormula("info", "formulas/info.info");

        const TString message = storage.GetErrorLog();
        UNIT_ASSERT(message.size());
        UNIT_ASSERT(message.find("info") != TString::npos);
        UNIT_ASSERT(message.find("formulas/info.info") != TString::npos);

        const TString sameMessage = storage.GetErrorLog(true);
        UNIT_ASSERT_EQUAL(message, sameMessage);

        const TString emptyMessage = storage.GetErrorLog();
        UNIT_ASSERT_EQUAL(emptyMessage.size(), 0);
    }

    void TestErrorLogException() {
        TFormulasStorage storage;
        storage.AddFormula("info", "formulas/info.info");
        storage.AddFormula("info", "formulas/info.info");

        UNIT_CHECK_GENERATED_EXCEPTION(storage.GetErrorLog(), yexception);
    }

    void TestMd5() {
        TFormulasStorage storage;
        storage.AddFormula("real_info", "formulas/info.info");
        storage.AddFormula("real_mnmc", "formulas/mnmc/mnmc.mnmc");
        storage.AddFormula("real_regtree", "formulas/regtree/regtree.regtree");
        storage.AddFormula("real_info_cbm", "formulas/catboost.cbm");
        storage.AddFormulasFromArchive("formulas/archive/models.archive");
        storage.WaitAsyncJobs();

        UNIT_ASSERT_VALUES_EQUAL(storage.GetFormulaMd5("info"), "b622bd379d9f18a02b78b2025786801a");
        UNIT_ASSERT_VALUES_EQUAL(storage.GetFormulaMd5("info_copy"), "b622bd379d9f18a02b78b2025786801a");
        UNIT_ASSERT_VALUES_EQUAL(storage.GetFormulaMd5("real_info"), "b622bd379d9f18a02b78b2025786801a");
        UNIT_ASSERT_VALUES_EQUAL(storage.GetFormulaMd5("real_mnmc"), "93072d46e4133d891402d5f7370262d2");
        UNIT_ASSERT_VALUES_EQUAL(storage.GetFormulaMd5("real_regtree"), "f7eaca5e8672d7698d8452667916293b");
        UNIT_ASSERT_VALUES_EQUAL(storage.GetFormulaMd5("real_info_cbm"), "c7c12628273937c5de903b1d2af352af");
    }

    void TestFmlIDStorage() {
        TFormulasStorage storage;

        storage.AddFormulasFromDirectoryRecursive("formulas");
        storage.Finalize();
        UNIT_ASSERT_VALUES_EQUAL(storage.Size(), 6);

        UNIT_ASSERT(storage.GetFormulaByFmlID("fml-mn-12345") == nullptr);
        UNIT_ASSERT(storage.GetFormulaByName("fml-mn-12345") == nullptr);
        UNIT_ASSERT(storage.GetFormula("fml-mn-12345") == nullptr);

        UNIT_ASSERT(storage.GetFormulaByFmlID("fml-mn-28465") != nullptr);
        UNIT_ASSERT(storage.GetFormulaByName("fml-mn-28465") == nullptr);
        UNIT_ASSERT(storage.GetFormula("fml-mn-28465") != nullptr);

        UNIT_ASSERT(storage.GetFormulaByFmlID("fml-mn-28465") == storage.GetFormula("from_fml"));
        UNIT_ASSERT(storage.GetFormula("fml-mn-28465") == storage.GetFormula("from_fml"));

        for (size_t idx = 0; idx < FLOAT_FORMULA_DATA_SIZE; ++idx) {
            double valueFromStorage = storage.GetFormulaValue("from_fml", FromFmlData[idx]);
            double valueFromFmlStorage = storage.GetFormula("fml-mn-28465")->DoCalcRelev(FromFmlData[idx]);
            UNIT_ASSERT_DOUBLES_EQUAL(valueFromStorage, valueFromFmlStorage, EPS);
            UNIT_ASSERT_DOUBLES_EQUAL(valueFromStorage, FROM_FML_VALUES[idx], EPS);
        }

        storage.RemoveFormula("info");
        UNIT_ASSERT_VALUES_EQUAL(storage.Size(), 5);

        UNIT_ASSERT(storage.GetFormulaByFmlID("fml-mn-28465") != nullptr);
        UNIT_ASSERT(storage.GetFormula("fml-mn-28465") != nullptr);

        UNIT_ASSERT(storage.GetFormulaByFmlID("fml-mn-28465") == storage.GetFormula("from_fml"));
        UNIT_ASSERT(storage.GetFormula("fml-mn-28465") == storage.GetFormula("from_fml"));

        storage.RemoveFormula("from_fml");
        UNIT_ASSERT_VALUES_EQUAL(storage.Size(), 4);

        UNIT_ASSERT(storage.GetFormulaByFmlID("fml-mn-28465") == nullptr);
    }
};

UNIT_TEST_SUITE_REGISTRATION(TFormulasStorageTest);
