#include "target_graph_parser.h"

#include <tools/clustermaster/common/vector_to_string.h>

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(TTargetGraphParserTest) {
    Y_UNIT_TEST(TestSimple) {
        TTargetGraphParsed graph = TTargetGraphParsed::Parse(
                "_scenario() {\n"
                "AA = h h h\n"
                "AA first\n"
                "AA second: ^ [1->0,2->1]\n"
                "}\n"
            );

        UNIT_ASSERT_VALUES_EQUAL(size_t(3), graph.Nodes.size());

        TVector<TTargetParsed> targets = graph.GetTargets();

        UNIT_ASSERT_VALUES_EQUAL(size_t(2), targets.size());

        const TTargetParsed& second = targets.at(1);
        UNIT_ASSERT_VALUES_EQUAL("second", second.Name);

        UNIT_ASSERT_VALUES_EQUAL(size_t(1), second.Depends.size());

        const TTargetParsed::TDepend& depend = second.Depends.at(0);
        UNIT_ASSERT(depend.Name.empty());

        const TVector<TTargetParsed::TParamMapping>& paramMappings = *depend.ParamMappings;
        UNIT_ASSERT_VALUES_EQUAL(size_t(2), paramMappings.size());
    }

    Y_UNIT_TEST(TestIncorrectMapping) {
        const char* script =
                "_scenario() {\n"
                "AA = h h h\n"
                "AA first\n"
                "AA second: ^ [1<-0]\n"
                "}\n"
                ;
        UNIT_ASSERT_EXCEPTION(TTargetGraphParsed::Parse(script), yexception);
    }
}
