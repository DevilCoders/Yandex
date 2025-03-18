// must be included before library/cpp/testing/unittest,
// otherwise UNIT_ASSERT_VALUES_EQUAL won't work

#include "make_vector.h"
#include "target_type_parameters.h"

#include <tools/clustermaster/common/vector_to_string.h>

#include <library/cpp/containers/sorted_vector/sorted_vector.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/hash_set.h>
#include <util/generic/vector.h>
#include <util/random/mersenne.h>

Y_UNIT_TEST_SUITE(TTargetTypeParametersCountTest) {
    Y_UNIT_TEST(TestDepth0) {

        for (ui32 i = 0; i <= 2; ++i) {

            TParamListManager paramListManager;

            TTargetTypeParameters parameters(
                    "d0",
                    &paramListManager,
                    MakeVector<TParamListManager::TListReference>());

            if (i >= 1) {
                parameters.AddTask(MakeVector<TString>());
            }

            if (i == 2) {
                UNIT_ASSERT_EXCEPTION(parameters.AddTask(MakeVector<TString>()), yexception);
                continue;
            }

            parameters.CompleteBuild();

            UNIT_ASSERT_VALUES_EQUAL(ui32(i), parameters.GetCount());

            TTargetTypeParameters::TIterator it = parameters.Iterator();

            if (i >= 1) {
                UNIT_ASSERT(it.Next());
                UNIT_ASSERT_EQUAL(MakeVector<TString>(), it.CurrentPath());
            }

            UNIT_ASSERT(!it.Next());
        }
    }

    Y_UNIT_TEST(TestDepth1) {
        for (ui32 i = 0; i <= 3; ++i) {
            TParamListManager paramListManager;

            TTargetTypeParameters parameters(
                    "d1",
                    &paramListManager,
                    paramListManager.GetOrCreateLists(MakeVector<TVector<TString> >(
                            MakeVector<TString>("aaa", "bbb"))));

            if (i >= 1) {
                parameters.AddTask(MakeVector<TString>("aaa"));
            }
            if (i >= 2) {
                parameters.AddTask(MakeVector<TString>("bbb"));
            }
            if (i == 3) {
                UNIT_ASSERT_EXCEPTION(parameters.AddTask(MakeVector<TString>("aaa")), yexception);
                continue;
            }

            parameters.CompleteBuild();

            UNIT_ASSERT_VALUES_EQUAL(ui32(i), parameters.GetCount());

            TTargetTypeParameters::TIterator it = parameters.Iterator();

            if (i >= 1) {
                UNIT_ASSERT(it.Next());
                UNIT_ASSERT_EQUAL(MakeVector<TString>("aaa"), it.CurrentPath());
            }
            if (i >= 2) {
                UNIT_ASSERT(it.Next());
                UNIT_ASSERT_EQUAL(MakeVector<TString>("bbb"), it.CurrentPath());
            }

            UNIT_ASSERT(!it.Next());
        }
    }

    Y_UNIT_TEST(TestDepth2) {
        for (ui32 i = 0; i <= 4; ++i) {
            TParamListManager paramListManager;

            TTargetTypeParameters parameters(
                    "d2",
                    &paramListManager,
                    paramListManager.GetOrCreateLists(MakeVector<TVector<TString> >(
                        MakeVector<TString>("aaa", "bbb"),
                        MakeVector<TString>("bbb", "aaa"))));

            if (i >= 1) {
                parameters.AddTask(MakeVector<TString>("aaa", "bbb"));
            }
            if (i >= 2) {
                parameters.AddTask(MakeVector<TString>("aaa", "aaa"));
            }
            if (i >= 3) {
                parameters.AddTask(MakeVector<TString>("bbb", "aaa"));
            }
            if (i == 4) {
                UNIT_ASSERT_EXCEPTION(parameters.AddTask(MakeVector<TString>("aaa", "bbb")), yexception);
                continue;
            }

            parameters.CompleteBuild();

            UNIT_ASSERT_VALUES_EQUAL(ui32(i), parameters.GetCount());

            TTargetTypeParameters::TIterator it = parameters.Iterator();

            if (i >= 1) {
                UNIT_ASSERT(it.Next());
                UNIT_ASSERT_EQUAL(MakeVector<TString>("aaa", "bbb"), it.CurrentPath());
            }
            if (i >= 2) {
                UNIT_ASSERT(it.Next());
                UNIT_ASSERT_EQUAL(MakeVector<TString>("aaa", "aaa"), it.CurrentPath());
            }
            if (i >= 3) {
                UNIT_ASSERT(it.Next());
                UNIT_ASSERT_EQUAL(MakeVector<TString>("bbb", "aaa"), it.CurrentPath());
            }


            UNIT_ASSERT(!it.Next());
        }
    }

    Y_UNIT_TEST(TestDepthRandom) {
        TVector<TString> randomStrings;
        for (char c = 'a'; c <= 'z'; ++c) {
            TString s;
            s += c;
            s += c;
            s += c;
            randomStrings.push_back(s);
        }

        TMersenne<ui32> random;

        for (ui32 i = 0; i < 200; ++i) {
            ui32 depth = 1 + random.GenRand() % 4;

            TVector<TVector<TString> > paths;

            {
                THashSet<int> pathsSet1;


                THashSet<TVector<TString>, TSimpleRangeHash> pathsSet;
                for (ui32 j = 0; j < 20; ++j) {
                    for (;;) {
                        TVector<TString> path;
                        for (ui32 k = 0; k < depth; ++k) {
                            path.push_back(randomStrings.at(random.GenRand() % randomStrings.size()));
                        }

                        if (!pathsSet.insert(path).second) {
                            continue;
                        }

                        paths.push_back(path);

                        break;
                    }
                }
            }

            TVector<TVector<TString> > paramNames;
            {

                TVector<NSorted::TSortedVector<TString> > paramNamesUnique;
                paramNamesUnique.resize(depth);
                for (TVector<TVector<TString> >::const_iterator it = paths.begin(); it != paths.end(); ++it) {
                    for (size_t j = 0; j < depth; ++j) {
                        paramNamesUnique.at(j).InsertUnique(it->at(j));
                    }
                }

                for (TVector<NSorted::TSortedVector<TString> >::const_iterator it = paramNamesUnique.begin();
                        it != paramNamesUnique.end(); ++it)
                {
                    paramNames.push_back(*it);
                }
            }

            TParamListManager paramListManager;

            TTargetTypeParameters parameters(
                    "dr",
                    &paramListManager,
                    paramListManager.GetOrCreateLists(paramNames));

            for (TVector<TVector<TString> >::const_iterator it = paths.begin(); it != paths.end(); ++it) {
                parameters.AddTask(*it);
            }

            parameters.CompleteBuild();

            UNIT_ASSERT_VALUES_EQUAL(size_t(20), parameters.GetCount());

            TTargetTypeParameters::TIterator it = parameters.Iterator();

            for (ui32 j = 0; j < 20; ++j) {
                UNIT_ASSERT(it.Next());
            }

            UNIT_ASSERT(!it.Next());
        }
    }

    Y_UNIT_TEST(TestIteratorParamFixed) {
        TParamListManager paramListManager;

        TTargetTypeParameters parameters(
                "dpf",
                &paramListManager,
                paramListManager.GetOrCreateLists(MakeVector<TVector<TString> >(
                    MakeVector<TString>("aaa", "eee", "jjj", "lll"),
                    MakeVector<TString>("bbb", "fff", "hhh"),
                    MakeVector<TString>("ccc", "ddd", "ggg", "iii", "kkk"))));

        parameters.AddTask(MakeVector<TString>("aaa", "bbb", "ccc"));
        parameters.AddTask(MakeVector<TString>("aaa", "bbb", "ddd"));
        parameters.AddTask(MakeVector<TString>("eee", "fff", "ggg"));
        parameters.AddTask(MakeVector<TString>("eee", "hhh", "iii"));
        parameters.AddTask(MakeVector<TString>("jjj", "bbb", "ccc"));
        parameters.AddTask(MakeVector<TString>("jjj", "bbb", "kkk"));
        parameters.AddTask(MakeVector<TString>("lll", "fff", "ggg"));

        parameters.CompleteBuild();

        TTargetTypeParameters::TIterator it = parameters.IteratorParamFixed(2, "bbb");

        UNIT_ASSERT(it.Next());
        UNIT_ASSERT_VALUES_EQUAL(MakeVector<TString>("aaa", "bbb", "ccc"), it.CurrentPath());
        UNIT_ASSERT_VALUES_EQUAL(ui32(0), it.CurrentN().GetN());
        UNIT_ASSERT(it.Next());
        UNIT_ASSERT_VALUES_EQUAL(MakeVector<TString>("aaa", "bbb", "ddd"), it.CurrentPath());
        UNIT_ASSERT_VALUES_EQUAL(ui32(1), it.CurrentN().GetN());
        UNIT_ASSERT(it.Next());
        UNIT_ASSERT_VALUES_EQUAL(MakeVector<TString>("jjj", "bbb", "ccc"), it.CurrentPath());
        UNIT_ASSERT_VALUES_EQUAL(ui32(4), it.CurrentN().GetN());
        UNIT_ASSERT(it.Next());
        UNIT_ASSERT_VALUES_EQUAL(MakeVector<TString>("jjj", "bbb", "kkk"), it.CurrentPath());
        UNIT_ASSERT_VALUES_EQUAL(ui32(5), it.CurrentN().GetN());
        UNIT_ASSERT(!it.Next());

        UNIT_ASSERT_VALUES_EQUAL(MakeVector<TString>("jjj", "bbb", "ccc"), parameters.GetPathForN(4));
    }

    Y_UNIT_TEST(TestIteratorParamFixedDepth1) {
        TParamListManager paramListManager;

        TTargetTypeParameters parameters(
                "dpf",
                &paramListManager,
                paramListManager.GetOrCreateLists(MakeVector<TVector<TString> >(
                        MakeVector<TString>("aaa", "bbb", "ccc"))));

        parameters.AddTask(MakeVector<TString>("aaa"));
        parameters.AddTask(MakeVector<TString>("bbb"));
        parameters.AddTask(MakeVector<TString>("ccc"));

        parameters.CompleteBuild();

        TTargetTypeParameters::TIterator it = parameters.IteratorParamFixed(1, "bbb");
        UNIT_ASSERT(it.Next());
        UNIT_ASSERT_VALUES_EQUAL(MakeVector<TString>("bbb"), it.CurrentPath());
        UNIT_ASSERT(!it.Next());
    }

    Y_UNIT_TEST(TestGetCountAndGetCountAtFirstLevel) {
        TParamListManager paramListManager;

        TTargetTypeParameters parameters(
                "asdf",
                &paramListManager,
                paramListManager.GetOrCreateLists(MakeVector<TVector<TString> >(
                    MakeVector<TString>("aaa", "bbb", "ccc"),
                    MakeVector<TString>("ddd", "eee", "fff"),
                    MakeVector<TString>("ggg", "hhh", "iii", "jjj"))
                ));

        parameters.AddTask(MakeVector<TString>("aaa", "ddd", "ggg"));
        parameters.AddTask(MakeVector<TString>("aaa", "eee", "hhh"));
        parameters.AddTask(MakeVector<TString>("bbb", "ddd", "hhh"));
        parameters.AddTask(MakeVector<TString>("bbb", "ddd", "iii"));
        parameters.AddTask(MakeVector<TString>("bbb", "fff", "jjj"));
        parameters.AddTask(MakeVector<TString>("ccc", "eee", "ggg"));

        parameters.CompleteBuild();

        UNIT_ASSERT_VALUES_EQUAL(ui32(6), parameters.GetCount());
        UNIT_ASSERT_VALUES_EQUAL(ui32(2), parameters.GetCountFirstLevelFixed(parameters.GetParamIdAtLevel(1, "aaa")));
        UNIT_ASSERT_VALUES_EQUAL(ui32(1), parameters.GetCountFirstLevelFixed(parameters.GetParamIdAtLevel(1, "ccc")));

        TTargetTypeParameters parameters2(
                "ghjk",
                &paramListManager,
                paramListManager.GetOrCreateLists(MakeVector<TVector<TString> >(MakeVector<TString>("aaa", "bbb", "ccc")))
                );
        parameters2.AddTask(MakeVector<TString>("aaa"));
        parameters2.AddTask(MakeVector<TString>("bbb"));

        parameters2.CompleteBuild();

        UNIT_ASSERT_VALUES_EQUAL(ui32(2), parameters2.GetCount());
        UNIT_ASSERT_VALUES_EQUAL(ui32(1), parameters2.GetCountFirstLevelFixed(parameters2.GetParamIdAtLevel(1, "aaa")));
        UNIT_ASSERT_VALUES_EQUAL(ui32(1), parameters2.GetCountFirstLevelFixed(parameters2.GetParamIdAtLevel(1, "bbb")));

        TTargetTypeParameters empty1(
                "zxc",
                &paramListManager,
                paramListManager.GetOrCreateLists(MakeVector<TVector<TString> >())
                );

        empty1.CompleteBuild();

        UNIT_ASSERT_VALUES_EQUAL(ui32(0), empty1.GetCount());
        TIdForString::TIdSafe fakeId(0, 0);
        UNIT_ASSERT_VALUES_EQUAL(ui32(0), empty1.GetCountFirstLevelFixed(fakeId));

        TParamListManager paramListManager2;

        TVector<TParamListManager::TListReference> lists = paramListManager2.GetOrCreateLists(MakeVector<TVector<TString> >(
            MakeVector<TString>("aaa", "bbb"),
            MakeVector<TString>("ccc", "ddd"))
        );

        TTargetTypeParameters empty2("vbn", &paramListManager2, lists);

        empty2.CompleteBuild();

        UNIT_ASSERT_VALUES_EQUAL(ui32(0), empty2.GetCount());

        TIdForString::TIdSafe fakeId2(lists.at(0).N, 0);
        UNIT_ASSERT_VALUES_EQUAL(ui32(0), empty2.GetCountFirstLevelFixed(fakeId2));
    }
}
