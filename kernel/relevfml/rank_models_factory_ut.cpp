#include "rank_models_factory.h"
#include <library/cpp/testing/unittest/registar.h>

#include <util/stream/output.h>
#include <util/system/defaults.h>

#define LOCAL_ASSERT_MATRIXNET(MODEL, MN)  \
    UNIT_ASSERT_EQUAL(f.GetMatrixnet(MODEL), MN)

#define LOCAL_ASSERT_EMATRIXNET(MODEL, MN) \
    UNIT_ASSERT_EQUAL(f.GetMatrixnet(MODEL, exps), MN)

#define LOCAL_ASSERT_BUNDLE(NAME, MN) { \
    const auto b = f.GetBundle(NAME); \
    UNIT_ASSERT(b); \
    UNIT_ASSERT_EQUAL(b->Matrixnets.size(), 1); \
    UNIT_ASSERT_EQUAL(b->Matrixnets.front().Matrixnet, MN); \
}

#define LOCAL_ASSERT_EBUNDLE(NAME, MN) { \
    const auto b = f.GetBundle(NAME, exps); \
    UNIT_ASSERT(b); \
    UNIT_ASSERT_EQUAL(b->Matrixnets.size(), 1); \
    UNIT_ASSERT_EQUAL(b->Matrixnets.front().Matrixnet, MN); \
}


Y_UNIT_TEST_SUITE(TRankModelsMapFactoryTest) {

Y_UNIT_TEST(SimpleTest) {
    // level1:
    //   mn -> 123
    //   sub:
    //     mn -> 0
    //     sub:
    //       mn -> 321
    TRankModelsMapFactory f;
    // fake pointers
    NMatrixnet::TMnSsePtr mn123(new NMatrixnet::TMnSseDynamic());
    NMatrixnet::TMnSsePtr mn321(new NMatrixnet::TMnSseDynamic());
    const char *l1           = "level1";
    const char *l1__sub      = "level1.sub";
    const char *l1__sub__sub = "level1.sub.sub";
    f.SetMatrixnet(l1,           mn123);
    f.SetMatrixnet(l1__sub__sub, mn321);

    LOCAL_ASSERT_MATRIXNET(l1,           mn123);
    LOCAL_ASSERT_MATRIXNET(l1__sub,      mn123);
    LOCAL_ASSERT_MATRIXNET(l1__sub__sub, mn321);
}

Y_UNIT_TEST(SubTreeTest) {
    // level1:
    //   mn -> 123
    //   sub:
    //     mn -> 0
    //     sub:
    //       mn -> 321
    //
    TRankModelsMapFactory f;
    // fake matrixnet models
    NMatrixnet::TMnSsePtr mn123(new NMatrixnet::TMnSseDynamic());
    NMatrixnet::TMnSsePtr mn321(new NMatrixnet::TMnSseDynamic());
    const char *l1           = "level1";
    const char *l1__sub      = "level1.sub";
    const char *l1__sub__sub = "level1.sub.sub";
    f.SetMatrixnet(l1,           mn123);
    f.SetMatrixnet(l1__sub__sub, mn321);

    LOCAL_ASSERT_MATRIXNET(l1,                 mn123);
    LOCAL_ASSERT_MATRIXNET(l1__sub,            mn123);
    LOCAL_ASSERT_MATRIXNET(l1__sub__sub,       mn321);
    LOCAL_ASSERT_MATRIXNET("level1.sub:1",     mn123);
    LOCAL_ASSERT_MATRIXNET("level1.sub:3",     mn123);
    LOCAL_ASSERT_MATRIXNET("level1.sub.sub:1", mn321);
    LOCAL_ASSERT_MATRIXNET("level1.sub.sub:2", mn321);
    LOCAL_ASSERT_MATRIXNET("level1.sub.sub:3", mn321);
}

Y_UNIT_TEST(ExperimentsTest) {
    TRankModelsMapFactory f;
    // fake matrixnet models
    NMatrixnet::TMnSsePtr mn123(new NMatrixnet::TMnSseDynamic());
    NMatrixnet::TMnSsePtr mn333(new NMatrixnet::TMnSseDynamic());
    NMatrixnet::TMnSsePtr mn444(new NMatrixnet::TMnSseDynamic());


    const char *L1       = "level1";
    const char *L1Sub    = "level1.sub";
    const char *L1SubSub = "level1.sub.sub";
    const char *L1_e1_   = "level1:e1:";
    const char *L1Sub_e2e3_ = "level1.sub:e2.e3:";

    // level1:
    //   mn -> 123
    //   >e1:
    //     mn -> 333
    //   >e2:
    //     :2:
    //       mn -> 0
    //   sub:
    //     mn -> 0
    //     >e1
    //       1:
    //         mn -> 0
    //     >e2:
    //       mn -> 444
    //     >e3:
    //       mn -> 444
    //     sub:
    //       mn -> 0
    //
    f.SetMatrixnet(L1,          mn123);
    f.SetMatrixnet(L1_e1_,      mn333);
    f.SetMatrixnet(L1Sub_e2e3_, mn444);

    LOCAL_ASSERT_MATRIXNET(L1,                 mn123);
    LOCAL_ASSERT_MATRIXNET(L1Sub,              mn123);
    LOCAL_ASSERT_MATRIXNET(L1SubSub,           mn123);
    LOCAL_ASSERT_MATRIXNET("level1.sub:1",     mn123);
    LOCAL_ASSERT_MATRIXNET("level1.sub:3",     mn123);
    LOCAL_ASSERT_MATRIXNET("level1.sub.sub:1", mn123);
    LOCAL_ASSERT_MATRIXNET("level1.sub.sub:2", mn123);
    LOCAL_ASSERT_MATRIXNET("level1.sub.sub:3", mn123);
    LOCAL_ASSERT_MATRIXNET(L1_e1_,             mn333);

    TSet<TString> exps;
    exps.insert("e1");
    LOCAL_ASSERT_EMATRIXNET(L1,                 mn333);
    LOCAL_ASSERT_EMATRIXNET(L1Sub,              mn333);
    exps.insert("e2");
    LOCAL_ASSERT_EMATRIXNET(L1,                 mn333);
    LOCAL_ASSERT_EMATRIXNET(L1Sub,              mn444);
    exps.erase("e1");
    LOCAL_ASSERT_EMATRIXNET(L1,                 mn123);
    LOCAL_ASSERT_EMATRIXNET(L1Sub,              mn444);
    exps.insert("e3");
    LOCAL_ASSERT_EMATRIXNET(L1Sub,              mn444);
    exps.erase("e2");
    LOCAL_ASSERT_EMATRIXNET(L1Sub,              mn444);

}

Y_UNIT_TEST(BundleSimpleTest) {
    const auto mn1 = MakeAtomicShared<NMatrixnet::TMnSseDynamic>();
    const auto mn2 = MakeAtomicShared<NMatrixnet::TMnSseDynamic>();

    TRankModelsMapFactory f;
    f.SetMatrixnet("mn1", mn1);
    f.SetMatrixnet("mn2", mn2);

    const NMatrixnet::TBundleDescription bnd1{ NMatrixnet::TModelInfo(), { NMatrixnet::TBundleElement<TString>("mn1") }};
    const NMatrixnet::TBundleDescription bnd2{ NMatrixnet::TModelInfo(), { NMatrixnet::TBundleElement<TString>("mn2") }};

    const char *l1 = "level1";
    const char *l2 = "level1.level2";
    f.SetBundle(l1, bnd1);
    f.SetBundle(l2, bnd2);

    LOCAL_ASSERT_BUNDLE(l1, mn1);
    LOCAL_ASSERT_BUNDLE(l2, mn2);
}

Y_UNIT_TEST(BundleSubTreeTest) {
    const auto mn1 = MakeAtomicShared<NMatrixnet::TMnSseDynamic>();
    const auto mn2 = MakeAtomicShared<NMatrixnet::TMnSseDynamic>();

    TRankModelsMapFactory f;
    f.SetMatrixnet("mn1", mn1);
    f.SetMatrixnet("mn2", mn2);

    const NMatrixnet::TBundleDescription bnd1{ NMatrixnet::TModelInfo(), { NMatrixnet::TBundleElement<TString>("mn1") }};
    const NMatrixnet::TBundleDescription bnd2{ NMatrixnet::TModelInfo(), { NMatrixnet::TBundleElement<TString>("mn2") }};

    const char *l1           = "level1";
    const char *l1__sub      = "level1.sub";
    const char *l1__sub__sub = "level1.sub.sub";
    f.SetBundle(l1,           bnd1);
    f.SetBundle(l1__sub__sub, bnd2);

    LOCAL_ASSERT_BUNDLE(l1,                 mn1);
    LOCAL_ASSERT_BUNDLE(l1__sub,            mn1);
    LOCAL_ASSERT_BUNDLE(l1__sub__sub,       mn2);
    LOCAL_ASSERT_BUNDLE("level1.sub:1",     mn1);
    LOCAL_ASSERT_BUNDLE("level1.sub:3",     mn1);
    LOCAL_ASSERT_BUNDLE("level1.sub.sub:1", mn2);
    LOCAL_ASSERT_BUNDLE("level1.sub.sub:2", mn2);
    LOCAL_ASSERT_BUNDLE("level1.sub.sub:3", mn2);
}

Y_UNIT_TEST(BundleExperimentsTest) {
    const auto mn1 = MakeAtomicShared<NMatrixnet::TMnSseDynamic>();
    const auto mn2 = MakeAtomicShared<NMatrixnet::TMnSseDynamic>();
    const auto mn3 = MakeAtomicShared<NMatrixnet::TMnSseDynamic>();
    const auto mn4 = MakeAtomicShared<NMatrixnet::TMnSseDynamic>();
    const auto mn4e2 = MakeAtomicShared<NMatrixnet::TMnSseDynamic>();

    TRankModelsMapFactory f;
    f.SetMatrixnet("mn1", mn1);
    f.SetMatrixnet("mn2", mn2);
    f.SetMatrixnet("mn3", mn3);
    f.SetMatrixnet("mn4", mn4);
    f.SetMatrixnet("mn4:e2:", mn4e2);

    const NMatrixnet::TBundleDescription bnd1{ NMatrixnet::TModelInfo(), { NMatrixnet::TBundleElement<TString>("mn1") }};
    const NMatrixnet::TBundleDescription bnd2{ NMatrixnet::TModelInfo(), { NMatrixnet::TBundleElement<TString>("mn2") }};
    const NMatrixnet::TBundleDescription bnd3{ NMatrixnet::TModelInfo(), { NMatrixnet::TBundleElement<TString>("mn3") }};
    const NMatrixnet::TBundleDescription bnd4{ NMatrixnet::TModelInfo(), { NMatrixnet::TBundleElement<TString>("mn4") }};

    const char *L1       = "level1";
    const char *L1Sub    = "level1.sub";
    const char *L1SubSub = "level1.sub.sub";
    const char *L1_e1_   = "level1:e1:";
    const char *L1Sub_e2e3_ = "level1.sub:e2.e3:";

    f.SetBundle(L1,          bnd1);
    f.SetBundle(L1_e1_,      bnd3);
    f.SetBundle(L1Sub_e2e3_, bnd4);

    LOCAL_ASSERT_BUNDLE(L1,                 mn1);
    LOCAL_ASSERT_BUNDLE(L1Sub,              mn1);
    LOCAL_ASSERT_BUNDLE(L1SubSub,           mn1);
    LOCAL_ASSERT_BUNDLE("level1.sub:1",     mn1);
    LOCAL_ASSERT_BUNDLE("level1.sub:3",     mn1);
    LOCAL_ASSERT_BUNDLE("level1.sub.sub:1", mn1);
    LOCAL_ASSERT_BUNDLE("level1.sub.sub:2", mn1);
    LOCAL_ASSERT_BUNDLE("level1.sub.sub:3", mn1);

    TSet<TString> exps;
    exps.insert("e1");
    LOCAL_ASSERT_EBUNDLE(L1,    mn3);
    LOCAL_ASSERT_EBUNDLE(L1Sub, mn3);
    exps.insert("e2");
    LOCAL_ASSERT_EBUNDLE(L1,    mn3);
    LOCAL_ASSERT_EBUNDLE(L1Sub, mn4e2);
    exps.erase("e1");
    LOCAL_ASSERT_EBUNDLE(L1,    mn1);
    LOCAL_ASSERT_EBUNDLE(L1Sub, mn4e2);
    exps.insert("e3");
    LOCAL_ASSERT_EBUNDLE(L1Sub, mn4e2);
    exps.erase("e2");
    LOCAL_ASSERT_EBUNDLE(L1Sub, mn4);
}

Y_UNIT_TEST(BundleInBundleTest) {
    const auto mn1 = MakeAtomicShared<NMatrixnet::TMnSseDynamic>();
    const auto mn2 = MakeAtomicShared<NMatrixnet::TMnSseDynamic>();
    const NMatrixnet::TBundleDescription bnd1 = { NMatrixnet::TModelInfo(),
        { NMatrixnet::TBundleElement<TString>("mn1", {1.0, 10.0}),
        NMatrixnet::TBundleElement<TString>("mn2", {0.5, 0.0}) }};

    const auto mn3 = MakeAtomicShared<NMatrixnet::TMnSseDynamic>();
    const NMatrixnet::TBundleDescription bnd2 = { NMatrixnet::TModelInfo(),
        { NMatrixnet::TBundleElement<TString>("bnd1", {2.0, 100.0}),
        NMatrixnet::TBundleElement<TString>("mn3", {2.0, 0.0}) }};

    const auto mn4 = MakeAtomicShared<NMatrixnet::TMnSseDynamic>();
    const NMatrixnet::TBundleDescription bnd3 = { NMatrixnet::TModelInfo(),
        { NMatrixnet::TBundleElement<TString>("bnd1", {3.0, 0.0}),
        NMatrixnet::TBundleElement<TString>("mn4", {3.0, 0.0}) }};

    const NMatrixnet::TBundleDescription bnd4 = { NMatrixnet::TModelInfo(),
        { NMatrixnet::TBundleElement<TString>("bnd2", {4.0, 0.0}),
        NMatrixnet::TBundleElement<TString>("bnd3", {4.0, 0.0}) }};

    TRankModelsMapFactory f;
    f.SetMatrixnet("mn1", mn1);
    f.SetMatrixnet("mn2", mn2);
    f.SetMatrixnet("mn3", mn3);
    f.SetMatrixnet("mn4", mn4);
    f.SetBundle("bnd1", bnd1);
    f.SetBundle("bnd2", bnd2);
    f.SetBundle("bnd3", bnd3);
    f.SetBundle("bnd4", bnd4);

    const auto bundle = f.GetBundle("bnd4");
    UNIT_ASSERT(bundle);
    UNIT_ASSERT_EQUAL(bundle->Matrixnets.size(), 4);

    THashMap<const NMatrixnet::TMnSseInfo*, NMatrixnet::TBundleRenorm> expected;
    expected[mn1.Get()] = {4.0 * 2.0 * 1.0 + 4.0 * 3.0 * 1.0, 600.0};
    expected[mn2.Get()] = {4.0 * 2.0 * 0.5 + 4.0 * 3.0 * 0.5, 400.0};
    expected[mn3.Get()] = {4.0 * 2.0, 0.0};
    expected[mn4.Get()] = {4.0 * 3.0, 0.0};

    THashSet<const NMatrixnet::TMnSseInfo*> visited;
    for (const auto& element : bundle->Matrixnets) {
        UNIT_ASSERT(!visited.contains(element.Matrixnet.Get()));
        visited.insert(element.Matrixnet.Get());
        const auto expectedValue = expected.FindPtr(element.Matrixnet.Get());
        UNIT_ASSERT(expectedValue);
        UNIT_ASSERT_VALUES_EQUAL(*expectedValue, element.Renorm);
    }
    UNIT_ASSERT_EQUAL(expected.size(), visited.size());
}

Y_UNIT_TEST(BundleInBundleCycleTest) {
    const auto mn1 = MakeAtomicShared<NMatrixnet::TMnSseDynamic>();
    const auto mn2 = MakeAtomicShared<NMatrixnet::TMnSseDynamic>();
    const NMatrixnet::TBundleDescription bnd1 = { NMatrixnet::TModelInfo(),
        { NMatrixnet::TBundleElement<TString>("mn1", {1.0, 0.0}),
        NMatrixnet::TBundleElement<TString>("bnd4", {0.5, 0.0}) }};

    const auto mn3 = MakeAtomicShared<NMatrixnet::TMnSseDynamic>();
    const NMatrixnet::TBundleDescription bnd2 = { NMatrixnet::TModelInfo(),
        { NMatrixnet::TBundleElement<TString>("bnd1", {2.0, 0.0}),
        NMatrixnet::TBundleElement<TString>("mn3", {2.0, 0.0}) }};

    const auto mn4 = MakeAtomicShared<NMatrixnet::TMnSseDynamic>();
    const NMatrixnet::TBundleDescription bnd3 = { NMatrixnet::TModelInfo(),
        { NMatrixnet::TBundleElement<TString>("bnd1", {3.0, 0.0}),
        NMatrixnet::TBundleElement<TString>("mn4", {3.0, 0.0}) }};

    const NMatrixnet::TBundleDescription bnd4 = { NMatrixnet::TModelInfo(),
        { NMatrixnet::TBundleElement<TString>("bnd2", {4.0, 0.0}),
        NMatrixnet::TBundleElement<TString>("bnd3", {4.0, 0.0}) }};

    TRankModelsMapFactory f;
    f.SetMatrixnet("mn1", mn1);
    f.SetMatrixnet("mn2", mn2);
    f.SetMatrixnet("mn3", mn3);
    f.SetMatrixnet("mn4", mn4);
    f.SetBundle("bnd1", bnd1);
    f.SetBundle("bnd2", bnd2);
    f.SetBundle("bnd3", bnd3);
    f.SetBundle("bnd4", bnd4);

    UNIT_ASSERT_EXCEPTION(f.GetBundle("bnd4"), yexception);
}

Y_UNIT_TEST(BundleIdTest) {
    const auto mn1 = MakeAtomicShared<NMatrixnet::TMnSseDynamic>();
    mn1->Info["formula-id"] = "fml-mn-111744";
    const auto mn2 = MakeAtomicShared<NMatrixnet::TMnSseDynamic>();
    mn2->Info["formula-id"] = "fml-mn-62220";
    const auto mn3 = MakeAtomicShared<NMatrixnet::TMnSseDynamic>();
    mn3->Info["formula-id"] = "fml-mn-78365";

    TRankModelsMapFactory f;
    f.SetMatrixnet("full.ru", mn1);
    f.SetMatrixnet("full.ru.commercial:mn62220:", mn2);
    f.SetMatrixnet("full.ru.commercial:mn78365:", mn3);

    const NMatrixnet::TBundleDescription description = {
        NMatrixnet::TModelInfo(),
        {
            NMatrixnet::TBundleElement<TString>("full.ru", {0.94, 0.0}),
            NMatrixnet::TBundleElement<TString>("@fml-mn-62220", {0.01, 0.0}),
            NMatrixnet::TBundleElement<TString>("@fml-mn-78365", {0.05, 0.0})
        }
    };

    f.SetBundle("full.ru.commercial", description);

    const auto bundle = f.GetBundle("full.ru.commercial");
    UNIT_ASSERT(bundle);
    UNIT_ASSERT_EQUAL(bundle->Matrixnets.size(), 3);
    UNIT_ASSERT_EQUAL(bundle->Matrixnets[0].Matrixnet, mn1);
    UNIT_ASSERT_EQUAL(bundle->Matrixnets[0].Renorm, NMatrixnet::TBundleRenorm(0.94, 0.0));
    UNIT_ASSERT_EQUAL(bundle->Matrixnets[1].Matrixnet, mn2);
    UNIT_ASSERT_EQUAL(bundle->Matrixnets[1].Renorm, NMatrixnet::TBundleRenorm(0.01, 0.0));
    UNIT_ASSERT_EQUAL(bundle->Matrixnets[2].Matrixnet, mn3);
    UNIT_ASSERT_EQUAL(bundle->Matrixnets[2].Renorm, NMatrixnet::TBundleRenorm(0.05, 0.0));
}

Y_UNIT_TEST(GetModelTest) {
    const auto mn1 = MakeAtomicShared<NMatrixnet::TMnSseDynamic>();
    const NMatrixnet::TBundleDescription description1 = { NMatrixnet::TModelInfo(),
        { NMatrixnet::TBundleElement<TString>("full.ru", {1.0, 0.0}) }};

    TRankModelsMapFactory f;
    f.SetMatrixnet("full.ru", mn1);
    f.SetBundle("full.ru.regional", description1);

    LOCAL_ASSERT_MATRIXNET("full.ru", mn1);
    LOCAL_ASSERT_MATRIXNET("full.ru.regional", mn1);
    LOCAL_ASSERT_MATRIXNET("full.ru.regional.moscow", mn1);

    const auto bundle1 = f.GetBundle("full.ru");
    UNIT_ASSERT(!bundle1);

    const auto bundle2 = f.GetBundle("full.ru.regional");
    UNIT_ASSERT(bundle2);
    UNIT_ASSERT_EQUAL(bundle2->Matrixnets.size(), 1);
    UNIT_ASSERT_EQUAL(bundle2->Matrixnets.front().Matrixnet, mn1);

    const auto bundle3 = f.GetBundle("full.ru.regional.moscow");
    UNIT_ASSERT(bundle3);
    UNIT_ASSERT_EQUAL(bundle2->Matrixnets, bundle3->Matrixnets);

    const auto model1 = f.GetModel("full.ru");
    UNIT_ASSERT_EQUAL(model1, mn1);

    const auto model2 = f.GetModel("full.ru.regional");
    UNIT_ASSERT(model2);
    const auto model2bundle = dynamic_cast<const NMatrixnet::TBundle*>(model2.Get());
    UNIT_ASSERT(model2bundle);
    UNIT_ASSERT_EQUAL(bundle2->Matrixnets, model2bundle->Matrixnets);

    const auto model3 = f.GetModel("full.ru.regional.moscow");
    UNIT_ASSERT(model3);
    const auto model3bundle = dynamic_cast<const NMatrixnet::TBundle*>(model3.Get());
    UNIT_ASSERT(model3bundle);
    UNIT_ASSERT_EQUAL(bundle3->Matrixnets, model3bundle->Matrixnets);
}

};

#undef LOCAL_ASSERT_EBUNDLE
#undef LOCAL_ASSERT_BUNDLE
#undef LOCAL_ASSERT_EMATRIXNET
#undef LOCAL_ASSERT_MATRIXNET
