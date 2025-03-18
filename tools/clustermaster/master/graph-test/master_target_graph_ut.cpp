// include before unittest

#include "data.h"

#include <tools/clustermaster/master/master_target.h>
#include <tools/clustermaster/master/master_target_graph.h>

#include <tools/clustermaster/common/make_vector.h>
#include <tools/clustermaster/common/vector_to_string.h>

#include <library/cpp/archive/yarchive.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/memory/blob.h>
#include <util/stream/output.h>

namespace {
    struct TStaticArchive: public TArchiveReader {
        TStaticArchive()
            : TArchiveReader(TBlob::NoCopy(NMasterGraphTestData::GetData().data(), NMasterGraphTestData::GetData().size()))
        {
        }
    };
}

struct TMasterTaskPath {
    TString Target;
    TTargetTypeParameters::TPath Path;

    bool operator<(const TMasterTaskPath& that) const {
        if (Target < that.Target)
            return true;
        if (Path < that.Path)
            return true;
        return false;
    }

    bool operator==(const TMasterTaskPath& that) const {
        return Target == that.Target && Path == that.Path;
    }
};


template <>
void Out<TMasterTaskPath>(IOutputStream& os, TTypeTraits<TMasterTaskPath>::TFuncParam path) {
    os << path.Target << "/" << ToString(path.Path);
}


struct TMasterGraphTester {
    TMasterListManager ListManager;
    TMasterGraph Graph;

    THolder<IWorkerPoolVariables> Variables;

    enum EHostsConfig {
        NO_CONFIG, HOSTS_CFG, HOSTS_LIST
    };

    TMasterGraphTester(const TString& name, EHostsConfig hostsConfig = NO_CONFIG)
        : Graph(&ListManager, true)
        , Variables(new TWorkerPoolVariablesEmpty())
    {
        if (hostsConfig == HOSTS_CFG) {
            TAutoPtr<IInputStream> hostCfg = Singleton<TStaticArchive>()->ObjectByKey("/" + name + ".host.cfg");
            ListManager.LoadHostcfgFromStream(*hostCfg);
        } else if (hostsConfig == HOSTS_LIST) {
            TAutoPtr<IInputStream> hostsList = Singleton<TStaticArchive>()->ObjectByKey("/" + name + ".list");
            ListManager.LoadHostlistFromString(hostsList->ReadAll());
        }

        TAutoPtr<IInputStream> script = Singleton<TStaticArchive>()->ObjectByKey("/" + name + ".sh");
        Graph.LoadConfig(TMasterConfigSource(script->ReadAll()), nullptr);

        Graph.SetAllTaskState(TS_PENDING);
        AssertPokeNothing();
    }

    void PokeAll() {
        for (TMasterGraph::TTargetsList::const_iterator target = Graph.GetTargets().begin(); target != Graph.GetTargets().end(); ++target) {
            (*target)->UpdatePokeState();
        }
    }

    TVector<TMasterTaskPath> GetPokes() {
        TVector<TMasterTaskPath> r;
        for (TMasterGraph::TTargetsList::const_iterator ti = Graph.GetTargets().begin();
                ti != Graph.GetTargets().end();
                ++ti)
        {
            const TMasterTarget* target = *ti;

            for (TMasterTarget::TConstTaskIterator task = target->TaskIterator(); task.Next(); ) {
                if (task->PokeReady) {
                    r.emplace_back();
                    TMasterTaskPath& path = r.back();
                    path.Target = target->Name;
                    path.Path = task.GetPath();
                }
            }
        }

        std::sort(r.begin(), r.end());

        return r;
    }

    /* if not pokeReady then we are checking 'depfail' poke */
    void AssertPokeOnly(const TVector<TMasterTaskPath>& tasks) {
        PokeAll();

        TVector<TMasterTaskPath> tasksSorted = tasks;
        std::sort(tasksSorted.begin(), tasksSorted.end());

        TVector<TMasterTaskPath> actualTasks = GetPokes();

        if (tasksSorted != actualTasks) {
            ythrow yexception() << "expected " << ToString(tasksSorted) << " actual " << ToString(actualTasks);
        }
    }

    void CollectTasksOnTargetHost(TVector<TMasterTaskPath>& dest, const TString& targetName, const TString& host) {
        const TMasterTarget& target = Graph.GetTargetByName(targetName);

        for (TMasterTarget::TConstTaskIterator task = target.TaskIteratorForHost(host); task.Next(); ) {
            dest.emplace_back();
            TMasterTaskPath& path = dest.back();
            path.Target = targetName;
            path.Path = task.GetPath();
        }
    }

    void AssertPokeOnly(const TString& targetName) {
        TVector<TMasterTaskPath> tasks;

        TMasterTarget& target = Graph.GetTargetByName(targetName);
        for (TVector<TString>::const_iterator host = target.Type->GetHosts().begin(); host != target.Type->GetHosts().end(); ++host) {
            CollectTasksOnTargetHost(tasks, targetName, *host);
        }

        AssertPokeOnly(tasks);
    }

    void AssertPokeOnlyTargetOnHosts(const TString& targetName, const TVector<TString>& hosts) {
        TVector<TMasterTaskPath> tasks;

        for (TVector<TString>::const_iterator host = hosts.begin(); host != hosts.end(); ++host) {
            CollectTasksOnTargetHost(tasks, targetName, *host);
        }

        AssertPokeOnly(tasks);
    }

    void AssertPokeOnlyTargetOnHost(const TString& targetName, const TString& host) {
        TVector<TString> hosts;
        hosts.push_back(host);
        AssertPokeOnlyTargetOnHosts(targetName, hosts);
    }

    void AssertPokeOnlyTargetHostTask(const TString& targetName, const TString& host, const TString& task) {
        AssertPokeOnlyTargetHostsTask(targetName, MakeVector<TString>(host), task);
    }

    void AssertPokeOnlyTargetHostsTask(const TString& targetName, const TVector<TString>& hosts, const TString& taskName) {
        TVector<TMasterTaskPath> tasks;

        TMasterTarget& target = Graph.GetTargetByName(targetName);

        for (TVector<TString>::const_iterator host = hosts.begin(); host != hosts.end(); ++host) {
            TIdForString::TIdSafe hostId = target.Type->GetHostIdSafe(*host);
            TIdForString::TIdSafe taskId = target.Type->GetParameters().GetNameListAtLevel(TTargetTypeParameters::FIRST_PARAM_LEVEL_ID).GetIdByName(taskName);

            TTargetTypeParameters::TProjection proj = target.Type->GetParameters().FirstTwoProjection(hostId, taskId);

            for (TMasterTarget::TConstTaskIterator task = target.TaskIteratorForProjection(proj); task.Next(); ) {
                tasks.emplace_back();
                tasks.back().Target = targetName;
                tasks.back().Path = task.GetPath();
            }
        }

        AssertPokeOnly(tasks);
    }

    void AssertPokeOnlyTargetTasks(const TString& targetName, const TVector<TVector<TString> >& paths) {
        TVector<TMasterTaskPath> fullPaths;
        for (TVector<TVector<TString> >::const_iterator path = paths.begin(); path != paths.end(); ++path) {
            fullPaths.emplace_back();
            fullPaths.back().Target = targetName;
            fullPaths.back().Path = *path;
        }
        AssertPokeOnly(fullPaths);
    }

    // all hosts
    void AssertPokeOnlyTargetTask(const TString& targetName, const TString& task) {
        AssertPokeOnlyTargetHostsTask(targetName, Graph.GetTargetByName(targetName).Type->GetHosts(), task);
    }

    void AssertPokeNothing() {
        TVector<TMasterTaskPath> tasks;
        AssertPokeOnly(tasks);
    }
};

Y_UNIT_TEST_SUITE(TMasterGraphTest) {
    Y_UNIT_TEST(TestDependOnPrevDefault) {
        TMasterGraphTester graphTester("depend-on-prev-default");

        graphTester.Graph.GetTargetByName("first").SetAllTaskState(TS_SUCCESS);

        graphTester.PokeAll();
        graphTester.AssertPokeOnly("second");
    }

    Y_UNIT_TEST(TestDependOnPrevCaret) {
        TMasterGraphTester graphTester("depend-on-prev-caret");

        graphTester.Graph.GetTargetByName("first").SetAllTaskState(TS_SUCCESS);

        graphTester.PokeAll();
        graphTester.AssertPokeOnly("second");
    }

    Y_UNIT_TEST(TestLocal) {
        TMasterGraphTester graphTester("local");

        graphTester.Graph.GetTargetByName("first").SetAllTaskState(TS_SUCCESS);

        graphTester.PokeAll();
        graphTester.AssertPokeNothing();
    }

    Y_UNIT_TEST(TestLocalMultiple) {
        TMasterGraphTester graphTester("local-multiple");

        UNIT_ASSERT_VALUES_EQUAL(size_t(3), graphTester.Graph.GetTargetByName("first").Type->GetHostCount());

        graphTester.Graph.GetTargetByName("first").SetAllTaskState(TS_SUCCESS);

        graphTester.AssertPokeNothing();
    }

    Y_UNIT_TEST(TestLocalNotGather) {
        TMasterGraphTester graphTester("local-not-gather");

        UNIT_ASSERT_VALUES_EQUAL(size_t(3), graphTester.Graph.GetTargetByName("first").Type->GetHostCount());
        UNIT_ASSERT_VALUES_EQUAL(size_t(3), graphTester.Graph.GetTargetByName("second").Type->GetParamCountAtSecondLevel());

        graphTester.Graph.GetTargetByName("first").SetAllTaskState(TS_SUCCESS);

        graphTester.AssertPokeNothing();
    }

    Y_UNIT_TEST(TestLocalNotScatter) {
        TMasterGraphTester graphTester("local-not-scatter");

        UNIT_ASSERT_VALUES_EQUAL(size_t(3), graphTester.Graph.GetTargetByName("first").Type->GetHostCount());
        UNIT_ASSERT_VALUES_EQUAL(size_t(3), graphTester.Graph.GetTargetByName("first").Type->GetParamCountAtSecondLevel());

        graphTester.Graph.GetTargetByName("first").SetAllTaskState(TS_SUCCESS);

        graphTester.AssertPokeNothing();
    }

    Y_UNIT_TEST(TestLocalForceGather) {
        TMasterGraphTester graphTester("local-force-gather");

        graphTester.Graph.SetAllTaskState(TS_PENDING);
        graphTester.Graph.GetTargetByName("first").SetAllTaskStateOnWorker("worker2", TS_SUCCESS);

        graphTester.AssertPokeOnlyTargetTask("second", "worker2");
    }

    Y_UNIT_TEST(TestLocalForceScatter) {
        TMasterGraphTester graphTester("local-force-scatter");

        graphTester.Graph.GetTargetByName("first").SetTaskState("worker0", "worker2", TS_SUCCESS);
        graphTester.Graph.GetTargetByName("first").SetAllTaskStateOnWorker("worker1", TS_SUCCESS);
        graphTester.AssertPokeNothing();

        graphTester.Graph.GetTargetByName("first").SetTaskState("worker2", "worker2", TS_SUCCESS);
        graphTester.AssertPokeOnlyTargetOnHost("second", "worker2");
    }

    Y_UNIT_TEST(TestDependsOnTwo) {
        TMasterGraphTester graphTester("depends-on-two");

        graphTester.Graph.GetTargetByName("first").SetAllTaskState(TS_SUCCESS);

        graphTester.AssertPokeNothing();

        graphTester.Graph.GetTargetByName("second").SetAllTaskState(TS_SUCCESS);

        graphTester.AssertPokeOnly("third");
    }

    Y_UNIT_TEST(TestCrossnode) {
        TMasterGraphTester graphTester("crossnode");

        graphTester.Graph.GetTargetByName("first").SetAllTaskStateOnWorker("hosta", TS_SUCCESS);
        graphTester.AssertPokeNothing();

        graphTester.Graph.GetTargetByName("first").SetAllTaskStateOnWorker("hostb", TS_SUCCESS);
        graphTester.AssertPokeNothing();

        graphTester.Graph.GetTargetByName("first").SetAllTaskStateOnWorker("hostc", TS_SUCCESS);
        graphTester.AssertPokeOnly("second");
    }

    Y_UNIT_TEST(TestCrossnodeSome) {
        TMasterGraphTester graphTester("crossnode-some");

        graphTester.Graph.GetTargetByName("first").SetAllTaskStateOnWorker("hosta", TS_SUCCESS);
        graphTester.AssertPokeNothing();

        graphTester.Graph.GetTargetByName("second").SetAllTaskStateOnWorker("hosta", TS_SUCCESS);
        graphTester.Graph.GetTargetByName("second").SetAllTaskStateOnWorker("hostb", TS_SUCCESS);
        graphTester.AssertPokeNothing();

        graphTester.Graph.GetTargetByName("second").SetAllTaskStateOnWorker("hostc", TS_SUCCESS);
        graphTester.AssertPokeOnlyTargetOnHost("third", "hosta");
    }

    Y_UNIT_TEST(TestCrossnodeWithTwoParams) {
        TMasterGraphTester graphTester("crossnode-p2");

        graphTester.Graph.GetTargetByName("first").SetAllTaskStateOnWorker("hosta", TS_SUCCESS);
        graphTester.AssertPokeNothing();

        graphTester.Graph.GetTargetByName("first").SetTaskState(MakeVector<TString>("hostb", "11", "bb"), TS_SUCCESS);
        graphTester.Graph.GetTargetByName("first").SetTaskState(MakeVector<TString>("hostb", "22", "bb"), TS_SUCCESS);
        graphTester.Graph.GetTargetByName("first").SetTaskState(MakeVector<TString>("hostb", "33", "bb"), TS_SUCCESS);
        graphTester.AssertPokeNothing();

        graphTester.Graph.GetTargetByName("first").SetTaskState(MakeVector<TString>("hostb", "11", "aa"), TS_SUCCESS);
        graphTester.Graph.GetTargetByName("first").SetTaskState(MakeVector<TString>("hostb", "22", "aa"), TS_SUCCESS);
        graphTester.AssertPokeNothing();

        graphTester.Graph.GetTargetByName("first").SetTaskState(MakeVector<TString>("hostb", "33", "aa"), TS_SUCCESS);
        graphTester.AssertPokeOnly("second");
    }

    Y_UNIT_TEST(TestScatter) {
        TMasterGraphTester graphTester("scatter");

        graphTester.Graph.GetTargetByName("copy_to_workers").SetTaskState("main", "worker2", TS_SUCCESS);

        graphTester.AssertPokeOnlyTargetOnHost("split_8", "worker2");
    }

    Y_UNIT_TEST(TestGather) {
        TMasterGraphTester graphTester("gather");

        graphTester.Graph.GetTargetByName("join").SetAllTaskStateOnWorker("worker3", TS_SUCCESS);

        graphTester.AssertPokeOnlyTargetHostTask("copy_from_workers", "main", "worker3");
    }

    Y_UNIT_TEST(TestScatterNonUniform) {
        TMasterGraphTester graphTester("scatter-non-uniform", TMasterGraphTester::HOSTS_CFG);

        graphTester.Graph.GetTargetByName("first").SetAllTaskStateOnWorker("walrus000", TS_SUCCESS);
        graphTester.Graph.GetTargetByName("first").SetAllTaskStateOnWorker("walrus002", TS_SUCCESS);
        graphTester.Graph.GetTargetByName("first").SetAllTaskStateOnWorker("walrus003", TS_SUCCESS);

        graphTester.AssertPokeNothing();

        graphTester.Graph.GetTargetByName("first").SetTaskState("walrus001", "primus002", TS_SUCCESS);

        graphTester.AssertPokeOnlyTargetOnHost("second", "primus002");

        graphTester.Graph.GetTargetByName("first").SetTaskState("walrus001", "primus001", TS_SUCCESS);

        graphTester.AssertPokeOnlyTargetOnHosts("second", MakeVector<TString>("primus001", "primus002"));
    }

    Y_UNIT_TEST(TestGatherNonUniform) {
        TMasterGraphTester graphTester("gather-non-uniform", TMasterGraphTester::HOSTS_CFG);

        graphTester.Graph.GetTargetByName("first").SetAllTaskStateOnWorker("primus004", TS_SUCCESS);

        graphTester.AssertPokeOnlyTargetHostsTask("second", MakeVector<TString>("walrus002", "walrus003", "walrus004"), "primus004");
    }

    Y_UNIT_TEST(TestExplicitMapping12_20) {
        TMasterGraphTester graphTester("12-20");

        graphTester.Graph.GetTargetByName("first").SetTaskState(MakeVector<TString>("hosta", "xx", "worker1"), TS_SUCCESS);
        graphTester.AssertPokeNothing();
        graphTester.Graph.GetTargetByName("first").SetTaskState(MakeVector<TString>("hostb", "xx", "worker1"), TS_SUCCESS);
        graphTester.AssertPokeOnlyTargetTasks("second",
                MakeVector(
                        MakeVector<TString>("worker1", "00", "xx"),
                        MakeVector<TString>("worker1", "01", "xx")
                ));
    }

    Y_UNIT_TEST(TestGroup) {
        TMasterGraphTester graphTester("group", TMasterGraphTester::HOSTS_CFG);

        TMasterTarget& first = graphTester.Graph.GetTargetByName("first");

        first.SetAllTaskStateOnWorker("walrus000", TS_SUCCESS);
        first.SetAllTaskStateOnWorker("walrus002", TS_SUCCESS);
        first.SetAllTaskStateOnWorker("walrus003", TS_SUCCESS);

        graphTester.AssertPokeNothing();

        first.SetAllTaskStateOnWorker("walrus001", TS_SUCCESS);
        graphTester.AssertPokeOnlyTargetOnHosts(
                "second",
                MakeVector<TString>("primus000", "primus001", "primus002"));

        UNIT_ASSERT(first.Depends.size() == 0 && first.Followers.size() == 1);

        TMasterDepend& groupdep = first.Followers[0];

        IPrecomputedTaskIdsInitializer* initializer = groupdep.GetPrecomputedTaskIdsMaybe()->Get();

        TDependEdgesEnumerator en(initializer->GetIds());

        UNIT_ASSERT(en.Next());
        UNIT_ASSERT_EQUAL(MakeVector<ui32>(0, 1), en.GetDepTaskIds().ToVector());
        UNIT_ASSERT_EQUAL(MakeVector<ui32>(0, 1, 2), en.GetMyTaskIds().ToVector());

        UNIT_ASSERT(en.Next());
        UNIT_ASSERT_EQUAL(MakeVector<ui32>(2, 3, 4), en.GetDepTaskIds().ToVector());
        UNIT_ASSERT_EQUAL(MakeVector<ui32>(3, 4), en.GetMyTaskIds().ToVector());

        UNIT_ASSERT(!en.Next());
    }

    Y_UNIT_TEST(TestGroupIntParam) {
        TMasterGraphTester graphTester("group-int-param", TMasterGraphTester::HOSTS_CFG);

        graphTester.Graph.GetTargetByName("first").SetAllTaskStateOnWorker("walrus000", TS_SUCCESS);
        graphTester.Graph.GetTargetByName("first").SetAllTaskStateOnWorker("walrus002", TS_SUCCESS);
        graphTester.Graph.GetTargetByName("first").SetAllTaskStateOnWorker("walrus003", TS_SUCCESS);

        graphTester.AssertPokeNothing();

        graphTester.Graph.GetTargetByName("first").SetTaskState("walrus001", "0", TS_SUCCESS);

        graphTester.AssertPokeNothing();

        graphTester.Graph.GetTargetByName("first").SetTaskState("walrus001", "1", TS_SUCCESS);
        graphTester.Graph.GetTargetByName("first").SetTaskState("walrus001", "2", TS_SUCCESS);

        graphTester.AssertPokeOnlyTargetOnHosts(
                "second",
                MakeVector<TString>("primus000", "primus001", "primus002"));
    }

    Y_UNIT_TEST(TestDifferentOrder) {
        // fails to load if parameters are matched unsorted
        TMasterGraphTester("different-order");
    }

    Y_UNIT_TEST(TestRegression1_cm_robmerge_tree) {
        TMasterGraphTester("cm_robmerge_tree_stt", TMasterGraphTester::HOSTS_CFG);
    }

    Y_UNIT_TEST(TestRegression2_undected_gather) {
        TMasterGraphTester graphTester("undectected-gather", TMasterGraphTester::HOSTS_CFG);

        UNIT_ASSERT_VALUES_EQUAL(size_t(0), graphTester.Graph.GetTargetByName("A").Depends.size());
        UNIT_ASSERT_VALUES_EQUAL(size_t(1), graphTester.Graph.GetTargetByName("B").Depends.size());

        TMasterDepend& depend = *graphTester.Graph.GetTargetByName("B").Depends.begin();
        UNIT_ASSERT_VALUES_EQUAL("A", depend.GetRealTarget()->GetName());
        UNIT_ASSERT_VALUES_EQUAL("[0->1]", depend.GetJoinParamMappings()->ToString());
    }

    Y_UNIT_TEST(TestRegression3_undectected_cluster) {
        TMasterGraphTester graphTester("cm_multi_cluster_dep_stt", TMasterGraphTester::HOSTS_CFG);

        const TMasterTarget& d = graphTester.Graph.GetTargetByName("D");
        UNIT_ASSERT_VALUES_EQUAL(size_t(1), d.Depends.size());
        UNIT_ASSERT_VALUES_EQUAL("[0->0,1->1,2->2,3->3,4->4]", d.Depends.front().GetJoinParamMappings()->ToString());
    }

    void CheckPartialOrderForTwoGroups(const TVector<TString>& first, const TVector<TString>& second,
            const THashMap<TString, int>& indices, const TVector<TString>& result)
    {
        for (TVector<TString>::const_iterator i = second.begin(); i != second.end(); i++) {
            int secondInd = indices.find(*i)->second;
            for (TVector<TString>::const_iterator j = first.begin(); j != first.end(); j++) {
                if (!(secondInd > indices.find(*j)->second)) {
                    UNIT_FAIL(Sprintf("Index of '%s' must be greater than index of '%s'! List which don't satisfy: '%s'", (*i).data(), (*j).data(), ToString(result).data()));
                }
            }
        }
    }

    void CheckPartialOrder(const TVector<TVector<TString> >& groups, const TVector<TString>& result) {
        Y_VERIFY(groups.size() > 1);
        THashMap<TString, int> indices;
        int c = 0;
        for (TVector<TString>::const_iterator i = result.begin(); i != result.end(); i++, c++) {
            indices[*i] = c;
        }
        for (size_t i = 1; i < groups.size(); i++) {
            CheckPartialOrderForTwoGroups(groups.at(i - 1), groups.at(i), indices, result);
        }
    }

    typedef TList<const TMasterTarget*> TTargetList;

    TVector<TString> ReverseAndGetNames(const TTargetList& targets) {
        TVector<TString> result;
        for (TTargetList::const_reverse_iterator i = targets.rbegin(); i != targets.rend(); i++) {
            result.push_back((*i)->GetName());
        }
        return result;
    }

    Y_UNIT_TEST(TestTopoSortedSubgraph) {
        TMasterGraphTester graphTester("topo_sort");

        {
            const TMasterTarget& t1 = graphTester.Graph.GetTargetByName("t1");

            TTargetList topoSorted;
            TMasterTarget::TTraversalGuard guard;
            t1.TopoSortedSubgraph<TMasterGraph::TMasterTargetEdgePredicate>(&topoSorted, false, guard);

            TVector<TString> names = ReverseAndGetNames(topoSorted);

            UNIT_ASSERT(std::find(names.begin(), names.end(), "t8") == names.end()); // because of conditional depend

            CheckPartialOrder(MakeVector<TVector<TString> >(MakeVector<TString>("t1"), MakeVector<TString>("t2", "t3"),
                    MakeVector<TString>("t4", "t5"), MakeVector<TString>("t6", "t7")), names);
        }

        {
            const TMasterTarget& t6 = graphTester.Graph.GetTargetByName("t6");

            TTargetList topoSorted;
            TMasterTarget::TTraversalGuard guard;
            t6.TopoSortedSubgraph<TMasterGraph::TMasterTargetEdgePredicate>(&topoSorted, true, guard);

            TVector<TString> names = ReverseAndGetNames(topoSorted);

            UNIT_ASSERT(std::find(names.begin(), names.end(), "t0") == names.end());

            CheckPartialOrder(MakeVector<TVector<TString> >(MakeVector<TString>("t6"), MakeVector<TString>("t4", "t5"),
                    MakeVector<TString>("t2", "t1")), names);
        }

        {
            const TMasterTarget& t3 = graphTester.Graph.GetTargetByName("t3");

            TTargetList topoSorted;
            TMasterTarget::TTraversalGuard guard;
            t3.TopoSortedSubgraph<TMasterGraph::TMasterTargetEdgePredicate>(&topoSorted, true, guard);

            UNIT_ASSERT_VALUES_EQUAL(ToString(MakeVector<TString>("t3", "t1")), ToString(ReverseAndGetNames(topoSorted)));
        }
    }

    Y_UNIT_TEST(TestFakeStates) {
        TMasterGraphTester graphTester("fake-states", TMasterGraphTester::HOSTS_LIST);

        const TMasterDepend& dep = graphTester.Graph.GetTargetByName("b").Depends[0];

        TVector<ui32> RealStatesExpected = MakeVector<ui32>(0, 1, 4, 5, 6, 7);

        IPrecomputedTaskIdsInitializer* precomputed = dep.GetPrecomputedTaskIdsMaybe()->Get();
        // precomputed->Initialize();

        TVector<ui32> RealStatesGot;
        for (TDependEdgesEnumerator en(precomputed->GetIds()); en.Next(); ) {
            RealStatesGot.push_back(en.GetNAnyState());
        }

        UNIT_ASSERT_VALUES_EQUAL(ToString(RealStatesExpected), ToString(RealStatesGot));
    }

    TVector<TString> GetTargetNames(const TCronState::TSubgraph& subgraph) {
        TVector<TString> result;
        for (TCronState::TSubgraph::TTargets::const_iterator i = subgraph.GetTargets().begin(); i != subgraph.GetTargets().end(); ++i) {
            result.push_back(i->GetTarget()->GetName());
        }
        return result;
    }

    TCronState::TSubgraphTargetWithTasks::TTasks GetTasksForTarget(TString targetName,
            const TCronState::TSubgraph& subgraph)
    {
        for (TCronState::TSubgraph::TTargets::const_iterator i = subgraph.GetTargets().begin(); i != subgraph.GetTargets().end(); ++i) {
            if (i->GetTarget()->GetName() == targetName) {
                return i->GetTasks();
            }
        }
        Y_FAIL();
    }

    Y_UNIT_TEST(TestCronSubgraph) {
        TMasterGraphTester graphTester("cron_subgraph");

        TMasterTarget& v = graphTester.Graph.GetTargetByName("v");
        TCronState& cron_v = v.CronStateForHost("w8");
        UNIT_ASSERT_EQUAL(MakeVector<TString>("w7", "w8", "w9", "ya"), cron_v.GetSubgraphHostsSorted());
        UNIT_ASSERT_EQUAL(MakeVector<TString>("v", "u", "u1", "u0"), GetTargetNames(cron_v.GetSubgraph()));
        UNIT_ASSERT_EQUAL(MakeVector<ui32>(4, 5, 6, 7), GetTasksForTarget("v", cron_v.GetSubgraph()));
        UNIT_ASSERT_EQUAL(MakeVector<ui32>(1, 4, 7), GetTasksForTarget("u", cron_v.GetSubgraph()));
        UNIT_ASSERT_EQUAL(MakeVector<ui32>(4, 5, 6, 7), GetTasksForTarget("u1", cron_v.GetSubgraph()));
        UNIT_ASSERT_EQUAL(MakeVector<ui32>(1), GetTasksForTarget("u0", cron_v.GetSubgraph()));

        TMasterTarget& t = graphTester.Graph.GetTargetByName("t");
        TCronState& cron_t = t.CronStateForHost("w1");
        UNIT_ASSERT_EQUAL(MakeVector<TString>("w1", "w2"), cron_t.GetSubgraphHostsSorted());
        UNIT_ASSERT_EQUAL(MakeVector<TString>("t", "s"), GetTargetNames(cron_t.GetSubgraph()));
        UNIT_ASSERT_EQUAL(MakeVector<ui32>(0, 1), GetTasksForTarget("s", cron_t.GetSubgraph()));
        UNIT_ASSERT_EQUAL(MakeVector<ui32>(0, 1), GetTasksForTarget("t", cron_t.GetSubgraph()));
    }

    Y_UNIT_TEST(TestHostlistComments) {
        TMasterGraphTester graphTester("comments", TMasterGraphTester::HOSTS_LIST);
        TVector<TString> result;
        graphTester.ListManager.GetList("hostlist", result);
        UNIT_ASSERT_EQUAL(MakeVector<TString>("host1", "host2"), result);
    }
}
