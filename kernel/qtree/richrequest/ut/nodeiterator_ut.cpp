#include <kernel/qtree/richrequest/nodeiterator.h>

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(QtreeIterators) {
    TCreateTreeOptions defaultOptions;

    TRichTreePtr MakeQtree() {
        TRichTreePtr ans = CreateRichTree(UTF8ToWide(TStringBuf("(hello | heelo | hola) (world|(mundo|monde)|all), -this is (a|the) test tree")), defaultOptions);
        const int manualExtension = 2;
        ans->Root->AddSynonym(*ans->Root->Children[1], new TSynonym(u"pays", defaultOptions, manualExtension));
        return ans;
    }

    Y_UNIT_TEST(WordIterator) {
        const auto tree = MakeQtree();
        TWordIterator it(tree->Root);
        const size_t undefined = size_t(-1);
        const TVector<std::pair<TString, TVector<size_t>>> expected = {
            {"hello", {0,0}},
            {"heelo", {0,1}},
            {"hola", {0,2}},
            {"world", {1,0}},
            {"mundo", {1,1,0}},
            {"monde", {1,1,1}},
            {"all", {1,2}},
            {"pays", {undefined}},
            {"this", {2}},
            {"is", {3}},
            {"a", {4,0}},
            {"the", {4,1}},
            {"test", {5}},
            {"tree", {6}}
        };
        const auto& leaves = it.CollectLeaves();
        UNIT_ASSERT_VALUES_EQUAL(leaves.size(), expected.size());
        for (size_t i = 0; i < leaves.size(); ++i) {
            UNIT_ASSERT_VALUES_EQUAL(WideToUTF8(leaves[i].Leaf->Print()), expected[i].first);
            if(expected[i].second && expected[i].second.front() == undefined)
                continue;
            UNIT_ASSERT_VALUES_EQUAL(leaves[i].Path, expected[i].second);
        }
    }

    Y_UNIT_TEST(UserWordsIterator) {
        const auto tree = MakeQtree();
        TUserWordsIterator it(tree->Root);
        const TVector<std::pair<TString, TVector<size_t>>> expected = {
            {"hello", {0,0}},
            {"heelo", {0,1}},
            {"hola", {0,2}},
            {"world", {1,0}},
            {"mundo", {1,1,0}},
            {"monde", {1,1,1}},
            {"all", {1,2}},
            {"this", {2}},
            {"is", {3}},
            {"a", {4,0}},
            {"the", {4,1}},
            {"test", {5}},
            {"tree", {6}}
        };
        const auto& leaves = it.CollectLeaves();
        UNIT_ASSERT_VALUES_EQUAL(leaves.size(), expected.size());
        for (size_t i = 0; i < leaves.size(); ++i) {
            UNIT_ASSERT_VALUES_EQUAL(WideToUTF8(leaves[i].Leaf->Print()), expected[i].first);
            UNIT_ASSERT_VALUES_EQUAL(leaves[i].Path, expected[i].second);
        }
    }
}
