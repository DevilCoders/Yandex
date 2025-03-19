#include <kernel/qtree/richrequest/builder/postprocess.cpp>
#include <kernel/qtree/richrequest/nodeiterator.h>
#include <kernel/qtree/richrequest/printrichnode.h>

#include <kernel/lemmer/core/langcontext.h>

#include <kernel/qtree/request/request.h>
#include <kernel/reqerror/reqerror.h>
#include <kernel/qtree/request/req_node.h>
#include <kernel/qtree/request/fixreq.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/tokenclassifiers/token_markup.h>
#include <library/cpp/tokenclassifiers/classifiers/email_classifier.h>
#include <library/cpp/tokenclassifiers/classifiers/url_classifier.h>
#include <library/cpp/tokenclassifiers/token_classifiers_singleton.h>
#include <library/cpp/langmask/langmask.h>
#include <library/cpp/archive/yarchive.h>


using namespace NTokenClassification;

void AddNode(TUtf16String src, TNodeSequence& children, TCreateTreeOptions options) {
    TRichNodePtr child = CreateRichNode(src, options);
    children.Append(child);
}

TAllNodesIterator GetChildWithDelimiter(TRichNodePtr fulltree, char delimiter) {

    TAllNodesIterator iter(fulltree);
    for (; !iter.IsDone(); ++iter) {
        const TUtf16String& text = iter->GetText();
        if (text.find(delimiter) != TUtf16String::npos) {
            return iter;
        }
    }
    return iter;
}


class TNodeUtilTest : public TTestBase {
    UNIT_TEST_SUITE(TNodeUtilTest);
        UNIT_TEST(TestSuccessReturnValue);
        UNIT_TEST(TestFailReturnValue);
        UNIT_TEST(TestSuccessRichTree);
        UNIT_TEST(TestFailRichTree);
        UNIT_TEST(TestGeneralModifyTree);
    UNIT_TEST_SUITE_END();
public:
    void TestSuccessReturnValue();
    void TestFailReturnValue();
    void TestSuccessRichTree();
    void TestFailRichTree();
    void TestGeneralModifyTree();
};


struct EmailCase {
    TRichNodePtr Fulltree;
    TRichRequestNode* Parent;
    TRichRequestNode* EmailChild;
    TNodeSequence NewChildren;
    EmailCase(TUtf16String source, TCreateTreeOptions options);
};

static char DELIMITER = 'D';

EmailCase::EmailCase(TUtf16String source, TCreateTreeOptions options) {
    Parent = nullptr;
    EmailChild = nullptr;
    using NTokenClassification::TMarkupGroup;
    using namespace NTokenClassification::NEmailClassification;
    Fulltree = CreateRichNode(source, options);
    TMultitokenMarkupGroups markupGroups;
    TAllNodesIterator iter = GetChildWithDelimiter(Fulltree, DELIMITER);
    UNIT_ASSERT_EQUAL(iter.IsDone(), false);

    if (!iter.IsDone()) {
        Parent = &iter.GetParent();
        EmailChild = &(*iter);
        const TUtf16String& text = iter->GetText();
        size_t pos = text.find(DELIMITER);
        AddNode(text.substr(0, pos), NewChildren, options);
        AddNode(text.substr(pos + 1, text.size()), NewChildren, options);
    }
}

void TNodeUtilTest::TestSuccessReturnValue() {
    EmailCase goodEmailCase(UTF8ToWide(TString("something sobakaDyandexru")), TCreateTreeOptions());
    UNIT_ASSERT_EQUAL(NRichTreeBuilder::ReplaceWithNodes(goodEmailCase.Parent, goodEmailCase.EmailChild, goodEmailCase.NewChildren), true);
}

void TNodeUtilTest::TestFailReturnValue() {
    EmailCase badEmailCase(UTF8ToWide(TString("sobakaDyandexru")), TCreateTreeOptions());
    UNIT_ASSERT_EQUAL(NRichTreeBuilder::ReplaceWithNodes(badEmailCase.Parent, badEmailCase.EmailChild, badEmailCase.NewChildren), false);
}

void TNodeUtilTest::TestSuccessRichTree() {
    TUtf16String fixedQtree = UTF8ToWide(TString("beach &/(-64 64) parrot &/(-64 64) palmru"));
    EmailCase treeToModify(UTF8ToWide(TString("beach parrotDpalmru")), TCreateTreeOptions());
    NRichTreeBuilder::ReplaceWithNodes(treeToModify.Parent, treeToModify.EmailChild, treeToModify.NewChildren);
    UNIT_ASSERT_EQUAL(PrintRichRequest(*treeToModify.Parent), fixedQtree);
}

void TNodeUtilTest::TestFailRichTree() {
    TUtf16String fixedQtree = UTF8ToWide(TString("parrotDpalm. &&/(-32768 32768) ru"));
    EmailCase treeNotToModify(UTF8ToWide(TString("parrotDpalm.ru")), TCreateTreeOptions());
    NRichTreeBuilder::ReplaceWithNodes(treeNotToModify.Parent, treeNotToModify.EmailChild, treeNotToModify.NewChildren);
    UNIT_ASSERT_EQUAL(PrintRichRequest(*treeNotToModify.Parent), fixedQtree);
}

void TNodeUtilTest::TestGeneralModifyTree() {
    TCreateTreeOptions options;
    TRichRequestNode* parent = nullptr;
    TRichNodePtr originalFullTree = CreateRichNode(UTF8ToWide(TString("first secondDevilDanything")), options);
    TNodeSequence testChildren;

    AddNode(UTF8ToWide(TString("remedy")), testChildren, options);
    AddNode(UTF8ToWide(TString("for")), testChildren, options);
    AddNode(UTF8ToWide(TString("past")), testChildren, options);

    TRichRequestNode* nodeToReplace = nullptr;
    for (TWordIterator iter(originalFullTree); !iter.IsDone(); ++iter) {
        const TUtf16String& text = iter->GetText();
        if (text.find(DELIMITER) != TUtf16String::npos) {
            parent = &iter.GetParent();
            nodeToReplace = &(*iter);
            break;
        }
    }
    TUtf16String nodeToReplaceWtr = UTF8ToWide(TString("secondDevilDanything"));
    TUtf16String result = UTF8ToWide(TString("first &/(-64 64) remedy &/(-64 64) for &/(-64 64) past"));
    UNIT_ASSERT_EQUAL(PrintRichRequest(*nodeToReplace), nodeToReplaceWtr);
    NRichTreeBuilder::ReplaceWithNodes(parent, nodeToReplace, testChildren);
    UNIT_ASSERT_EQUAL(PrintRichRequest(*originalFullTree), result);
}

class TAddEmailSynonymTest : public TTestBase {
    UNIT_TEST_SUITE(TAddEmailSynonymTest);
        UNIT_TEST(CheckSplitEmail);
    UNIT_TEST_SUITE_END();
public:
    void CheckSplitEmail();
    void CheckSplitEmailTreesNoChildren();
    void CheckSplitEmailTreesWithChildren();
    void CheckQTreeConsistency(TUtf16String requestWtr, const TString& no_email_qtree_file, const TString& email_qtree_file);
};

void TAddEmailSynonymTest::CheckSplitEmail() {
    TCreateTreeOptions options;
    options.Reqflags |= RPF_SPLIT_EMAILS;
    options.Reqflags |= RPF_USE_TOKEN_CLASSIFIER;
    TUtf16String resultTree = UTF8ToWide(TString("my &&/(-32768 32768) split &/(-64 64) evil &/(-64 64) yandex &/(1 1) ru"));
    TRichNodePtr originalFullTree = CreateRichNode(UTF8ToWide(TString("my split evil@yandex.ru")), options);
    Cout << PrintRichRequest(*originalFullTree) << Endl;
    UNIT_ASSERT_EQUAL(PrintRichRequest(*originalFullTree), resultTree);
}


UNIT_TEST_SUITE_REGISTRATION(TNodeUtilTest);
UNIT_TEST_SUITE_REGISTRATION(TAddEmailSynonymTest);

