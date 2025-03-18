#include "make_vector.h"
#include "target_graph_impl.h"
#include "target_impl.h"
#include "target_type.h"

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(TargetGraphTest) {
    Y_UNIT_TEST_DECLARE(RangeParser);
    Y_UNIT_TEST_DECLARE(GraphParserWhitespace);
    Y_UNIT_TEST_DECLARE(GraphParserComments);
    Y_UNIT_TEST_DECLARE(GraphParserTokens);

    Y_UNIT_TEST_DECLARE(GraphParserTargetType);
    Y_UNIT_TEST_DECLARE(GraphParserTargetTypeLists);
    Y_UNIT_TEST_DECLARE(GraphParserTargetTypeOptions);
    Y_UNIT_TEST_DECLARE(GraphParserTargetTypeErrors);

    Y_UNIT_TEST_DECLARE(GraphParserTarget);
    Y_UNIT_TEST_DECLARE(GraphParserTargetDepends);
    Y_UNIT_TEST_DECLARE(GraphParserTargetOptions);
    Y_UNIT_TEST_DECLARE(GraphParserTargetConditionalDepends);
    Y_UNIT_TEST_DECLARE(GraphParserTargetConditionalDependsCorrect);
    Y_UNIT_TEST_DECLARE(GraphParserTargetErrors);
}

class TTestGraph;
class TTestTarget;
class TTestTargetType;
class TTestListManager;
class TTestDepend;
struct TTestPrecomputedTaskIdsContainer;

struct TTestGraphTypes {
    typedef TTestGraph TGraph;
    typedef TTestTarget TTarget;
    typedef TTestTargetType TTargetType;
    typedef TTestListManager TListManager;
    typedef TDependBase<TTestGraphTypes> TDepend;
    typedef bool TSpecificTaskStatus;
    typedef TTestPrecomputedTaskIdsContainer TPrecomputedTaskIdsContainer;
};

struct TTestPrecomputedTaskIdsContainer {
    TPrecomputedTaskIdsContainer<TTestGraphTypes, TParamsByTypeOrdinary> Container;
};

class TTestListManager: public IListManager {
public:
    void GetListForVariable(const TString&, TVector<TString>&) const {
    }
};

namespace {
    TTestListManager listManager;
}

class TTestTargetType: public TTargetTypeBase<TTestGraphTypes> {
    TTargetTypeParameters Parameters;

public:
    TTestTargetType(TTestGraph* graph, const TConfigMessage*, const TString& name,
                const TVector<TVector<TString> >& paramss, TTestListManager*);

    const TTargetTypeParameters& GetParameters() const {
        return Parameters;
    }
};

class TTestTarget: public TTargetBase<TTestGraphTypes> {
public:
    TTestTarget(const TString &n, TTestTargetType* t, const TParserState& parserState)
        : TTargetBase<TTestGraphTypes>(n, t, parserState)
    {
    }
};

class TTestGraph: public TTargetGraphBase<TTestGraphTypes> {
public:
    TTestGraph()
        : TTargetGraphBase<TTestGraphTypes>(&listManager, false)
    {
    }

    Y_UNIT_TEST_FRIEND(TargetGraphTest, RangeParser);
    Y_UNIT_TEST_FRIEND(TargetGraphTest, GraphParserWhitespace);
    Y_UNIT_TEST_FRIEND(TargetGraphTest, GraphParserComments);
    Y_UNIT_TEST_FRIEND(TargetGraphTest, GraphParserTokens);

    Y_UNIT_TEST_FRIEND(TargetGraphTest, GraphParserTargetType);
    Y_UNIT_TEST_FRIEND(TargetGraphTest, GraphParserTargetTypeLists);
    Y_UNIT_TEST_FRIEND(TargetGraphTest, GraphParserTargetTypeOptions);
    Y_UNIT_TEST_FRIEND(TargetGraphTest, GraphParserTargetTypeErrors);

    Y_UNIT_TEST_FRIEND(TargetGraphTest, GraphParserTarget);
    Y_UNIT_TEST_FRIEND(TargetGraphTest, GraphParserTargetDepends);
    Y_UNIT_TEST_FRIEND(TargetGraphTest, GraphParserTargetOptions);
    Y_UNIT_TEST_FRIEND(TargetGraphTest, GraphParserTargetConditionalDepends);
    Y_UNIT_TEST_FRIEND(TargetGraphTest, GraphParserTargetConditionalDependsCorrect);
    Y_UNIT_TEST_FRIEND(TargetGraphTest, GraphParserTargetErrors);
};

TTestTargetType::TTestTargetType(TTestGraph* graph, const TConfigMessage*, const TString& name,
        const TVector<TVector<TString> >& paramss, TTestListManager*)
    : TTargetTypeBase<TTestGraphTypes>(graph, name, paramss)
    , Parameters(name, &Graph->GetParamListManager(), MakeVector<TParamListManager::TListReference>())
{
    Parameters.CompleteBuild();
}



Y_UNIT_TEST_SUITE_IMPLEMENTATION(TargetGraphTest) {
    void GraphParserCreator(TTestGraph* g, ...) {
        TString config;

        config += "#!/bin/sh\n";
        config += "_scenario() {\n";

        const char* arg;
        va_list args;
        va_start(args, g);
        while (arg = va_arg(args, const char *)) {
            config += arg;
            config += "\n";
        }
        config += "}\n";
        va_end(args);

        TConfigMessage m;
        m.SetConfig(config);

        g->ParseConfig(&m);
    }

#if 0
    Y_UNIT_TEST(RangeParser) {
        TTestGraph g;

        TVector<TString> out;
        UNIT_ASSERT(g.ParseRange("0", "9", out));
        UNIT_ASSERT_EQUAL(out.size(), 10);
        UNIT_ASSERT_EQUAL(out.front(), "0");
        UNIT_ASSERT_EQUAL(out.back(), "9");

        out.clear();
        UNIT_ASSERT(g.ParseRange("test00", "test99", out));
        UNIT_ASSERT_EQUAL(out.size(), 100);
        UNIT_ASSERT_EQUAL(out.front(), "test00");
        UNIT_ASSERT_EQUAL(out.back(), "test99");

        out.clear();
        UNIT_ASSERT(g.ParseRange("00test", "99test", out));
        UNIT_ASSERT_EQUAL(out.size(), 100);
        UNIT_ASSERT_EQUAL(out.front(), "00test");
        UNIT_ASSERT_EQUAL(out.back(), "99test");

        out.clear();
        UNIT_ASSERT(g.ParseRange("zm000.yandex.ru:10000", "zm999.yandex.ru:10000", out));
        UNIT_ASSERT_EQUAL(out.size(), 1000);
        UNIT_ASSERT_EQUAL(out.front(), "zm000.yandex.ru:10000");
        UNIT_ASSERT_EQUAL(out.back(), "zm999.yandex.ru:10000");

        out.clear();
        UNIT_ASSERT(g.ParseRange("zm000.yandex.ru:10000", "zm000.yandex.ru:10999", out));
        UNIT_ASSERT_EQUAL(out.size(), 1000);
        UNIT_ASSERT_EQUAL(out.front(), "zm000.yandex.ru:10000");
        UNIT_ASSERT_EQUAL(out.back(), "zm000.yandex.ru:10999");

        out.clear();
        UNIT_ASSERT(g.ParseRange("somestring", "somestring", out));
        UNIT_ASSERT_EQUAL(out.size(), 1);
        UNIT_ASSERT_EQUAL(out.front(), "somestring");

        UNIT_ASSERT(!g.ParseRange("somestring", "othrstring", out));
        UNIT_ASSERT(!g.ParseRange("test00test", "test999test", out));
        UNIT_ASSERT(!g.ParseRange("0", "99", out));
        UNIT_ASSERT(!g.ParseRange("00", "9", out));
        UNIT_ASSERT(!g.ParseRange("foo00bar", "foo99baz", out));
        UNIT_ASSERT(!g.ParseRange("foo99", "foo00", out));
    }
#endif

    Y_UNIT_TEST(GraphParserWhitespace) {
        TTestGraph g;

        GraphParserCreator(&g, "TYPE1 = host1", "", "   ", "  ", "TYPE2      =    host2", NULL);
        UNIT_ASSERT(g.Types.find("TYPE1") != g.Types.end());
        UNIT_ASSERT(g.Types.find("TYPE2") != g.Types.end());
    }

    Y_UNIT_TEST(GraphParserComments) {
        TTestGraph g;

        GraphParserCreator(&g,
                "TYPE = host # comment",
                "TYPE target # comment",
                "# comment",
                "#comment",
                " #idented",
                "  #idented",
                "# curly should be ignored -> } {}{}{}",
                NULL);
        UNIT_ASSERT(g.Types.find("TYPE") != g.Types.end());
        UNIT_ASSERT(g.Targets.find("target") != g.Targets.end());
    }

    Y_UNIT_TEST(GraphParserTokens) {
        TTestGraph g;

        GraphParserCreator(&g, "FOO_TYPE-123 = mymegahost.yandex-mega-domain.ru __some-complex.cluster_name__", NULL);

        TTestGraph::TTargetTypesList::const_iterator type = g.Types.find("FOO_TYPE-123");
        UNIT_ASSERT(type != g.Types.end());
        UNIT_ASSERT_EQUAL((*type)->GetName(), "FOO_TYPE-123");
        //UNIT_ASSERT_EQUAL((*type)->GetHosts().size(), 1);
        //UNIT_ASSERT_EQUAL((*type)->GetHosts().front(), "mymegahost.yandex-mega-domain.ru");
        //UNIT_ASSERT_EQUAL((*type)->Clusters.size(), 1);
        //UNIT_ASSERT_EQUAL((*type)->Clusters.front(), "__some-complex.cluster_name__");
    }

    Y_UNIT_TEST(GraphParserTargetType) {
        TTestGraph g;

        GraphParserCreator(&g, "FOO = somehost", NULL);
        UNIT_ASSERT_EQUAL(g.Types.size(), 1);
        UNIT_ASSERT_EQUAL(g.Types.size(), 1);

        TTestGraph::TTargetTypesList::const_iterator type = g.Types.find("FOO");
        UNIT_ASSERT(type != g.Types.end());
        UNIT_ASSERT_EQUAL((*type)->GetName(), "FOO");
        //UNIT_ASSERT_EQUAL((*type)->GetHosts().size(), 1);
        //UNIT_ASSERT_EQUAL((*type)->GetHosts().front(), "somehost");
        //UNIT_ASSERT_EQUAL((*type)->Clusters.size(), 1);
        //UNIT_ASSERT_EQUAL((*type)->Clusters.front(), "");
    }

    Y_UNIT_TEST(GraphParserTargetTypeLists) {
        TTestGraph g;

        GraphParserCreator(&g, "FOO = host0, host1..host9, host10..host99,host100, host101 c0, c1..c9,c10..c99, c100,c101", NULL);
        UNIT_ASSERT_EQUAL(g.Types.size(), 1);
        UNIT_ASSERT_EQUAL(g.Types.size(), 1);

        TTestGraph::TTargetTypesList::const_iterator type = g.Types.find("FOO");
        UNIT_ASSERT(type != g.Types.end());
        //UNIT_ASSERT_EQUAL((*type)->GetHosts().size(), 102);
        //UNIT_ASSERT_EQUAL((*type)->GetHosts().front(), "host0");

        //UNIT_ASSERT_EQUAL((*type)->GetHosts().back(), "host101");

        //UNIT_ASSERT_EQUAL((*type)->Clusters.size(), 102);
        //UNIT_ASSERT_EQUAL((*type)->Clusters.front(), "c0");
        //UNIT_ASSERT_EQUAL((*type)->Clusters.back(), "c101");
    }

    Y_UNIT_TEST(GraphParserTargetTypeOptions) {
        TTestGraph g;

        GraphParserCreator(&g, "FOO = host option1=123123 option2=nobody@nowhere.com option3= #empty", NULL);
        UNIT_ASSERT_EQUAL(g.Types.size(), 1);
        UNIT_ASSERT_EQUAL(g.Types.size(), 1);

        TTestGraph::TTargetTypesList::const_iterator type = g.Types.find("FOO");
        UNIT_ASSERT(type != g.Types.end());
        //UNIT_ASSERT_EQUAL((*type)->GetHosts().size(), 1);
        //UNIT_ASSERT_EQUAL((*type)->GetHosts().front(), "host");
        //UNIT_ASSERT_EQUAL((*type)->Clusters.size(), 1);
        //UNIT_ASSERT_EQUAL((*type)->Clusters.front(), "");

        TTestTargetType::TOptionsMap::const_iterator option = (*type)->Options.find("option1");
        UNIT_ASSERT(option != (*type)->Options.end());
        UNIT_ASSERT_EQUAL(option->second, "123123");

        option = (*type)->Options.find("option2");
        UNIT_ASSERT(option != (*type)->Options.end());
        UNIT_ASSERT_EQUAL(option->second, "nobody@nowhere.com");

        option = (*type)->Options.find("option3");
        UNIT_ASSERT(option != (*type)->Options.end());
        UNIT_ASSERT_EQUAL(option->second, "");
    }

    Y_UNIT_TEST(GraphParserTargetTypeErrors) {
        TTestGraph g;

        UNIT_ASSERT_EXCEPTION(GraphParserCreator(&g, "FOO", NULL), yexception);
        UNIT_ASSERT_EXCEPTION(GraphParserCreator(&g, "FOO =", NULL), yexception);
        //UNIT_ASSERT_EXCEPTION(GraphParserCreator(&g, "FOO = !unknown_table", NULL), yexception);
        UNIT_ASSERT_EXCEPTION(GraphParserCreator(&g, "FOO = host", "FOO = host", NULL), yexception);
        UNIT_ASSERT_EXCEPTION(GraphParserCreator(&g, "FOO = host1..host0", NULL), yexception);
        UNIT_ASSERT_EXCEPTION(GraphParserCreator(&g, "FOO = aaaa0..bbbb0", NULL), yexception);
        UNIT_ASSERT_EXCEPTION(GraphParserCreator(&g, "FOO = host1..", NULL), yexception);
        UNIT_ASSERT_EXCEPTION(GraphParserCreator(&g, "FOO = ..host1", NULL), yexception);
    }

    Y_UNIT_TEST(GraphParserMultiparameters) {
        TTestGraph g;
        GraphParserCreator(&g, "FOO = host cluster wtf", NULL);
        TTestTargetType type = g.GetTargetTypeByName("FOO");
        UNIT_ASSERT_VALUES_EQUAL(ui32(2), type.GetParamCount());
    }

    Y_UNIT_TEST(GraphParserTarget) {
        TTestGraph g;

        GraphParserCreator(&g, "TYPE = host", "TYPE target", NULL);
        UNIT_ASSERT_EQUAL(g.Targets.size(), 1);
        UNIT_ASSERT_EQUAL(g.Targets.size(), 1);

        TTestGraph::TTargetsList::const_iterator target = g.Targets.find("target");
        UNIT_ASSERT(target != g.Targets.end());
        UNIT_ASSERT_EQUAL((*target)->Name, "target");
        UNIT_ASSERT_EQUAL((*target)->Type->GetName(), "TYPE");
    }

    Y_UNIT_TEST(GraphParserTargetDepends) {
        TTestGraph g;

        GraphParserCreator(&g,
                "TYPE = host",
                "OTHERTYPE = otherhost",
                "TYPE foo",
                "TYPE bar               # implicit",
                "TYPE t1:               # none",
                "TYPE t2:               # none",
                "TYPE exp1: t1          # explicit",
                "TYPE exp2 : t1         # explicit",
                "TYPE exp3 :t1          # explicit",
                "TYPE up : ^            # up",
                "TYPE mul : ^ t1 t2     # multiple",
                "TYPE cn1 []             # crossnode",
                "TYPE cn2:^[] t1[]        # crossnode",
                "TYPE cn3 : ^ [] t2 []    # crossnode",
                "OTHERTYPE other        # crossnode",
                "TYPE back: ^           # crossnode",
                NULL);

        UNIT_ASSERT_EQUAL(g.Types.size(), 2);
        UNIT_ASSERT_EQUAL(g.Types.size(), 2);

        UNIT_ASSERT_EQUAL(g.Targets.size(), 14);
        UNIT_ASSERT_EQUAL(g.Targets.size(), 14);

        TTestTarget* target = &g.GetTargetByName("foo");
        UNIT_ASSERT_EQUAL(target->Depends.size(), 0);
        UNIT_ASSERT_EQUAL(target->Followers.size(), 1);
        UNIT_ASSERT_EQUAL(target->Followers.front().GetTarget()->Name, "bar");

        target = &g.GetTargetByName("bar");
        UNIT_ASSERT_EQUAL(target->Followers.size(), 0);
        UNIT_ASSERT_EQUAL(target->Depends.size(), 1);
        UNIT_ASSERT_EQUAL(target->Depends.front().GetTarget()->Name, "foo");

        target = &g.GetTargetByName("t1");
        UNIT_ASSERT_EQUAL(target->Depends.size(), 0);

        target = &g.GetTargetByName("t2");
        UNIT_ASSERT_EQUAL(target->Depends.size(), 0);

        target = &g.GetTargetByName("exp1");
        UNIT_ASSERT_EQUAL(target->Depends.size(), 1);
        UNIT_ASSERT_EQUAL(target->Depends.front().GetTarget()->Name, "t1");

        target = &g.GetTargetByName("exp2");
        UNIT_ASSERT_EQUAL(target->Depends.size(), 1);
        UNIT_ASSERT_EQUAL(target->Depends.front().GetTarget()->Name, "t1");

        target = &g.GetTargetByName("exp3");
        UNIT_ASSERT_EQUAL(target->Depends.size(), 1);
        UNIT_ASSERT_EQUAL(target->Depends.front().GetTarget()->Name, "t1");

        target = &g.GetTargetByName("up");
        UNIT_ASSERT_EQUAL(target->Depends.size(), 1);
        UNIT_ASSERT_EQUAL(target->Depends.front().GetTarget()->Name, "exp3");

        target = &g.GetTargetByName("mul");
        UNIT_ASSERT_EQUAL(target->Depends.size(), 3);

        target = &g.GetTargetByName("cn1");
        UNIT_ASSERT_EQUAL(target->Depends.size(), 1);
        UNIT_ASSERT_EQUAL(target->Depends.front().GetTarget()->Name, "mul");
#if 0 // disabled because DF_CROSSNODE is no longer set
        UNIT_ASSERT(target->Depends.front().Flags & DF_CROSSNODE);
#endif

        target = &g.GetTargetByName("cn2");
        UNIT_ASSERT_EQUAL(target->Depends.size(), 2);
        UNIT_ASSERT_EQUAL(target->Depends.front().GetTarget()->Name, "cn1");
#if 0
        UNIT_ASSERT(target->Depends.front().Flags & DF_CROSSNODE);
#endif
        UNIT_ASSERT_EQUAL(target->Depends.back().GetTarget()->Name, "t1");
#if 0
        UNIT_ASSERT(target->Depends.back().Flags & DF_CROSSNODE);
#endif

        target = &g.GetTargetByName("cn3");
        UNIT_ASSERT_EQUAL(target->Depends.size(), 2);
        UNIT_ASSERT_EQUAL(target->Depends.front().GetTarget()->Name, "cn2");
#if 0
        UNIT_ASSERT(target->Depends.front().Flags & DF_CROSSNODE);
#endif
        UNIT_ASSERT_EQUAL(target->Depends.back().GetTarget()->Name, "t2");
#if 0
        UNIT_ASSERT(target->Depends.back().Flags & DF_CROSSNODE);
#endif

#if 0 // disabled because crossnode is now detected on master and sent to worker
        target = &g.GetTargetByName("other");
        UNIT_ASSERT_EQUAL(target->Depends.size(), 1);
        UNIT_ASSERT_EQUAL(target->Depends.front().GetTarget()->Name, "cn3");
        UNIT_ASSERT(target->Depends.front().Flags & DF_CROSSNODE);

        target = &g.GetTargetByName("back");
        UNIT_ASSERT_EQUAL(target->Depends.size(), 1);
        UNIT_ASSERT_EQUAL(target->Depends.front().GetTarget()->Name, "other");
        UNIT_ASSERT(target->Depends.front().Flags & DF_CROSSNODE);
#endif
    }

    Y_UNIT_TEST(GraphParserTargetOptions) {
        TTestGraph g;

        GraphParserCreator(&g, "FOO = host", "FOO target option1=123123 option2=nobody@nowhere.com option3= #empty", NULL);
        UNIT_ASSERT_EQUAL(g.Targets.size(), 1);
        UNIT_ASSERT_EQUAL(g.Targets.size(), 1);

        TTestGraph::TTargetsList::const_iterator target = g.Targets.find("target");
        UNIT_ASSERT(target != g.Targets.end());

        // No way to check options for generic graph yet
    }

    Y_UNIT_TEST(GraphParserTargetConditionalDepends) {
        TTestGraph g;

        GraphParserCreator(&g,
                "BAR := foo",
                "TYPE = host",
                "TYPE base",
                "TYPE target1 : base ?FOO",
                "TYPE target2 : base ?!FOO",
                "TYPE target3 : base ?FOO~bar",
                "TYPE target4 : base ?!FOO~bar",
                "TYPE target5 : base ?!_SOME_VAR_123_name_~_sophisticated_000_value_",
                "TYPE target6 : base ?FOO=bar",
                "TYPE target7 : base ?!FOO=bar",
                "TYPE target8 : base ?FOO=$BAR",
                "TYPE target9 : base ?!FOO=$BAR",
                NULL);
    }

    Y_UNIT_TEST(GraphParserTargetConditionalDependsCorrect) {
        TTestGraph g;

        GraphParserCreator(&g,
                "TYPE = host",
                "TYPE a",
                "TYPE b:",
                "TYPE c: a ?FOO b ?!FOO",
                NULL);

        TTestGraph::TTargetsList::const_iterator target;
        UNIT_ASSERT((target = g.Targets.find("a")) != g.Targets.end());
        UNIT_ASSERT((*target)->Followers.front().GetCondition().GetOriginal() == "FOO");

        UNIT_ASSERT((target = g.Targets.find("b")) != g.Targets.end());
        UNIT_ASSERT((*target)->Followers.front().GetCondition().GetOriginal() == "!FOO");

        UNIT_ASSERT((target = g.Targets.find("c")) != g.Targets.end());
        UNIT_ASSERT((*target)->Depends.front().GetCondition().GetOriginal() == "FOO");
        UNIT_ASSERT((*target)->Depends.back().GetCondition().GetOriginal() == "!FOO");
    }

    Y_UNIT_TEST(GraphParserTargetErrors) {
        TTestGraph g;

        UNIT_ASSERT_EXCEPTION(GraphParserCreator(&g, "TYPE = host", "TYPE", NULL), yexception);
        UNIT_ASSERT_EXCEPTION(GraphParserCreator(&g, "TYPE = host", "TYPO foo", NULL), yexception);
        UNIT_ASSERT_EXCEPTION(GraphParserCreator(&g, "TYPE = host", "TYPE foo wtf", NULL), yexception);
        UNIT_ASSERT_EXCEPTION(GraphParserCreator(&g, "TYPE = host", "TYPE foo: nosuchtarget", NULL), yexception);
        UNIT_ASSERT_EXCEPTION(GraphParserCreator(&g, "TYPE = host", "TYPE foo", "TYPE foo", NULL), yexception);
        UNIT_ASSERT_EXCEPTION(GraphParserCreator(&g, "TYPE = host", "TYPE foo", "TYPE bar: ^ foo", NULL), yexception);
        UNIT_ASSERT_EXCEPTION(GraphParserCreator(&g, "TYPE = host", "TYPE foo", "TYPE bar: foo foo", NULL), yexception);
    }
}
