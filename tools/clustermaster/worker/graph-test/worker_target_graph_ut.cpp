// include before unittest

#include "data.h"

#include <tools/clustermaster/worker/graph_change_watcher.h>
#include <tools/clustermaster/worker/worker_target_graph.h>

#include <tools/clustermaster/common/make_vector.h>
#include <tools/clustermaster/common/vector_to_string.h>

#include <library/cpp/archive/yarchive.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/memory/blob.h>
#include <util/stream/output.h>

namespace {
    struct TStaticArchive: public TArchiveReader {
        TStaticArchive()
            : TArchiveReader(TBlob::NoCopy(NWorkerGraphTestData::GetData().data(), NWorkerGraphTestData::GetData().size()))
        {
        }
    };
}


struct TWorkerTaskPath {
    TString Target;
    TTargetTypeParameters::TId TaskId;

    bool operator<(const TWorkerTaskPath& that) const {
        if (Target < that.Target)
            return true;
        if (TaskId.GetN() < that.TaskId.GetN())
            return true;
        return false;
    }

    bool operator==(const TWorkerTaskPath& that) const {
        return Target == that.Target && TaskId.GetN() == that.TaskId.GetN();
    }
};

template <>
void Out<TWorkerTaskPath>(IOutputStream& out, TTypeTraits<TWorkerTaskPath>::TFuncParam path) {
    out << path.Target << "/" << path.TaskId.GetN();
}


struct TWorkerGraphTester {
    TWorkerListManager ListManager;
    TWorkerGlobals Globals;
    TWorkerGraph Graph;

    TWorkerGraphTester(const TString& name)
        : Graph(&ListManager, &Globals, true)
    {
        TConfigMessage config;

        TString configFileText = Singleton<TStaticArchive>()->ObjectByKey("/" + name + ".sh")->ReadAll();

        config.SetWorkerName("localhost");

        config.SetConfig(configFileText);

        THashSet<TString> localTargetTypes;

        TTargetGraphParsed graphParsed = TTargetGraphParsed::Parse(TStringBuf(configFileText));
        const TVector<TTargetTypeParsed>& typeNames = graphParsed.GetTypes();
        for (TVector<TTargetTypeParsed>::const_iterator type = typeNames.begin();
                type != typeNames.end(); ++type)
        {
            if (Contains(type->Paramss.at(0), TString("localhost"))) {
                config.AddThisWorkerTargetTypes()->SetName(type->Name);
                localTargetTypes.insert(type->Name);
            }
        }

        const TVector<TTargetParsed>& targetsParsed = graphParsed.GetTargets();
        for (TVector<TTargetParsed>::const_iterator target = targetsParsed.begin();
                target != targetsParsed.end(); ++target)
        {
            if (localTargetTypes.find(target->Type) != localTargetTypes.end()) {
                ::NProto::TConfigMessage_TThisWorkerTarget* msgTarget = config.AddThisWorkerTargets();
                msgTarget->SetName(target->Name);
                msgTarget->SetHasCrossnodeDepends(target->Name.EndsWith("_cn"));
            }
        }

        Graph.ImportConfig(&config, false);

        Graph.SetAllTaskState(TS_IDLE);

        AssertChangeNothing();
    }

    void SetTaskState(const TString& targetName, const TVector<TString> params, const TTaskState& state) {
        TWorkerTarget& target = Graph.GetTargetByName(targetName);
        TWorkerTaskStatus& status = target.GetTasks().At(target.Type->GetIdByScriptParams(params));
        status.SetState(state);
    }

    void TryReadyAllSomeTasks(TGraphChangeWatcher& watcher) {
        for (TWorkerGraph::TTargetsList::const_iterator target = Graph.GetTargets().begin(); target != Graph.GetTargets().end(); ++target) {
            (*target)->TryReadySomeTasks(watcher);
        }
    }

    void AssertChangeOnly(const TVector<TWorkerTaskPath>& expectedTasks) {
        TGraphChangeWatcher watcher;
        TryReadyAllSomeTasks(watcher);

        TVector<TWorkerTaskPath> actualTasks;

        for (TGraphChangeWatcher::TChanges::const_iterator change = watcher.GetChanges().begin();
                change != watcher.GetChanges().end(); ++change)
        {
            actualTasks.emplace_back();
            actualTasks.back().Target = (*change)->Target->GetName();
            actualTasks.back().TaskId = (*change)->nTask;
        }

        TVector<TWorkerTaskPath> expectedTasksSorted = Sorted(expectedTasks);
        TVector<TWorkerTaskPath> actualTasksSorted = Sorted(actualTasks);

        UNIT_ASSERT_VALUES_EQUAL(ToString(expectedTasksSorted), ToString(actualTasksSorted));
    }

    void AssertChangeNothing() {
        AssertChangeOnly(MakeVector<TWorkerTaskPath>());
    }

    void AssertChangeAllOnTarget(const TString& target) {
        TVector<TWorkerTaskPath> tasks;

        const TWorkerTargetType* type = Graph.GetTargetByName(target).Type;
        TTargetTypeParameters::TIterator it = type->GetParameters().Iterator();
        while (it.Next()) {
            tasks.emplace_back();
            tasks.back().Target = target;
            tasks.back().TaskId = it.CurrentN();
        }

        AssertChangeOnly(tasks);
    }

    void AssertChangeOnlyTaskOnTarget(const TString& targetName, const TVector<TString>& params) {
        AssertChangeOnlyTasksOnTarget(targetName, MakeVector(params));
    }

    void AssertChangeOnlyTasksOnTarget(const TString& targetName, const TVector<TVector<TString> >& paramss) {

        TWorkerTarget& target = Graph.GetTargetByName(targetName);

        TVector<TWorkerTaskPath> paths;

        for (TVector<TVector<TString> >::const_iterator params = paramss.begin();
                params != paramss.end();
                ++params)
        {
            paths.emplace_back();
            paths.back().Target = targetName;
            paths.back().TaskId = target.Type->GetIdByScriptParams(*params);

        }


        return AssertChangeOnly(paths);
    }

};


Y_UNIT_TEST_SUITE(TWorkerGraphTest) {

    Y_UNIT_TEST(TestSimple) {
        TWorkerGraphTester gt("simple");

        gt.Graph.SetAllTaskState(TS_PENDING);

        gt.AssertChangeAllOnTarget("first");

        gt.Graph.GetTargetByName("first").SetAllTaskState(TS_SUCCESS);

        gt.AssertChangeAllOnTarget("second");
    }

    Y_UNIT_TEST(TestSimpleClustered) {
        TWorkerGraphTester gt("simple-clustered");

        gt.Graph.SetAllTaskState(TS_PENDING);

        gt.AssertChangeAllOnTarget("first");

        gt.SetTaskState("first", MakeVector<TString>("bbb"), TS_SUCCESS);

        gt.AssertChangeOnlyTaskOnTarget("second", MakeVector<TString>("bbb"));
    }

    Y_UNIT_TEST(TestSimpleClusteredP2) {
        TWorkerGraphTester gt("simple-clustered-p2");

        gt.Graph.SetAllTaskState(TS_PENDING);

        gt.AssertChangeAllOnTarget("first");

        gt.SetTaskState("first", MakeVector<TString>("bbb", "003"), TS_SUCCESS);

        gt.AssertChangeOnlyTaskOnTarget("second", MakeVector<TString>("bbb", "003"));
    }

    Y_UNIT_TEST(TestExplicitMapping00_12_32) {
        TWorkerGraphTester gt("00-12-32");

        gt.Graph.SetAllTaskState(TS_PENDING);

        gt.AssertChangeAllOnTarget("first");

        gt.SetTaskState("first", MakeVector<TString>("xx", "worker1", "01"), TS_SUCCESS);
        gt.AssertChangeNothing();

        gt.SetTaskState("first", MakeVector<TString>("xx", "worker2", "01"), TS_SUCCESS);
        gt.AssertChangeNothing();

        gt.SetTaskState("first", MakeVector<TString>("yy", "worker2", "01"), TS_SUCCESS);
        gt.AssertChangeNothing();

        gt.SetTaskState("first", MakeVector<TString>("xx", "worker3", "01"), TS_SUCCESS);
        gt.AssertChangeOnlyTasksOnTarget("second", MakeVector(
                MakeVector<TString>("01", "xx", "aa"),
                MakeVector<TString>("01", "xx", "bb")
            ));
    }

    Y_UNIT_TEST(TestCrossnode) {
        TWorkerGraphTester gt("crossnode");

        gt.Graph.SetAllTaskState(TS_PENDING);

        gt.AssertChangeAllOnTarget("first");

        gt.Graph.GetTargetByName("first").SetAllTaskState(TS_SUCCESS);

        gt.AssertChangeNothing();
    }

    /* distinguish only TS_DEPFAILED and TS_PENDING states */
    TString StatesAsString(const TWorkerTarget::TTaskList& tasks) {
        TString resultingStr;
        for (TWorkerTarget::TTaskList::TConstEnumerator i = tasks.Enumerator(); i.Next(); ) {
            switch (i->GetState().GetId()) {
            case NProto::TS_DEPFAILED:
                resultingStr += 'D';
                break;
            case NProto::TS_PENDING:
                resultingStr += 'P';
                break;
            default:
                resultingStr += 'O';
                break;
            }
        }
        return resultingStr;
    }

    Y_UNIT_TEST(TestDepfail) {
        TWorkerGraphTester gt("depfail");

        gt.Graph.SetAllTaskState(TS_PENDING);

        TWorkerTarget* a = &gt.Graph.GetTargetByName("a");
        const TWorkerTarget& b = gt.Graph.GetTargetByName("b");
        const TWorkerTarget& c = gt.Graph.GetTargetByName("c");
        const TWorkerTarget& d = gt.Graph.GetTargetByName("d");
        const TWorkerTarget& e = gt.Graph.GetTargetByName("e");
        const TWorkerTarget& f = gt.Graph.GetTargetByName("f");
        const TWorkerTarget& g = gt.Graph.GetTargetByName("g");
        const TWorkerTarget& i = gt.Graph.GetTargetByName("i");

        TGraphChangeWatcher watcher;
        TTargetTypeParametersMap<bool> startMask(&a->Type->GetParametersHyperspace());
        startMask.At(0) = true;
        startMask.At(1) = true;
        gt.Graph.Depfail(a, startMask, watcher, false);
        watcher.Commit();

        UNIT_ASSERT_EQUAL("DDPP", StatesAsString(a->GetTasks()));
        UNIT_ASSERT_EQUAL("DDPP", StatesAsString(b.GetTasks()));
        UNIT_ASSERT_EQUAL("DPDP", StatesAsString(c.GetTasks()));
        UNIT_ASSERT_EQUAL("DDDP", StatesAsString(d.GetTasks()));
        UNIT_ASSERT_EQUAL("DDDDDDPP", StatesAsString(e.GetTasks()));
        UNIT_ASSERT_EQUAL("DDPPDDPP", StatesAsString(f.GetTasks()));
        UNIT_ASSERT_EQUAL("", StatesAsString(g.GetTasks())); // no tasks - this target is on other worker
        UNIT_ASSERT_EQUAL("PPPP", StatesAsString(i.GetTasks())); // is connected to its 'brothers' on localhost through other worker
    }

    typedef TList<const TWorkerTarget*> TTargetList;

    TVector<TString> ReverseAndGetNames(const TTargetList& targets) {
        TVector<TString> result;
        for (TTargetList::const_reverse_iterator i = targets.rbegin(); i != targets.rend(); i++) {
            result.push_back((*i)->GetName());
        }
        return result;
    }

    Y_UNIT_TEST(TestTopoSortedSubgraph) {
        TWorkerGraphTester gt("topo-sort");

        {
            const TWorkerTarget& t1 = gt.Graph.GetTargetByName("t1");

            TTargetList topoSorted;
            TWorkerTarget::TTraversalGuard guard;
            t1.TopoSortedSubgraph<TWorkerGraph::TWorkerTargetEdgePredicate>(&topoSorted, false, guard);

            UNIT_ASSERT_EQUAL(ToString(MakeVector<TString>("t1", "t2", "t5", "t4_cn", "t3")), ToString(ReverseAndGetNames(topoSorted)));
        }
    }

    TString TargetMaskAsString(TWorkerGraph& graph, const TString& targetName,
            const NGatherSubgraph::TResult<TWorkerTarget>& result)
    {
        const TWorkerTarget& target = graph.GetTargetByName(targetName);

        NGatherSubgraph::TResultForTarget* resultForTarget = result.GetResultByTarget().find(&target)->second;

        TString resultingStr;
        for (NGatherSubgraph::TMask::TConstEnumerator i = resultForTarget->Mask.Enumerator(); i.Next(); ) {
            if (*i == true) {
                if (resultForTarget->Skipped == NGatherSubgraph::TResultForTarget::SS_NOT_SKIPPED) {
                    resultingStr += '1';
                } else { // SS_SKIPPED
                    resultingStr += '!';
                }
            } else {
                resultingStr += '0';
            }
        }
        return resultingStr;
    }

    Y_UNIT_TEST(TestSubgraphGatherer) {
        TWorkerGraphTester gt("depfail");

        const TWorkerTarget& a = gt.Graph.GetTargetByName("a");

        TTargetTypeParametersMap<bool> map(&a.Type->GetParametersHyperspace());
        map.At(0) = true;
        map.At(1) = true;

        {
            NGatherSubgraph::TResult<TWorkerTarget> result;
            NGatherSubgraph::GatherSubgraph<TWorkerGraphTypes, TParamsByTypeHyperspace, TWorkerGraph::TWorkerDepfailTargetEdgePredicate>
                (a, map, NGatherSubgraph::M_DEPFAIL, &result);

            UNIT_ASSERT_EQUAL(9, result.GetResultByTarget().size());

            UNIT_ASSERT_EQUAL("1100", TargetMaskAsString(gt.Graph, "a", result));
            UNIT_ASSERT_EQUAL("1100", TargetMaskAsString(gt.Graph, "b", result));
            UNIT_ASSERT_EQUAL("1010", TargetMaskAsString(gt.Graph, "c", result));
            UNIT_ASSERT_EQUAL("1110", TargetMaskAsString(gt.Graph, "d", result));
            UNIT_ASSERT_EQUAL("11111100", TargetMaskAsString(gt.Graph, "e", result));
            UNIT_ASSERT_EQUAL("11001100", TargetMaskAsString(gt.Graph, "f", result));
            UNIT_ASSERT_EQUAL("11111111", TargetMaskAsString(gt.Graph, "h", result));

            // Testing 'inactive' tasks for turned-off conditional depend
            UNIT_ASSERT_EQUAL("!!00", TargetMaskAsString(gt.Graph, "t1", result));
            UNIT_ASSERT_EQUAL("!!00", TargetMaskAsString(gt.Graph, "t2", result));
        }

        {
            NGatherSubgraph::TResult<TWorkerTarget> result;
            NGatherSubgraph::GatherSubgraph<TWorkerGraphTypes, TParamsByTypeHyperspace, TWorkerGraph::TWorkerTargetEdgePredicate>
                (a, map, NGatherSubgraph::M_COMMAND_RECURSIVE_DOWN, &result);

            UNIT_ASSERT_EQUAL(11, result.GetResultByTarget().size());

            UNIT_ASSERT_EQUAL("1100", TargetMaskAsString(gt.Graph, "a", result));
            UNIT_ASSERT_EQUAL("1100", TargetMaskAsString(gt.Graph, "b", result));
            UNIT_ASSERT_EQUAL("1010", TargetMaskAsString(gt.Graph, "c", result));
            UNIT_ASSERT_EQUAL("1110", TargetMaskAsString(gt.Graph, "d", result));
            UNIT_ASSERT_EQUAL("11111100", TargetMaskAsString(gt.Graph, "e", result));
            UNIT_ASSERT_EQUAL("11001100", TargetMaskAsString(gt.Graph, "f", result));
            UNIT_ASSERT_EQUAL("00", TargetMaskAsString(gt.Graph, "g", result));
            UNIT_ASSERT_EQUAL("11110000", TargetMaskAsString(gt.Graph, "h", result));
            UNIT_ASSERT_EQUAL("0000", TargetMaskAsString(gt.Graph, "i", result));

            // Testing 'inactive' tasks for turned-off conditional depend
            UNIT_ASSERT_EQUAL("!!00", TargetMaskAsString(gt.Graph, "t1", result));
            UNIT_ASSERT_EQUAL("!!00", TargetMaskAsString(gt.Graph, "t2", result));
        }
    }

    TString TaskStatesAsString(const TWorkerTarget &t) {
        TString res = "";
        for (TWorkerTarget::TConstTaskIterator i = t.TaskIterator(); i.Next(); ) {
            switch (i->GetState().GetId()) {
                case NProto::TS_IDLE:
                    res += 'I';
                    break;
                case NProto::TS_PENDING:
                    res += 'P';
                    break;
                case NProto::TS_READY:
                    res += 'R';
                    break;
                case NProto::TS_SKIPPED:
                    res += 'S';
                    break;
                case NProto::TS_SUCCESS:
                    res += '+';
                    break;
                default:
                    res += '?';
                    break;
            }
        }
        return res;
    }

    Y_UNIT_TEST(TestCommand) {
        TWorkerGraphTester gt("command");

        TWorkerTarget &a = gt.Graph.GetTargetByName("a");

        a.Tasks[3].SetState(TS_SUCCESS);

        const TWorkerTarget &i = gt.Graph.GetTargetByName("i");

        TTargetTypeParametersMap<bool> map(&i.Type->GetParameters());
        map.At(0) = false;
        map.At(1) = false;
        map.At(2) = true;
        map.At(3) = true;

        // Applying 'RUN' command to the (--++) tasks mask of target i
        gt.Graph.Command(TCommandMessage::CF_RUN | TCommandMessage::CF_RECURSIVE_UP, i, map, TS_UNDEFINED);
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("a")), "IRR+");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("a0")), "IRRR");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("b")), "IPPR");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("c")), "ISSS");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("d")), "IIPP");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("e")), "IPPP");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("f")), "ISIS");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("g")), "IIPP");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("h")), "IPIP");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("i")), "IIPP");

        map.At(0) = false;
        map.At(1) = true;
        map.At(2) = false;
        map.At(3) = true;

        // Applying 'RESET' command to the (-+-+) tasks mask of target i
        gt.Graph.Command(TCommandMessage::CF_CANCEL | TCommandMessage::CF_INVALIDATE | TCommandMessage::CF_RECURSIVE_UP, i, map, TS_UNDEFINED);
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("a")), "IIII");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("b")), "IIII");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("c")), "IIII");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("d")), "IIPI");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("e")), "IIII");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("f")), "ISII");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("g")), "IIPI");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("h")), "IPII");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("i")), "IIPI");

        // Applying recursive-only 'RESET' command to target i
        gt.Graph.Command(TCommandMessage::CF_CANCEL | TCommandMessage::CF_INVALIDATE |
                         TCommandMessage::CF_RECURSIVE_UP | TCommandMessage::CF_RECURSIVE_ONLY, "i", TS_UNDEFINED);
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("a")), "IIII");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("b")), "IIII");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("c")), "IIII");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("d")), "IIII");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("e")), "IIII");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("f")), "IIII");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("g")), "IIII");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("h")), "IIII");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("i")), "IIPI");

        // Applying 'RUN' command to target i
        gt.Graph.Command(TCommandMessage::CF_RUN | TCommandMessage::CF_RECURSIVE_UP, "i", TS_UNDEFINED);
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("a")), "RRRR");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("b")), "PPPP");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("c")), "SSSS");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("d")), "PPPP");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("e")), "PPPP");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("f")), "SSSS");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("g")), "PPPP");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("h")), "PPPP");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("i")), "PPPP");

        // Applying non-recursive 'RESET' command to target i
        gt.Graph.Command(TCommandMessage::CF_CANCEL | TCommandMessage::CF_INVALIDATE, "i", TS_UNDEFINED);
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("a")), "RRRR");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("b")), "PPPP");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("c")), "SSSS");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("d")), "PPPP");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("e")), "PPPP");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("f")), "SSSS");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("g")), "PPPP");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("h")), "PPPP");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("i")), "IIII");

        // Applying recursive-down-only 'RESET' command to target c
        gt.Graph.Command(TCommandMessage::CF_CANCEL | TCommandMessage::CF_INVALIDATE |
                         TCommandMessage::CF_RECURSIVE_DOWN | TCommandMessage::CF_RECURSIVE_ONLY, "c", TS_UNDEFINED);
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("a")), "RRRR");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("b")), "PPPP");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("c")), "SSSS");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("d")), "PPPP");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("e")), "IIII");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("f")), "IIII");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("g")), "IIII");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("h")), "IIII");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("i")), "IIII");

        // Applying 'RESET' to everything
        gt.Graph.Command(TCommandMessage::CF_CANCEL | TCommandMessage::CF_INVALIDATE, TS_UNDEFINED);
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("a")), "IIII");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("b")), "IIII");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("c")), "IIII");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("d")), "IIII");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("e")), "IIII");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("f")), "IIII");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("g")), "IIII");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("h")), "IIII");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("i")), "IIII");

        // Applying 'RUN' to everything
        gt.Graph.Command(TCommandMessage::CF_RUN, TS_UNDEFINED);
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("a")), "RRRR");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("b")), "PPPP");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("c")), "PPPP");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("d")), "PPPP");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("e")), "PPPP");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("f")), "PPPP");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("g")), "PPPP");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("h")), "PPPP");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("i")), "PPPP");
    }

    Y_UNIT_TEST(TestCommandNonLocal) {
        TWorkerGraphTester gt("command_non_local");

        gt.Graph.Command(TCommandMessage::CF_RUN | TCommandMessage::CF_RECURSIVE_UP, "t5", TS_UNDEFINED);

        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("t1")), "R");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("t2")), "R");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("t3")), "R");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("t4")), ""); // target on other worker - no tasks
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("t_cn")), "P");
        UNIT_ASSERT_EQUAL(TaskStatesAsString(gt.Graph.GetTargetByName("t5")), "P");
    }
}

