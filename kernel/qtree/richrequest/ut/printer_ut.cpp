#include <kernel/qtree/richrequest/richnode.h>
#include <kernel/qtree/richrequest/printrichnode.h>

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(QtreePrinter) {
    TCreateTreeOptions defaultOptions;

    Y_UNIT_TEST(IgnoreMetaData) {
        TRichTreePtr tree = CreateRichTree(UTF8ToWide(TStringBuf("hello world, -this is a test tree")), defaultOptions);
        TRichRequestNode& root = *tree->Root;
        const auto& children = root.Children;

        const TUtf16String cleanTree = root.Print();

        // filters
        root.AddZoneFilter("Zone");
        root.AddRefineFilter(children[5]->Copy());
        root.AddRefineFactorFilter(children[0]->Copy(), "RefineFactor", .42f);
        root.AddRestrDocFilter(children[1]->Copy());
        root.AddAndNotFilter(children[2]->Copy());
        root.AddRestrictByPosFilter(children[3]->Copy());

        //markup
        root.AddSynonym(*children[0], *children[1], new TSynonym(u"holà", defaultOptions, 2));

        //necessity
        children[0]->Necessity = nMUSTNOT;
        children[1]->Necessity = nMUST;

        const TUtf16String cleanedTree = root.Print(PRRO_IgnoreMetaData);
        UNIT_ASSERT_VALUES_EQUAL(cleanTree, cleanedTree);
    }

    Y_UNIT_TEST(IgnoreAllSynonyms) {
        const TString query = "bank of america";
        TRichTreePtr tree = CreateRichTree(UTF8ToWide(query), defaultOptions);
        TRichRequestNode& root = *tree->Root;
        const auto& ch = root.Children;

        const TUtf16String cleanTree = root.Print();

        root.AddSynonym(*ch[0], new TSynonym(u"banker", defaultOptions, 2));
        root.AddSynonym(*ch[0], new TSynonym(u"банк", defaultOptions, 2));
        root.AddSynonym(*ch[2], new TSynonym(u"american", defaultOptions, 2));
        root.AddSynonym(*ch[2], new TSynonym(u"pro-american", defaultOptions, 2));
        root.AddSynonym(*ch[2], new TSynonym(u"америка", defaultOptions, 2));

        UNIT_ASSERT_VALUES_EQUAL(root.Print(PRRO_IgnoreMetaData), cleanTree);
    }
}
