#include "param_mapping.h"

#include <tools/clustermaster/common/make_vector.h>

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(TJoinEnumeratorTest) {
    Y_UNIT_TEST(Test0) {
        TParamListManager listManager;
        listManager.GetOrCreateList(MakeVector<TString>("aaa", "bbb", "ccc"));
        listManager.GetOrCreateList(MakeVector<TString>("000", "111", "222"));

        TTargetTypeParameters depParameters("first", &listManager,
                listManager.GetOrCreateLists(MakeVector<TVector<TString> >(MakeVector<TString>("aaa", "bbb", "ccc"))));
        TTargetTypeParameters myParameters("second", &listManager,
                listManager.GetOrCreateLists(MakeVector<TVector<TString> >(MakeVector<TString>("aaa", "bbb", "ccc"))));

        depParameters.CompleteBuild();
        myParameters.CompleteBuild();

        TParamMappings mappings(MakeVector<TParamMapping>());

        TJoinEnumerator en(&myParameters, &depParameters, mappings);

        UNIT_ASSERT(en.Next());
        UNIT_ASSERT(!en.Next());
    }

    Y_UNIT_TEST(Test1) {
        TParamListManager listManager;
        listManager.GetOrCreateList(MakeVector<TString>("000", "111", "222"));
        TParamListManager::TListReference listId = listManager.GetOrCreateList(
                MakeVector<TString>("aaa", "bbb", "ccc", "ddd"));
        TIdForString list = listManager.GetList(listId);
        listManager.GetOrCreateList(MakeVector<TString>("xxx", "yyy"));

        TTargetTypeParameters depParameters("first", &listManager,
                listManager.GetOrCreateLists(MakeVector<TVector<TString> >(
                        MakeVector<TString>("aaa", "bbb", "ccc", "ddd"),
                        MakeVector<TString>("aaa", "bbb", "ccc", "ddd")
                        )));

        TTargetTypeParameters myParameters("second", &listManager,
                listManager.GetOrCreateLists(MakeVector<TVector<TString> >(
                        MakeVector<TString>("aaa", "bbb", "ccc", "ddd"),
                        MakeVector<TString>("000", "111", "222")
                        )));

        myParameters.CompleteBuild();
        depParameters.CompleteBuild();

        TParamMappings mappings(
                MakeVector<TParamMapping>(
                        TParamMapping(myParameters.GetLevelId(1), depParameters.GetLevelId(2))));

        TJoinEnumerator en(&myParameters, &depParameters, mappings);

        for (ui32 i = 0; i < list.Size(); ++i) {
            UNIT_ASSERT(en.Next());
            TTargetTypeParameters::TProjection myProjection = en.GetMyProjection();
            TTargetTypeParameters::TProjection depProjection = en.GetDepProjection();

            UNIT_ASSERT_VALUES_EQUAL(list.GetSafeId(i), *myProjection.GetBindingAtLevel(myParameters.GetLevelId(1)));
            UNIT_ASSERT(!myProjection.GetBindingAtLevel(myParameters.GetLevelId(2)));

            UNIT_ASSERT(!depProjection.GetBindingAtLevel(myParameters.GetLevelId(1)));
            UNIT_ASSERT_VALUES_EQUAL(list.GetSafeId(i), *depProjection.GetBindingAtLevel(myParameters.GetLevelId(2)));
        }

        UNIT_ASSERT(!en.Next());
    }

    Y_UNIT_TEST(Test2) {
        TParamListManager listManager;

        TTargetTypeParameters depParameters("first", &listManager,
                listManager.GetOrCreateLists(MakeVector<TVector<TString> >(
                        MakeVector<TString>("aaa", "bbb", "ccc", "ddd"),
                        MakeVector<TString>("000", "111", "222"),
                        MakeVector<TString>("xxx", "yyy")
                        )));

        TTargetTypeParameters myParameters("second", &listManager,
                listManager.GetOrCreateLists(MakeVector<TVector<TString> >(
                        MakeVector<TString>("xxx", "yyy"),
                        MakeVector<TString>("000", "111", "222")
                        )));

        depParameters.CompleteBuild();
        myParameters.CompleteBuild();

        TParamMappings mappings(
                MakeVector<TParamMapping>(
                        TParamMapping(myParameters.GetLevelId(1), depParameters.GetLevelId(3)),
                        TParamMapping(myParameters.GetLevelId(2), depParameters.GetLevelId(2))));

        TJoinEnumerator en(&myParameters, &depParameters, mappings);

        for (ui32 i = 0; i < 6; ++i) {
            UNIT_ASSERT(en.Next());
        }

        UNIT_ASSERT(!en.Next());
    }

}

