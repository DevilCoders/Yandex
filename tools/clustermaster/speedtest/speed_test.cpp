#include "data.h"

#include <tools/clustermaster/master/http_common.h>
#include <tools/clustermaster/master/master_target_graph.h>

#include <library/cpp/archive/yarchive.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/ptr.h>
#include <util/memory/blob.h>
#include <util/stream/input.h>

namespace {
    struct TStaticArchive: public TArchiveReader {
        TStaticArchive()
            : TArchiveReader(TBlob::NoCopy(NSpeedTestData::GetData().data(), NSpeedTestData::GetData().size()))
        {
        }
    };
}

struct TSpeedTester {
    TMasterListManager ListManager;
    TMasterGraph Graph;

    TSpeedTester(const TString& name)
        : Graph(&ListManager, true)
    {
        TAutoPtr<IInputStream> script = Singleton<TStaticArchive>()->ObjectByKey("/" + name + ".sh");
        Graph.LoadConfig(TMasterConfigSource(script->ReadAll()), nullptr);
    }
};

Y_UNIT_TEST_SUITE(TCmSpeedTest) {
    template <typename T>
    void MeasureAllTasks(const TString& iteratorName, T iterator) {
        TStateStats stats;
        TInstant start = TInstant::Now();
        for (; iterator.Next(); ) {
            stats.Update(iterator->Status);
        }
        TDuration diff = TInstant::Now() - start;
        Cout << "Iterator " << iteratorName << " took " << diff << "\n";
    }

    Y_UNIT_TEST(AllTasksIterator) {
        TSpeedTester tester("500x500");

        UNIT_ASSERT_VALUES_EQUAL(size_t(500), tester.Graph.GetTargetTypeByName("HOSTShosts").GetHostList().Size());

        const TMasterTarget& target = tester.Graph.GetTargetByName("first");
        MeasureAllTasks("TaskIterator (old iterator)", target.TaskIterator());
        MeasureAllTasks("TaskFlatIterator (new iterator)", target.TaskFlatIterator());
    }

    Y_UNIT_TEST(ByHostIterator) {
        TSpeedTester tester("500x500");

        const TMasterTarget& target = tester.Graph.GetTargetByName("first");

        {
            TInstant start = TInstant::Now();
            TStateStats stats;
            for (TIdForString::TIterator i = target.Type->GetHostList().Iterator(); i.Next(); ) {
                for (TMasterTarget::TConstTaskIterator j = target.TaskIteratorForHost(i->Id); j.Next(); ) {
                    stats.Update(j->Status);
                }
            }
            TDuration diff = TInstant::Now() - start;
            Cout << "Iterator TaskIterator (old one) took " << diff << "\n";
        }

        {
            TInstant start = TInstant::Now();
            TStateStats stats;
            for (TIdForString::TIterator i = target.Type->GetHostList().Iterator(); i.Next(); ) {
                for (TMasterTarget::TFlatConstTaskIterator j = target.TaskFlatIteratorForHost(i->Id); j.Next(); ) {
                    stats.Update(j->Status);
                }
            }
            TDuration diff = TInstant::Now() - start;
            Cout << "Iterator TaskFlatIterator (new one) took " << diff << "\n";
        }
    }
}

Y_UNIT_TEST_SUITE(TCmUpdatePokeStateSpeedTest) {
    Y_UNIT_TEST(UpdatePokeStateTest) {
#ifndef NDEBUG
        Cout << "Use release build to estimate speedtest results properly" << Endl;
#endif
        Cout << "Preparing graph...\n";
        TSpeedTester tester("param_mapping");

        TMasterTarget &target = tester.Graph.GetTargetByName("second");
        UNIT_ASSERT_EQUAL(target.Depends.size(), 1);
        TMasterDepend *depend = &target.Depends[0];
        UNIT_ASSERT_EQUAL(depend->IsInvert(), false);
        THashSet<TTargetTypeParameters::TProjection> invalidate;

        Cout << "Ready for tests\n";
        {
            TInstant t_beg = TInstant::Now();
            for (TJoinEnumerator en(
                    &depend->GetSource()->Type->GetParameters(),
                    &depend->GetTarget()->Type->GetParameters(),
                    *depend->GetJoinParamMappings().Get());
                    en.Next(); ) {
                int counter = 0;
                for (TMasterTarget::TTaskIterator i = depend->GetTarget()->TaskIteratorForProjection(en.GetDepProjection()); ; counter++) {
                    if (!i.Next()) {
                        invalidate.insert(en.GetDepProjection());
                        break;
                    }
                }
            }
            TInstant t_end = TInstant::Now();
            TDuration duration = t_end - t_beg;
            Cout << "Updating poke state took: " << duration << "\n";
        }

        {
            TInstant t_beg = TInstant::Now();

            IPrecomputedTaskIdsInitializer* precomputed = depend->GetPrecomputedTaskIdsMaybe()->Get();
            // precomputed->Initialize();

            for (TDependEdgesEnumerator en(precomputed->GetIds()); en.Next(); ) {
                int counter = 0;
                for (TPrecomputedTaskIdsForOneSide::TIterator i = en.GetDepTaskIds().Begin(); i != en.GetDepTaskIds().End(); ++i) {
                    if (size_t(counter * 2) >= en.GetDepTaskIds().Length() - 1) {
                        break;
                    }
                }
            }

            TInstant t_end = TInstant::Now();
            TDuration duration = t_end - t_beg;
            Cout << "Updating poke state with precomputed data took: " << duration << "\n";
        }
    }
}
