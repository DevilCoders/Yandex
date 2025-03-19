
#include <kernel/gazetteer/gazetteer.h>
#include <kernel/gazetteer/richtree/gztres.h>
#include <kernel/gazetteer/proto/base.pb.h>
#include <kernel/qtree/richrequest/richnode.h>

#include <library/cpp/testing/unittest/registar.h>

#include <google/protobuf/descriptor_database.h>


using namespace NGzt;


class TGazetteerRichTreeTest : public TTestBase {
    UNIT_TEST_SUITE(TGazetteerRichTreeTest);
        UNIT_TEST(TestNormalization);
    UNIT_TEST_SUITE_END();
public:
    void TestNormalization();
};

static TAutoPtr<TGazetteer> MakeGazetteer(const TString& source) {
    TGazetteerBuilder builder;
    builder.UseAllBuiltinDescriptors();
    return builder.BuildFromText(source) ? builder.MakeGazetteer() : nullptr;
}

static size_t ArticlesFound(const TGazetteer& gazetteer, TRichTreePtr qtree, bool normalize = false) {
    TGztResults results(&gazetteer);
    results.Reset(qtree->Root, normalize);
    return results.Size();
}

static TRichTreePtr CreateTreeUtf8(const TStringBuf query, ELanguage lang) {
    return CreateRichTree(TUtf16String::FromUtf8(query), TCreateTreeOptions(TLanguageContext(lang)));
}

static const TString TEST_NORMALIZATION_GZT = R"EOF(
    encoding "utf8";

    import "kernel/gazetteer/proto/base.proto";

    TArticle "ё" { key = { "!королёв"  lang = RUS } } // CYRILLIC SMALL LETTER IO U+0451
    TArticle "e" { key = { "!наметкин" lang = RUS } } // CYRILLIC SMALL LETTER IE U+0435
    TArticle "i" { key = { "!мiр" lang = UKR } } // LATIN SMALL LETTER I U+0069
    TArticle "і" { key = { "!сір" lang = UKR } } // CYRILLIC SMALL LETTER BELORUSSIAN-UKRAINIAN I U+0456
    TArticle "й" { key = { "!мой" lang = RUS } }
    TArticle "и" { key = { "!заика" lang = RUS } }
    TArticle "I" { key = { "!halide" lang = TUR } }
    TArticle "ı" { key = { "!adıvar" lang = TUR } }
)EOF";

void TGazetteerRichTreeTest::TestNormalization() {
    TAutoPtr<TGazetteer> gzt = MakeGazetteer(TEST_NORMALIZATION_GZT);
    UNIT_ASSERT(gzt);

    UNIT_ASSERT_EQUAL(ArticlesFound(*gzt, CreateTreeUtf8("королёв намёткин", LANG_RUS), false), 1);
    // "королёв" matches twice, as a main form and an extra normal form
    UNIT_ASSERT_EQUAL(ArticlesFound(*gzt, CreateTreeUtf8("королёв намёткин", LANG_RUS), true), 3);
    // request is already in normal form, both words match once
    UNIT_ASSERT_EQUAL(ArticlesFound(*gzt, CreateTreeUtf8("королев наметкин", LANG_RUS), false), 2);
    UNIT_ASSERT_EQUAL(ArticlesFound(*gzt, CreateTreeUtf8("королев наметкин", LANG_RUS), true), 2);
    // renyx elimination
    UNIT_ASSERT_EQUAL(ArticlesFound(*gzt, CreateTreeUtf8("мiр сiр", LANG_UKR), false), 1); // latin
    UNIT_ASSERT_EQUAL(ArticlesFound(*gzt, CreateTreeUtf8("мір сір", LANG_UKR), false), 1); // cyrillic
    UNIT_ASSERT_EQUAL(ArticlesFound(*gzt, CreateTreeUtf8("мiр сiр", LANG_UKR), true), 2); // latin
    UNIT_ASSERT_EQUAL(ArticlesFound(*gzt, CreateTreeUtf8("мір сір", LANG_UKR), true), 1); // cyrillic
    // и/й is not affected
    UNIT_ASSERT_EQUAL(ArticlesFound(*gzt, CreateTreeUtf8("мои заика", LANG_RUS), false), 1);
    UNIT_ASSERT_EQUAL(ArticlesFound(*gzt, CreateTreeUtf8("мои заика", LANG_RUS), true), 1);
    UNIT_ASSERT_EQUAL(ArticlesFound(*gzt, CreateTreeUtf8("мой зайка", LANG_RUS), true), 1);
    // neither is ı/i
    UNIT_ASSERT_EQUAL(ArticlesFound(*gzt, CreateTreeUtf8("halide edib adıvar", LANG_TUR), true), 2);
    UNIT_ASSERT_EQUAL(ArticlesFound(*gzt, CreateTreeUtf8("halıde edib adivar", LANG_TUR), true), 0);
}

UNIT_TEST_SUITE_REGISTRATION(TGazetteerRichTreeTest);
