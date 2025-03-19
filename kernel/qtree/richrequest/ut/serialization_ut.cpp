#include <kernel/qtree/richrequest/richnode.h>
#include <kernel/qtree/richrequest/serialization/serializer.h>

#include <library/cpp/testing/unittest/registar.h>

class TRichrequestSerializationTest : public TTestBase {
    UNIT_TEST_SUITE(TRichrequestSerializationTest);
        UNIT_TEST(TestTextRequest);
        UNIT_TEST(TestSynonyms);
        UNIT_TEST(TestDeserializesWithUnserializeFlag);
    UNIT_TEST_SUITE_END();

    void TestTextRequest() const {
        TestTextRequest("два слова");
        TestTextRequest("Санкт-Петербург, Комендантский пр-т");
        TestTextRequest("(телефон ~~ iphone) | (телефон не &&/(1 1) iphone) | (телефон \"не iphone\")");
        TestTextRequest("Санкт-Петербург, Комендантский пр-т");
        TestTextRequest("мазда ^ toyota lancer");
        TestTextRequest("Имя Фамилия <- finame:((имя фамилия))");
        TestTextRequest("Имя Фамилия <- refinefactor:FioMatch finame:((имя фамилия))");
    }

    void TestSynonyms() const {
        TCreateTreeOptions opts(~TLangMask());

        TRichTreePtr richTree(CreateRichTree(u"мазда lancer mercedes", opts));
        TRichNodePtr s0 = CreateRichNode(u"toyota", opts);
        TRichNodePtr s1 = CreateRichNode(u"mitsubishi", opts);
        TRichNodePtr s2 = CreateRichNode(u"volkswagen", opts);

        TRichNodePtr s01 = CreateRichNode(u"japan", opts);
        TRichNodePtr s12 = CreateRichNode(u"китай", opts);

        TRichNodePtr s2_ = CreateRichNode(u"мерседес умерла", opts);

        TRichNodePtr s02 = CreateRichNode(u"машинки всякие", opts);

        richTree->Root->AddMarkup(0, 0, new TSynonym(s0, TE_TRANSLIT));
        richTree->Root->AddMarkup(1, 1, new TSynonym(s1, TE_DERIV));
        richTree->Root->AddMarkup(2, 2, new TSynonym(s2, TE_MANUAL_DERIV));

        richTree->Root->AddMarkup(0, 1, new TSynonym(s01, TE_TRANSLIT));
        richTree->Root->AddMarkup(1, 2, new TSynonym(s12, TE_DERIV));

        richTree->Root->AddMarkup(0, 2, new TSynonym(s02, TE_DERIV));
        richTree->Root->AddMarkup(2, 2, new TSynonym(s2_, TE_MANUAL_DERIV));

        TestTree(richTree, "synonyms request");
    }

    void TestDeserializesWithUnserializeFlag() const {
        {
            const TString qtree = "cHic5ZPPa9RQEMfnO8muj9dtCVsWtk_EGA8NxUIQAlKoyIKwqGjYQ5U91aKweyhChbLspQteiqCIPQgFL"
                "4XirYjgUZCC4E3Bgyf_A6969r3kJcQ1_ribS9h5M_M-mc-OvCIbYtaba5OPkCNqkqKAlug8rTQEeaTjFFJEl2rdWkK3aL0-wBP"
                "QPugA9Ar0BqSf96CPuKNeQ96UWZnQZaZdfTS8fW9zqOAXXeulrtQl03Xw_ctS3tVWTPWO6AI6iYBHyiYEFOqqCMkeaywaYGtZ2"
                "qM2hdAF1OPdoz4ON_SNisL5DaHfvHg_dAYY8ZgFT0D6WrUP2dPc7HHB7WwOYz0J-BbaqYB---5zLac2-VXIKymyOdW8pkADP3U"
                "t8ElpDgpaykEHYkRjyuCCWbHjTdBm3wlrEZmm_WeOmEy8lrlKPXBkV-aq-F8U_kHgc8gbUwJZyyvNgSvmcPCh6MhV4pzORT0FK"
                "E6l6eyytNOSU2GUjgC7fT48KuZAmaZ1WFE78toUHuLSxyKFw-_gUGGIO2f1N0EhDmai4onDY6EhKWMcUw4QZH8S4wKZCxt3TTy"
                "k_ouSmb3_ycxjyKvTatZ-UfMz3ddzhZm1qt2J093BdmAWBMknWLAFie3KpSnt9F9MufrXQ5yRriFuLgiolqi3Hh1_Wz21ePnlq"
                "k_LhiXSOfNpDjdnhOidENx0rid3sygXUdhow_ZzxdyWuewHnyLZng,,";
            TRichTreeDeserializer d;
            NSearchQuery::TRequest r;
            d.DeserializeBase64(qtree, r);
            UNIT_ASSERT(r.Root);
            UNIT_ASSERT_VALUES_EQUAL(r.Root->DebugString(), R"("" -> ["xiaomi", "mi5"])");
            d.DeserializeBase64(qtree, r, QTREE_UNSERIALIZE);
            UNIT_ASSERT(r.Root);
            UNIT_ASSERT_VALUES_EQUAL(r.Root->DebugString(), R"("" -> ["xiaomi", "mi", "5"])");
        }
        {
            const TString qtree = "cHicdZI_S8RAEMXfbJJzXU8IihC2ilsFq8TqsNLYBGwOKxGLIBaprUQQgq2NtpaKpY02IlxvYZFr_RBWfgB38"
                "__udLrdmXnz482IVAw5c5nHfQQsxNpy8TbNi9fpTSShsIVt7Ay548KDLkCIXSQY4wjpIPv8_nlfuiXcEx6o63shTAg6PggFnYUYUbz"
                "PyYXsahQCCml8bWshZHS-KbqcZ3IY4RCnA92EYD3jF7jE8ZPF89zdMNLyzhKJJkcJxkpy9IDRA3YSxwBn1KBijlA-kzipxRobeIPTE"
                "7UXXZj1oG1atMCK97QFJNuS2oFHVjvgizb1nwEp5QQ9S16Jgzlc6m-LSk6a4fxabSDpjwWxWOkjIEmRWgnbiEpCVIDdeFUdDPMtg2n"
                "WW__bBilA9foFC6N9PQ,,";

            TRichTreeDeserializer d;
            NSearchQuery::TRequest r;
            d.DeserializeBase64(qtree, r);
            UNIT_ASSERT(r.Root);
            UNIT_ASSERT_VALUES_EQUAL(r.Root->DebugString(), R"("иргы1")");
            d.DeserializeBase64(qtree, r, QTREE_UNSERIALIZE);
            UNIT_ASSERT(r.Root);
            UNIT_ASSERT_VALUES_EQUAL(r.Root->DebugString(), R"("" -> ["иргы", "1"])");
        }
    }
private:
    void TestTextRequest(const char* text) const {
        TRichTreePtr richTree(CreateRichTree(UTF8ToWide(text), TCreateTreeOptions(~TLangMask())));
        TestTree(richTree, text);
    }

    void TestTree(TRichTreePtr richTree, const char* text) const {
        TString msg("[");
        msg += text;
        msg += "]";
        TestTreeInt(richTree, msg);

        msg += " (updated)";
        UpdateRichTree(richTree->Root);
        TestTreeInt(richTree, msg);
    }

    void TestTreeInt(const TRichTreePtr& richTree, const TString& msg) const {
        TBinaryRichTree buffer;
        TRichTreePtr binaryClone;
        richTree->Serialize(buffer);
        binaryClone = DeserializeRichTree(buffer);
        if (!richTree->Compare(binaryClone.Get()) )
            UNIT_ASSERT_C(!"Binary serialization check failed!", msg);
    }
};

UNIT_TEST_SUITE_REGISTRATION(TRichrequestSerializationTest);
