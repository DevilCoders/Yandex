#include <kernel/qtree/richrequest/tokenlist.h>

#include <library/cpp/testing/unittest/registar.h>

class TRichRequestTokenListTest: public TTestBase {
    UNIT_TEST_SUITE(TRichRequestTokenListTest);
        UNIT_TEST(TestSimple);
    UNIT_TEST_SUITE_END();

    void TestSimple() const {
        UNIT_ASSERT(TokenString("a") == "a");
        UNIT_ASSERT(TokenString("a b") == "a + b");
        UNIT_ASSERT(TokenString("a-b") == "a + b");
        UNIT_ASSERT(TokenString("a/b%c") == "a + b + c");
        UNIT_ASSERT(TokenString("a,b") == "a + b");
        UNIT_ASSERT(TokenString("ab") == "ab");
        UNIT_ASSERT(TokenString("г санкт-петербург") == "г + санкт + петербург");

        // marks (alnum words)
        UNIT_ASSERT(TokenString("a2b3") == "a + 2 + b + 3");
        UNIT_ASSERT(TokenString("a b mp3") == "a + b + mp + 3");
        UNIT_ASSERT(TokenString("иркутск пр-т жукова д10 кв2") == "иркутск + пр + т + жукова + д + 10 + кв + 2");

        // quotes, parenth
        UNIT_ASSERT(TokenString("мама мыла раму")     == "мама + мыла + раму");
        UNIT_ASSERT(TokenString("\"мама мыла\" раму") == "мама + мыла + раму");
        UNIT_ASSERT(TokenString("мама \"мыла\" раму") == "мама + мыла + раму");
        UNIT_ASSERT(TokenString("мама \"мыла раму\"") == "мама + мыла + раму");
        UNIT_ASSERT(TokenString("\"мама мыла раму\"") == "мама + мыла + раму");
        UNIT_ASSERT(TokenString("раз (два три) четыре") == "раз + два + три + четыре");
        UNIT_ASSERT(TokenString("раз (два (три (четыре)))") == "раз + два + три + четыре");

        // misc search request syntax
        UNIT_ASSERT(TokenString("(телефон ~~ iphone) | (телефон не &&/(1 1) iphone) | (телефон \"не iphone\")") == "телефон + телефон + не + iphone + телефон + не + iphone");
        UNIT_ASSERT(TokenString("мазда ^ toyota lancer") == "мазда + toyota + lancer");
        UNIT_ASSERT(TokenString("Имя Фамилия <- finame:((имя фамилия))") == "Имя + Фамилия + имя + фамилия");

    }

private:
    TRichNodePtr MakeTree(const char* text) const {
        return CreateRichTree(UTF8ToWide(text), TCreateTreeOptions(~TLangMask()))->Root;
    }

    TString TokenString(const char* text) const {
        TRichNodePtr tree = MakeTree(text);
        TTokenList tlist(*tree);
        //Cerr << tlist.DebugString() << Endl;
        TStringStream ret;
        for (size_t i = 0; i < tlist.Size(); ++i) {
            if (i > 0)
                ret << " + ";
            ret << tlist[i].GetText();
        }
        //Cerr << ret.Str() << Endl;
        return ret.Str();
    }

};

UNIT_TEST_SUITE_REGISTRATION(TRichRequestTokenListTest);
