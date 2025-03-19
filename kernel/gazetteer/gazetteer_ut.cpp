#include <kernel/gazetteer/gazetteer.h>
#include <kernel/gazetteer/tokenize.h>
#include <kernel/gazetteer/simpletext/simpletext.h>
#include <kernel/gazetteer/gzttrie.h>
#include <kernel/gazetteer/proto/syntax.pb.h>
#include <kernel/gazetteer/proto/base.pb.h>
#include <kernel/gazetteer/generator.h>

#include <kernel/gazetteer/ut/test_compile.pb.h>
#include <kernel/gazetteer/ut/test_import1.pb.h>
#include <kernel/gazetteer/ut/test_import2.pb.h>
#include <kernel/gazetteer/proto/base.pb.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/archive/yarchive.h>

#include <google/protobuf/descriptor_database.h>

#include <util/system/tempfile.h>
#include <util/stream/file.h>
#include <util/folder/dirut.h>
#include <util/folder/path.h>


using namespace NGzt;


class TGazetteerTest : public TTestBase {
    UNIT_TEST_SUITE(TGazetteerTest);
        UNIT_TEST(TestCompile);
        UNIT_TEST(TestImport);
        UNIT_TEST(TestBuiltins);
        UNIT_TEST(TestBuildFromProtobuf);
        UNIT_TEST(TestGenerator1);
        UNIT_TEST(TestGenerator2);

        UNIT_TEST(TestLemma);
        UNIT_TEST(TestExactForm);
        UNIT_TEST(TestAllForms);
        UNIT_TEST(TestTitleCase1);
        UNIT_TEST(TestTitleCase2);
        UNIT_TEST(TestGztTrie);
        UNIT_TEST(TestVerb);
        UNIT_TEST(TestNumberFilters);
        UNIT_TEST(TestNumberFilters2);
        UNIT_TEST(TestGenderFilters);
        UNIT_TEST(TestCaseFilters1);
        UNIT_TEST(TestCaseFilters2);
        UNIT_TEST(TestCapitalizationFilters);
        UNIT_TEST(TestORFilters);
        UNIT_TEST(TestANDFilters);
        UNIT_TEST(TestLangTat);
        UNIT_TEST(TestLangRum);
        UNIT_TEST(TestLangFilters);
        UNIT_TEST(TestGleicheFilters1);
        UNIT_TEST(TestGleicheFilters2);
        UNIT_TEST(TestAllLemmaForms);
        UNIT_TEST(TestDiacritic1);
        UNIT_TEST(TestDiacritic2);
        UNIT_TEST(TestDiacritic3);
        UNIT_TEST(TestTokenize);
        UNIT_TEST(TestRetokenizer);
        UNIT_TEST(TestMultiTokenNone);
        UNIT_TEST(TestMultiTokenWizard);
        UNIT_TEST(TestMultiTokenIndex);
        UNIT_TEST(TestSimpleTextPunct);
        UNIT_TEST(TestApostrophe);
        UNIT_TEST(TestArticlePoolIter);
    UNIT_TEST_SUITE_END();
public:
    void TestCompile();
    void TestImport();
    void TestBuiltins();
    void TestBuildFromProtobuf();
    void TestGenerator1();
    void TestGenerator2();

    void TestExactForm();
    void TestAllForms();
    void TestGztTrie();
    void TestLemma();
    void TestTitleCase1();
    void TestTitleCase2();
    void TestVerb();
    void TestNumberFilters();
    void TestNumberFilters2();
    void TestGenderFilters();
    void TestCaseFilters1();
    void TestCaseFilters2();
    void TestCapitalizationFilters();
    void TestORFilters();
    void TestANDFilters();
    void TestLangTat();
    void TestLangRum();
    void TestLangFilters();
    void TestGleicheFilters1();
    void TestGleicheFilters2();
    void ERRORTestFiltersTwoGrams();
    void TestAllLemmaForms();
    void TestDiacritic1();
    void TestDiacritic2();
    void TestDiacritic3();
    void TestTokenize();
    void TestRetokenizer();
    void TestMultiTokenNone();
    void TestMultiTokenWizard();
    void TestMultiTokenIndex();
    void TestSimpleTextPunct();
    void TestApostrophe();
    void TestArticlePoolIter();
};

static const unsigned char TEST_DATA[] = {
    #include <kernel/gazetteer/ut/test_gazetteer.inc>
};




class TTempGzt {
public:
    TTempGzt(const TString& gztFileName)
        : TempFolder("./temp_test_gzt_d8w7sdk3")
        , Existed(TempFolder.Exists())
        , GztFileName(gztFileName)
    {
        if (!Existed)
            TempFolder.MkDir();

        ProtoFile1.Reset(CreateTempFile("test_compile.gztproto"));
        ProtoFile2.Reset(CreateTempFile("test_import1.gztproto"));
        ProtoFile3.Reset(CreateTempFile("test_import2.gztproto"));
        ProtoFile4.Reset(CreateTempFile("test_geo.gztproto"));
        GztFile.Reset(CreateTempFile(GztFileName));

        const TString gztBin = GztFileName + ".bin";
        GztBinFile.Reset(new TTempFile(TempFolder/gztBin));
    }

    ~TTempGzt() {
        GztBinFile.Reset(nullptr);
        GztFile.Reset(nullptr);
        ProtoFile1.Reset(nullptr);
        ProtoFile2.Reset(nullptr);
        ProtoFile3.Reset(nullptr);
        ProtoFile4.Reset(nullptr);

        if (!Existed)
            TempFolder.DeleteIfExists();
    }

    bool Compile() {
        return Builder.Compile(GztFile->Name(), GztBinFile->Name());
    }

    void UseAllBuiltinDescriptors() {
        Builder.UseAllBuiltinDescriptors();
    }

    bool BinaryExists() const {
        return NFs::Exists(GztBinFile->Name());
    }

    bool IsGoodBinary() {
        return Builder.IsGoodBinary(GztFile->Name(), GztBinFile->Name());
    }

    TAutoPtr<TGazetteer> Load() const {
        return new TGazetteer(TBlob::FromFile(GztBinFile->Name()));
    }

    TAutoPtr<TGazetteer> Make() {
        if (!Builder.BuildFromFile(GztFile->Name()))
            ythrow yexception() << "Cannot make gazetteer " << GztFileName << " from archive";
        return Builder.MakeGazetteer();
    }

private:
    static TAutoPtr<TTempFile> CreateTempFile(const TString& fileName, const TString& keyInArchive) {
        TArchiveReader reader(TBlob::NoCopy(TEST_DATA, sizeof(TEST_DATA)));
        TString body = reader.ObjectByKey(keyInArchive)->ReadAll();
        TOFStream fileStream(fileName);
        fileStream << body;
        fileStream.Finish();
//        Cerr << "Created tmp file " << fileName << " from [" << keyInArchive << "]" << Endl;
        return new TTempFile(fileName);
    }

    TAutoPtr<TTempFile> CreateTempFile(const TString& fileName) {
        return CreateTempFile(TempFolder/fileName, "/" + fileName);
    }

private:
    TFsPath TempFolder;
    bool Existed;

    TString GztFileName;

    THolder<TTempFile> ProtoFile1, ProtoFile2, ProtoFile3, ProtoFile4;
    THolder<TTempFile> GztFile;
    THolder<TTempFile> GztBinFile;



    TGazetteerBuilder Builder;
};


static TAutoPtr<TGazetteer> CreateGazetteerFromArchive(const TString& gztFileName) {
    TTempGzt tmpGzt(gztFileName);
    return tmpGzt.Make();
}

static inline void TestNotCompileImpl(const TString& gztFileName) {
    TTempGzt tmpGzt(gztFileName);
    UNIT_ASSERT(!tmpGzt.Compile());
    UNIT_ASSERT(!tmpGzt.BinaryExists());
}

void TGazetteerTest::TestCompile() {
    {
        TTempGzt tmpGzt("test_compile.gzt");

        UNIT_ASSERT(tmpGzt.Compile());
        UNIT_ASSERT(tmpGzt.BinaryExists());
        UNIT_ASSERT(tmpGzt.IsGoodBinary());

        UNIT_ASSERT_NO_EXCEPTION(tmpGzt.Load());
    }
    {
        // not compiled
        TestNotCompileImpl("test_not_compile1.gzt");
        TestNotCompileImpl("test_not_compile2.gzt");
        TestNotCompileImpl("test_not_compile3.gzt");
    }
}

void TGazetteerTest::TestBuiltins() {
    {
        TTempGzt tmpGzt("test_builtins.gzt");
        UNIT_ASSERT(!tmpGzt.Compile());     // should not compile without builtins
        UNIT_ASSERT(!tmpGzt.BinaryExists());
    }
    {
        TTempGzt tmpGzt("test_builtins.gzt");
        tmpGzt.UseAllBuiltinDescriptors();
        UNIT_ASSERT(tmpGzt.Compile());
        UNIT_ASSERT(tmpGzt.BinaryExists());
        UNIT_ASSERT(tmpGzt.IsGoodBinary());
        UNIT_ASSERT_NO_EXCEPTION(tmpGzt.Load());
    }
}


void TGazetteerTest::TestImport() {
    using namespace NProtoBuf;

    const DescriptorPool* genPool = DescriptorPool::generated_pool();

    // only .proto in generated pool
    UNIT_ASSERT(genPool->FindFileByName("kernel/gazetteer/ut/test_import1.gztproto") == nullptr);
    UNIT_ASSERT(genPool->FindFileByName("kernel/gazetteer/ut/test_import2.gztproto") == nullptr);
    UNIT_ASSERT(genPool->FindFileByName("kernel/gazetteer/ut/test_import1.proto") != nullptr);
    UNIT_ASSERT(genPool->FindFileByName("kernel/gazetteer/ut/test_import2.proto") != nullptr);

    const Descriptor* genType1 = genPool->FindMessageTypeByName("NGztUt.TTestImportArticle1");
    const Descriptor* genType2 = genPool->FindMessageTypeByName("NGztUt.TTestImportArticle2");
    UNIT_ASSERT(genType1 != nullptr && genType1 == NGztUt::TTestImportArticle1::descriptor());
    UNIT_ASSERT(genType2 != nullptr && genType2 == NGztUt::TTestImportArticle2::descriptor());

    TAutoPtr<TGazetteer> gztPtr = CreateGazetteerFromArchive("test_import.gzt");
    TGazetteer* gzt = gztPtr.Get();
    UNIT_ASSERT(gzt != nullptr);
    const TProtoPool& dynPool = gzt->ProtoPool();

    const Descriptor* dynType1 = dynPool.FindMessageTypeByName("NGztUt.TTestImportArticle1");
    const Descriptor* dynType2 = dynPool.FindMessageTypeByName("NGztUt.TTestImportArticle2");
    UNIT_ASSERT(dynType1 != nullptr);
    UNIT_ASSERT(dynType2 != nullptr);

    // loaded descriptors should be same as generated ones
    UNIT_ASSERT(dynType1 == genType1);
    UNIT_ASSERT(dynType2 == genType2);

    UNIT_ASSERT(dynPool.IsSubType(dynType2, dynType1));
    UNIT_ASSERT(!dynPool.IsSubType(dynType1, dynType2));

    TArticlePtr aaa = gzt->LoadArticle(u"AAA");
    UNIT_ASSERT(!aaa.IsNull());
    UNIT_ASSERT(aaa.GetType() == genType1);
    UNIT_ASSERT(aaa.As<NGztUt::TTestImportArticle1>() != nullptr);

    TArticlePtr bbb = gzt->LoadArticle(u"BBB");
    UNIT_ASSERT(!bbb.IsNull());
    UNIT_ASSERT(bbb.GetType() == genType2);
    UNIT_ASSERT(bbb.As<NGztUt::TTestImportArticle2>() != nullptr);
}

static size_t NumArticlesFound(const TGazetteer* gazetteer, const TString& key, const TString& title, TLangMask lang = LANG_RUS) {
    TArticleIter<TSimpleText> iter;
    TSimpleText text(lang);
    text.Reset(UTF8ToWide(key));
    size_t res = 0;
    TUtf16String wTitle = UTF8ToWide(title);
    for (gazetteer->IterArticles(text, &iter); iter.Ok(); ++iter) {
        TArticlePtr article = gazetteer->GetArticle(iter);
        if (article.GetTitle() == wTitle) {
            res++;
        }
    }
    return res;
}

static bool ShouldFind(const TGazetteer* gazetteer, const TString& key, const TString& title, TLangMask lang = LANG_RUS) {
    return (NumArticlesFound(gazetteer, key, title, lang) > 0);
}

bool ShouldFindAll(const TGazetteer*, const TString&, TLangMask) {
    return true;
}

template <typename... TTitles>
bool ShouldFindAll(const TGazetteer* gazetteer, const TString& key, TLangMask lang, const TString& title, const TTitles&... titles) {
    return (ShouldFind(gazetteer, key, title, lang) && ShouldFindAll(gazetteer, key, lang, titles...));
}

static bool ShouldNotFind(const TGazetteer* gazetteer, const TString& key, const TString& title, TLangMask lang = LANG_RUS ) {
    return (NumArticlesFound(gazetteer, key, title, lang) == 0);
}

/*
import "base.proto";
TArticle "test1" { key = "тест" }
TArticle "test2" { key = { "тесты" morph=EXACT_FORM } }
*/

void TGazetteerTest::TestBuildFromProtobuf() {

    // manually fill gazetteer source file
    TGztFileDescriptorProto file;
    file.set_name("testFromProto.gzt");
    file.add_dependency("base.proto");

    {
        TArticleDescriptorProto& art = *file.add_article();

        art.set_type("TArticle");
        art.set_name("test1");
        TArticleFieldDescriptorProto& field = *art.add_field();
        field.set_name("key");
        field.mutable_value()->set_type(TFieldValueDescriptorProto::TYPE_STRING);
        field.mutable_value()->set_string_or_identifier("тест");      // utf8
    }

    {
        TArticleDescriptorProto& art = *file.add_article();

        art.set_type("TArticle");
        art.set_name("test2");
        TArticleFieldDescriptorProto& field = *art.add_field();
        field.set_name("key");
        field.mutable_value()->set_type(TFieldValueDescriptorProto::TYPE_BLOCK);

        TArticleFieldDescriptorProto& field1 = *field.mutable_value()->add_sub_field();
        field1.mutable_value()->set_type(TFieldValueDescriptorProto::TYPE_STRING);
        field1.mutable_value()->set_string_or_identifier("тесты");      // utf8

        TArticleFieldDescriptorProto& field2 = *field.mutable_value()->add_sub_field();
        field2.set_name("morph");
        field2.mutable_value()->set_type(TFieldValueDescriptorProto::TYPE_IDENTIFIER);
        field2.mutable_value()->set_string_or_identifier("EXACT_FORM");
    }

    TGazetteerBuilder builder;
    UNIT_ASSERT(builder.BuildFromProtobuf(file));

    TAutoPtr<TGazetteer> gzt = builder.MakeGazetteer();
    UNIT_ASSERT(gzt.Get() != nullptr);


    UNIT_ASSERT(ShouldFind(gzt.Get(),  "тест", "test1"));
    UNIT_ASSERT(!ShouldFind(gzt.Get(), "тест", "test2"));
    UNIT_ASSERT(ShouldFind(gzt.Get(), "тесты", "test1"));
    UNIT_ASSERT(ShouldFind(gzt.Get(), "тесты", "test2"));
}


/*
import "base.proto";
TArticle "test1" { key = "тест" }
TArticle "test2" { key = { "тесты" morph=EXACT_FORM } }
UTestCompile "test3" { key = { "москва" gram="тв" } stringparam = "2", intparam = [3, 33], doubleparam = 4.1, boolparam = false, refparam = {["latin","кириллица"]}, enumparam = [EVALUE1, EVALUE2] }
}
*/

static void FillGenerator(TGztGenerator& gen) {
    {
        TArticle art;
        gen.AddKey(art, "тест");    // utf8
        gen.AddArticle(art, "test1");
    }
    {
        TArticle art;
        gen.AddExactFormKey(art, "тесты");
        gen.AddArticle(art, "test2");
    }
    {
        UTestCompile art;
        TSearchKey* key = gen.AddKey(art, "москва");
        key->add_gram()->add_allow()->add_g("твор");
        art.set_stringparam("2");
        art.add_intparam(3);
        art.add_intparam(33);
        art.set_doubleparam(4.1);
        art.set_boolparam(false);
        UTestCompile::USubTestCompile* sub = art.add_refparam();
            sub->add_repstringparam("latin");
            sub->add_repstringparam("кириллица");
        art.add_enumparam(UTestCompile::EVALUE1);
        art.add_enumparam(UTestCompile::EVALUE2);

        gen.AddArticle(art, "test3");
    }
}



void TGazetteerTest::TestGenerator1() {
    // gazetteer with necessary proto-definitions
    THolder<TGazetteer> descrGzt(CreateGazetteerFromArchive("test_compile.gzt"));

    // generate gazetteer source using descriptors from parent gazetteer
    TGztGenerator gen(descrGzt.Get());
    FillGenerator(gen);

    TAutoPtr<TGazetteer> gzt = gen.MakeGazetteer();
    UNIT_ASSERT(gzt.Get() != nullptr);


    UNIT_ASSERT(ShouldFind(gzt.Get(),  "тест", "test1"));
    UNIT_ASSERT(!ShouldFind(gzt.Get(), "тест", "test2"));
    UNIT_ASSERT(ShouldFind(gzt.Get(), "тесты", "test1"));
    UNIT_ASSERT(ShouldFind(gzt.Get(), "тесты", "test2"));
    UNIT_ASSERT(ShouldFind(gzt.Get(), "москвой", "test3"));
    UNIT_ASSERT(!ShouldFind(gzt.Get(), "москва", "test3"));
}

void TGazetteerTest::TestGenerator2() {
    // generate gazetteer source using builtin descriptor
    TGztGenerator gen;
    gen.UseBuiltinDescriptor<UTestCompile>();
    FillGenerator(gen);

    TAutoPtr<TGazetteer> gzt = gen.MakeGazetteer();
    UNIT_ASSERT(gzt.Get() != nullptr);


    UNIT_ASSERT(ShouldFind(gzt.Get(),  "тест", "test1"));
    UNIT_ASSERT(!ShouldFind(gzt.Get(), "тест", "test2"));
    UNIT_ASSERT(ShouldFind(gzt.Get(), "тесты", "test1"));
    UNIT_ASSERT(ShouldFind(gzt.Get(), "тесты", "test2"));
    UNIT_ASSERT(ShouldFind(gzt.Get(), "москвой", "test3"));
    UNIT_ASSERT(!ShouldFind(gzt.Get(), "москва", "test3"));
}



/*  test_morphology_file.gzt
UTestMorphology "testLemmaForm1" { "корова" }
UTestMorphology "testLemmaForm2" { "коровы" }
UTestMorphology "testLemmaForm3" { "коровой" }
UTestMorphology "testLemmaForm4" { "коровка" }
UTestMorphology "testLemmaForm5" { "корове" }

UTestMorphology "testMorphExactForm1" { "корова" morph=EXACT_FORM }
UTestMorphology "testMorphExactForm2" { "коровы" morph=EXACT_FORM }
UTestMorphology "testMorphExactForm3" { "коровой" morph=EXACT_FORM }
UTestMorphology "testMorphExactForm4" { "коровка" morph=EXACT_FORM }
UTestMorphology "testMorphExactForm5" { "корове" morph=EXACT_FORM }

UTestMorphology "testExactForm1" { "!корова" }
UTestMorphology "testExactForm2" { "!коровы" }
UTestMorphology "testExactForm3" { "!коровой" }
UTestMorphology "testExactForm4" { "!коровка" }
UTestMorphology "testExactForm5" { "!корове" }

UTestMorphology "testAllLemmaForm1" { {"корова" morph=ALL_LEMMA_FORMS} }
UTestMorphology "testAllLemmaForm2" { {"коровы" morph=ALL_LEMMA_FORMS} }
UTestMorphology "testAllLemmaForm3" { {"коровой" morph=ALL_LEMMA_FORMS} }
UTestMorphology "testAllLemmaForm4" { {"коровка" morph=ALL_LEMMA_FORMS} }
UTestMorphology "testAllLemmaForm5" { {"корове" morph=ALL_LEMMA_FORMS} }

UTestMorphology "testAllLemmaFormUkr1" { {"корова" morph=ALL_LEMMA_FORMS lang=UKR} }
UTestMorphology "testAllLemmaFormUkr2" { {"корового" morph=ALL_LEMMA_FORMS lang=UKR} }
UTestMorphology "testAllLemmaFormUkr3" { {"коровому" morph=ALL_LEMMA_FORMS lang=UKR} }
UTestMorphology "testAllLemmaFormUkr5" { {"корове" morph=ALL_LEMMA_FORMS lang=UKR} }

*/


void TGazetteerTest::TestExactForm() {
    TAutoPtr<TGazetteer> gztPtr = CreateGazetteerFromArchive("test_morphology.gzt");
    TGazetteer* gzt = gztPtr.Get();

    UNIT_ASSERT(ShouldFind(gzt, "корова", "testLemmaForm1"));
    UNIT_ASSERT(ShouldFind(gzt, "корова", "testMorphExactForm1"));
    UNIT_ASSERT(ShouldFind(gzt, "корова", "testExactForm1"));
    UNIT_ASSERT(ShouldFind(gzt, "корова", "testAllLemmaForm1"));
    UNIT_ASSERT(ShouldFind(gzt, "корова", "testAllLemmaFormUkr1", LANG_UKR));

    UNIT_ASSERT(ShouldFind(gzt, "коровы", "testLemmaForm2"));
    UNIT_ASSERT(ShouldFind(gzt, "коровы", "testMorphExactForm2"));
    UNIT_ASSERT(ShouldFind(gzt, "коровы", "testExactForm2"));
    UNIT_ASSERT(ShouldFind(gzt, "коровы", "testAllLemmaForm2"));

    UNIT_ASSERT(ShouldFind(gzt, "коровой", "testLemmaForm3"));
    UNIT_ASSERT(ShouldFind(gzt, "коровой", "testMorphExactForm3"));
    UNIT_ASSERT(ShouldFind(gzt, "коровой", "testExactForm3"));
    UNIT_ASSERT(ShouldFind(gzt, "коровой", "testAllLemmaForm3"));

    UNIT_ASSERT(ShouldFind(gzt, "коровка", "testLemmaForm4"));
    UNIT_ASSERT(ShouldFind(gzt, "коровка", "testMorphExactForm4"));
    UNIT_ASSERT(ShouldFind(gzt, "коровка", "testExactForm4"));
    UNIT_ASSERT(ShouldFind(gzt, "коровка", "testAllLemmaForm4"));

    UNIT_ASSERT(ShouldFind(gzt, "корове", "testLemmaForm5"));
    UNIT_ASSERT(ShouldFind(gzt, "корове", "testMorphExactForm5"));
    UNIT_ASSERT(ShouldFind(gzt, "корове", "testExactForm5"));
    UNIT_ASSERT(ShouldFind(gzt, "корове", "testAllLemmaForm5"));

}

void TGazetteerTest::TestLemma() {
    TAutoPtr<TGazetteer> gztPtr = CreateGazetteerFromArchive("test_morphology.gzt");
    TGazetteer* gzt = gztPtr.Get();

    UNIT_ASSERT(ShouldFind(gzt, "коровы", "testLemmaForm1"));
    UNIT_ASSERT(ShouldFind(gzt, "коровой", "testLemmaForm1"));
    UNIT_ASSERT(ShouldFind(gzt, "корове", "testLemmaForm1"));

    UNIT_ASSERT(ShouldFind(gzt, "коровы", "testAllLemmaForm1"));
    UNIT_ASSERT(ShouldFind(gzt, "коровой", "testAllLemmaForm1"));
    UNIT_ASSERT(ShouldFind(gzt, "корове", "testAllLemmaForm1"));

    UNIT_ASSERT(ShouldFind(gzt, "коровому", "testAllLemmaFormUkr1", LANG_UKR));
    UNIT_ASSERT(ShouldFind(gzt, "корового", "testAllLemmaFormUkr1", LANG_UKR));
    UNIT_ASSERT(ShouldFind(gzt, "корове", "testAllLemmaFormUkr1", LANG_UKR));
}

/*
UTestMorphology "testAllForms1" { key = {"перевозить" morph = ALL_FORMS } lemma = "перевозить" }
UTestMorphology "testAllForms2" { key = {"пианино" morph = ALL_FORMS } lemma = "перевозить" }
UTestMorphology "testAllForms3" { key = {"сантехник" morph = ALL_FORMS } lemma = "перевозить" }
UTestMorphology "testAllForms4" { key = {"вызов" morph = ALL_FORMS } lemma = "перевозить" }
*/
void TGazetteerTest::TestAllForms() {
    TAutoPtr<TGazetteer> gztPtr = CreateGazetteerFromArchive("test_morphology.gzt");
    TGazetteer* gzt = gztPtr.Get();

    UNIT_ASSERT(ShouldFind(gzt, "перевезти пианино", "testAllForms1"));
    UNIT_ASSERT(ShouldFind(gzt, "перевезти пианино", "testAllForms2"));

    UNIT_ASSERT(ShouldFind(gzt, "вызов сантехника", "testAllForms3"));
    UNIT_ASSERT(ShouldFind(gzt, "вызов сантехника", "testAllForms4"));
}



/*
UTestMorphology "testTitleLetters1" { {"СНЕГИРЬ" morph=EXACT_FORM} }
UTestMorphology "testTitleLetters2" { {"Снегирь" morph=ALL_LEMMA_FORMS} }
UTestMorphology "testTitleLetters3" {"Снегирь"}
UTestMorphology "testTitleLetters4" {"снеГирь"}
*/

void TGazetteerTest::TestTitleCase1() {
    TAutoPtr<TGazetteer> gztPtr = CreateGazetteerFromArchive("test_morphology.gzt");
    TGazetteer* gzt = gztPtr.Get();

    UNIT_ASSERT(ShouldFind(gzt, "Снегирь", "testTitleLetters1"));
    UNIT_ASSERT(ShouldFind(gzt, "снегирь", "testTitleLetters1"));

    UNIT_ASSERT(ShouldFind(gzt, "СНЕГИРИ", "testTitleLetters2"));

    UNIT_ASSERT(ShouldFind(gzt, "СНЕГИРЬ", "testTitleLetters3"));
    UNIT_ASSERT(ShouldFind(gzt, "снегИрь", "testTitleLetters4"));

    UNIT_ASSERT(ShouldFind(gzt, "сНегири", "testTitleLetters3"));
    UNIT_ASSERT(ShouldFind(gzt, "СнегирИ", "testTitleLetters4"));
}


/*
UTestMorphology "testTitleLetters5" { {"Метал" morph=ALL_LEMMA_FORMS} }
UTestMorphology "testTitleLetters6" { {"Металл" morph=ALL_LEMMA_FORMS} }
UTestMorphology "testTitleLetters7" { {"метал" morph=ALL_LEMMA_FORMS} }
UTestMorphology "testTitleLetters8" { {"металл" morph=ALL_LEMMA_FORMS} }
*/

void TGazetteerTest::TestTitleCase2() {
    TAutoPtr<TGazetteer> gztPtr = CreateGazetteerFromArchive("test_morphology.gzt");
    TGazetteer* gzt = gztPtr.Get();

    UNIT_ASSERT(ShouldFind(gzt, "Метал", "testTitleLetters5"));
    UNIT_ASSERT(ShouldFind(gzt, "метал", "testTitleLetters5"));
    UNIT_ASSERT(ShouldNotFind(gzt, "Металл", "testTitleLetters5"));
    UNIT_ASSERT(ShouldNotFind(gzt, "металл", "testTitleLetters5"));

    UNIT_ASSERT(ShouldFind(gzt, "Метал", "testTitleLetters7"));
    UNIT_ASSERT(ShouldFind(gzt, "метал", "testTitleLetters7"));
    UNIT_ASSERT(ShouldNotFind(gzt, "Металл", "testTitleLetters7"));
    UNIT_ASSERT(ShouldNotFind(gzt, "металл", "testTitleLetters7"));

    UNIT_ASSERT(ShouldNotFind(gzt, "Метал", "testTitleLetters6"));
    UNIT_ASSERT(ShouldNotFind(gzt, "метал", "testTitleLetters6"));
    UNIT_ASSERT(ShouldFind(gzt, "Металл", "testTitleLetters6"));
    UNIT_ASSERT(ShouldFind(gzt, "металл", "testTitleLetters6"));

    UNIT_ASSERT(ShouldNotFind(gzt, "Метал", "testTitleLetters8"));
    UNIT_ASSERT(ShouldNotFind(gzt, "метал", "testTitleLetters8"));
    UNIT_ASSERT(ShouldFind(gzt, "Металл", "testTitleLetters8"));
    UNIT_ASSERT(ShouldFind(gzt, "металл", "testTitleLetters8"));
}

/*
UTestFilter "testVerbV1" { key = { "раздел" gram = { "V" } } }
UTestFilter "testVerbV2" { key = { "раздевать" gram = { "V" } } }
UTestFilter "testVerbS" { key = { "раздел" gram = { "S" } } }
*/
void TGazetteerTest::TestVerb() {
    TAutoPtr<TGazetteer> gztPtr = CreateGazetteerFromArchive("test_filters.gzt");
    TGazetteer* gzt = gztPtr.Get();

    UNIT_ASSERT(ShouldNotFind(gzt, "раздел", "testVerbV1"));
    UNIT_ASSERT(ShouldNotFind(gzt, "раздела", "testVerbV1"));
    UNIT_ASSERT(ShouldNotFind(gzt, "раздевать", "testVerbV1"));
    UNIT_ASSERT(ShouldNotFind(gzt, "раздевал", "testVerbV1"));
    UNIT_ASSERT(ShouldNotFind(gzt, "раздевает", "testVerbV1"));

    UNIT_ASSERT(ShouldFind(gzt, "раздел", "testVerbV2"));
    UNIT_ASSERT(ShouldFind(gzt, "раздела", "testVerbV2"));
    UNIT_ASSERT(ShouldFind(gzt, "раздевать", "testVerbV2"));
    UNIT_ASSERT(ShouldFind(gzt, "раздевал", "testVerbV2"));
    UNIT_ASSERT(ShouldFind(gzt, "раздевает", "testVerbV2"));

    UNIT_ASSERT(ShouldFind(gzt, "раздел", "testVerbS"));
    UNIT_ASSERT(ShouldFind(gzt, "разделы", "testVerbS"));
    UNIT_ASSERT(ShouldFind(gzt, "раздела", "testVerbS"));
    UNIT_ASSERT(ShouldNotFind(gzt, "раздевать", "testVerbS"));
    UNIT_ASSERT(ShouldNotFind(gzt, "раздевал", "testVerbS"));
    UNIT_ASSERT(ShouldNotFind(gzt, "раздевает", "testVerbS"));
}


/*
UTestFilter "testFilterNumber1" { key = { "садовый кольцо" gram = { "мн" word = 1 } }  }
UTestFilter "testFilterNumber2" { key = { "садовый кольцо" gram = { "мн" } } }
UTestFilter "testFilterNumber3" { key = { "садовый кольцо" gram = { "ед" word = 1 } } }
UTestFilter "testFilterNumber4" { key = { "садовый кольцо" gram = { "ед" } } }
*/
void TGazetteerTest::TestNumberFilters2() {
    TAutoPtr<TGazetteer> gztPtr = CreateGazetteerFromArchive("test_filters.gzt");
    TGazetteer* gzt = gztPtr.Get();

    UNIT_ASSERT(ShouldFind(gzt, "садовые кольца", "testFilterNumber1"));
    UNIT_ASSERT(ShouldFind(gzt, "садовые кольца", "testFilterNumber2"));
    UNIT_ASSERT(ShouldNotFind(gzt, "садовые кольца", "testFilterNumber3"));
    UNIT_ASSERT(ShouldNotFind(gzt, "садовые кольца", "testFilterNumber4"));

    UNIT_ASSERT(ShouldFind(gzt, "садовые кольцо", "testFilterNumber1"));
    UNIT_ASSERT(ShouldNotFind(gzt, "садовые кольцо", "testFilterNumber2"));
    UNIT_ASSERT(ShouldNotFind(gzt, "садовые кольцо", "testFilterNumber3"));
    UNIT_ASSERT(ShouldNotFind(gzt, "садовые кольцо", "testFilterNumber4"));

    UNIT_ASSERT(ShouldNotFind(gzt, "садовое кольцо", "testFilterNumber1"));
    UNIT_ASSERT(ShouldNotFind(gzt, "садовое кольцо", "testFilterNumber2"));
    UNIT_ASSERT(ShouldFind(gzt, "садовое кольцо", "testFilterNumber3"));
    UNIT_ASSERT(ShouldFind(gzt, "садовое кольцо", "testFilterNumber4"));

    UNIT_ASSERT(ShouldNotFind(gzt, "садовое кольца", "testFilterNumber1"));
    UNIT_ASSERT(ShouldNotFind(gzt, "садовое кольца", "testFilterNumber2"));
    UNIT_ASSERT(ShouldFind(gzt, "садовое кольца", "testFilterNumber3"));
    UNIT_ASSERT(ShouldFind(gzt, "садовое кольца", "testFilterNumber4"));

    UNIT_ASSERT(ShouldNotFind(gzt, "садовому кольцу", "testFilterNumber1"));
    UNIT_ASSERT(ShouldNotFind(gzt, "садовому кольцу", "testFilterNumber2"));
    UNIT_ASSERT(ShouldFind(gzt, "садовому кольцу", "testFilterNumber3"));
    UNIT_ASSERT(ShouldFind(gzt, "садовому кольцу", "testFilterNumber4"));
}

/*
UTestFilter "testFilterNumber5" { key = { "ростов на дон" gram = { "ед" } } } // all words are in a single form
UTestFilter "testFilterNumber6" { key = { "ростов на дон" gram = { "мн" } } }
UTestFilter "testFilterNumber7" { key = { "ростов на дон" gram = { "ед" word = 1 } } }
UTestFilter "testFilterNumber8" { key = { "ростов на дон" gram = { "ед" word = 2 } } }
UTestFilter "testFilterNumber9" { key = { "ростов на дон" gram = { "ед" word = 3 } } }
UTestFilter "testFilterNumber10" { key = { "ростов на дон" } }
*/

void TGazetteerTest::TestNumberFilters() {
    TAutoPtr<TGazetteer> gztPtr = CreateGazetteerFromArchive("test_filters.gzt");
    TGazetteer* gzt = gztPtr.Get();

    UNIT_ASSERT(ShouldNotFind(gzt, "ростов на дону", "testFilterNumber5")); // "на" is not in a snigle form
    UNIT_ASSERT(ShouldNotFind(gzt, "ростов на дону", "testFilterNumber6"));
    UNIT_ASSERT(ShouldFind(gzt, "ростов на дону", "testFilterNumber7"));
    UNIT_ASSERT(ShouldNotFind(gzt, "ростов на дону", "testFilterNumber8"));
    UNIT_ASSERT(ShouldFind(gzt, "ростов на дону", "testFilterNumber9"));
    UNIT_ASSERT(ShouldFind(gzt, "ростов на дону", "testFilterNumber10"));

}

/*
UTestFilter "testFilterGender1" { key = { "кофе" }  }
UTestFilter "testFilterGender2" { key = { "кофе" gram = { "муж" } } }
UTestFilter "testFilterGender3" { key = { "кофе" gram = { "жен" } } }
*/
void TGazetteerTest::TestGenderFilters() {
    TAutoPtr<TGazetteer> gztPtr = CreateGazetteerFromArchive("test_filters.gzt");
    TGazetteer* gzt = gztPtr.Get();

    UNIT_ASSERT_EQUAL(NumArticlesFound(gzt, "кофе", "testFilterGender1"), 1);
    UNIT_ASSERT_EQUAL(NumArticlesFound(gzt, "кофе", "testFilterGender2"), 1);
    UNIT_ASSERT_EQUAL(NumArticlesFound(gzt, "кофе", "testFilterGender3"), 0);
}

/*
UTestFilter "testFilterCaseNom" { key = { "дом" gram = { "nom" } } }
UTestFilter "testFilterCaseGen" { key = { "дом" gram = { "gen" } } }
UTestFilter "testFilterCaseDat" { key = { "дом" gram = { "dat" } } }
UTestFilter "testFilterCaseAcc" { key = { "дом" gram = { "acc" } } }
UTestFilter "testFilterCaseIns" { key = { "дом" gram = { "ins" } } }
UTestFilter "testFilterCaseAbl" { key = { "дом" gram = { "abl" } } }
*/
void TGazetteerTest::TestCaseFilters1() {
    TAutoPtr<TGazetteer> gztPtr = CreateGazetteerFromArchive("test_filters.gzt");
    TGazetteer* gzt = gztPtr.Get();

    UNIT_ASSERT(ShouldFind(gzt, "дом", "testFilterCase1Nom"));
    UNIT_ASSERT(ShouldFind(gzt, "дом", "testFilterCase1Acc"));
    UNIT_ASSERT(ShouldNotFind(gzt, "дом", "testFilterCase1Gen"));
    UNIT_ASSERT(ShouldNotFind(gzt, "дом", "testFilterCase1Dat"));
    UNIT_ASSERT(ShouldNotFind(gzt, "дом", "testFilterCase1Ins"));
    UNIT_ASSERT(ShouldNotFind(gzt, "дом", "testFilterCase1Abl"));

    UNIT_ASSERT(ShouldFind(gzt, "дома", "testFilterCase1Gen"));
    UNIT_ASSERT(ShouldFind(gzt, "дома", "testFilterCase1NomPl"));
    UNIT_ASSERT(ShouldNotFind(gzt, "дома", "testFilterCase1Dat"));
    UNIT_ASSERT(ShouldNotFind(gzt, "дома", "testFilterCase1Ins"));
    UNIT_ASSERT(ShouldNotFind(gzt, "дома", "testFilterCase1Abl"));

    UNIT_ASSERT(ShouldFind(gzt, "дому", "testFilterCase1Dat"));
    UNIT_ASSERT(ShouldNotFind(gzt, "дому", "testFilterCase1Nom"));
    UNIT_ASSERT(ShouldNotFind(gzt, "дому", "testFilterCase1Gen"));
    UNIT_ASSERT(ShouldNotFind(gzt, "дому", "testFilterCase1Acc"));
    UNIT_ASSERT(ShouldNotFind(gzt, "дому", "testFilterCase1Ins"));
    UNIT_ASSERT(ShouldNotFind(gzt, "дому", "testFilterCase1Abl"));

    UNIT_ASSERT(ShouldFind(gzt, "домом", "testFilterCase1Ins"));
    UNIT_ASSERT(ShouldNotFind(gzt, "домом", "testFilterCase1Nom"));
    UNIT_ASSERT(ShouldNotFind(gzt, "домом", "testFilterCase1Gen"));
    UNIT_ASSERT(ShouldNotFind(gzt, "домом", "testFilterCase1Dat"));
    UNIT_ASSERT(ShouldNotFind(gzt, "домом", "testFilterCase1Acc"));
    UNIT_ASSERT(ShouldNotFind(gzt, "домом", "testFilterCase1Abl"));

    UNIT_ASSERT(ShouldFind(gzt, "доме", "testFilterCase1Abl"));
    UNIT_ASSERT(ShouldNotFind(gzt, "доме", "testFilterCase1Nom"));
    UNIT_ASSERT(ShouldNotFind(gzt, "доме", "testFilterCase1Gen"));
    UNIT_ASSERT(ShouldNotFind(gzt, "доме", "testFilterCase1Dat"));
    UNIT_ASSERT(ShouldNotFind(gzt, "доме", "testFilterCase1Acc"));
    UNIT_ASSERT(ShouldNotFind(gzt, "дому", "testFilterCase1Ins"));

    //pl
    UNIT_ASSERT(ShouldFind(gzt, "дома", "testFilterCase1Nom"));
    UNIT_ASSERT(ShouldFind(gzt, "домов", "testFilterCase1Gen"));
    UNIT_ASSERT(ShouldFind(gzt, "домам", "testFilterCase1Dat"));
    UNIT_ASSERT(ShouldFind(gzt, "дома", "testFilterCase1Acc"));
    UNIT_ASSERT(ShouldFind(gzt, "домами", "testFilterCase1Ins"));
    UNIT_ASSERT(ShouldFind(gzt, "домах", "testFilterCase1Abl"));
}
/*
UTestFilter "testFilterCase2Nom" { key = { "веревка" gram = { "nom" } } }
UTestFilter "testFilterCase2Gen" { key = { "веревка" gram = { "gen" } } }
UTestFilter "testFilterCase2Dat" { key = { "веревка" gram = { "dat" } } }
UTestFilter "testFilterCase2Acc" { key = { "веревка" gram = { "acc" } } }
UTestFilter "testFilterCase2Ins" { key = { "веревка" gram = { "ins" } } }
UTestFilter "testFilterCase2Abl" { key = { "веревка" gram = { "abl" } } }
*/

void TGazetteerTest::TestCaseFilters2() {
    TAutoPtr<TGazetteer> gztPtr = CreateGazetteerFromArchive("test_filters.gzt");
    TGazetteer* gzt = gztPtr.Get();

    // sg
    UNIT_ASSERT(ShouldFind(gzt, "веревка", "testFilterCase2Nom"));
    UNIT_ASSERT(ShouldFind(gzt, "веревки", "testFilterCase2Gen"));
    UNIT_ASSERT(ShouldFind(gzt, "веревке", "testFilterCase2Dat"));
    UNIT_ASSERT(ShouldFind(gzt, "веревку", "testFilterCase2Acc"));
    UNIT_ASSERT(ShouldFind(gzt, "веревкой", "testFilterCase2Ins"));
    UNIT_ASSERT(ShouldFind(gzt, "веревкою", "testFilterCase2Ins"));
    UNIT_ASSERT(ShouldFind(gzt, "веревке", "testFilterCase2Abl"));

    //pl
    UNIT_ASSERT(ShouldFind(gzt, "веревки", "testFilterCase2Nom"));
    UNIT_ASSERT(ShouldFind(gzt, "веревок", "testFilterCase2Gen"));
    UNIT_ASSERT(ShouldFind(gzt, "веревкам", "testFilterCase2Dat"));
    UNIT_ASSERT(ShouldFind(gzt, "веревки", "testFilterCase2Acc"));
    UNIT_ASSERT(ShouldFind(gzt, "веревками", "testFilterCase2Ins"));
    UNIT_ASSERT(ShouldFind(gzt, "веревках", "testFilterCase2Abl"));


    UNIT_ASSERT(ShouldNotFind(gzt, "веревка", "testFilterCase2Gen"));
    UNIT_ASSERT(ShouldNotFind(gzt, "веревка", "testFilterCase2Dat"));
    UNIT_ASSERT(ShouldNotFind(gzt, "веревка", "testFilterCase2Acc"));
    UNIT_ASSERT(ShouldNotFind(gzt, "веревка", "testFilterCase2Ins"));
    UNIT_ASSERT(ShouldNotFind(gzt, "веревка", "testFilterCase2Abl"));

    UNIT_ASSERT(ShouldNotFind(gzt, "веревки", "testFilterCase2Dat"));
    UNIT_ASSERT(ShouldNotFind(gzt, "веревки", "testFilterCase2Ins"));
    UNIT_ASSERT(ShouldNotFind(gzt, "веревки", "testFilterCase2Abl"));

    UNIT_ASSERT(ShouldNotFind(gzt, "веревке", "testFilterCase2Nom"));
    UNIT_ASSERT(ShouldNotFind(gzt, "веревке", "testFilterCase2Gen"));
    UNIT_ASSERT(ShouldNotFind(gzt, "веревке", "testFilterCase2Acc"));
    UNIT_ASSERT(ShouldNotFind(gzt, "веревке", "testFilterCase2Ins"));

    UNIT_ASSERT(ShouldNotFind(gzt, "веревку", "testFilterCase2Nom"));
    UNIT_ASSERT(ShouldNotFind(gzt, "веревку", "testFilterCase2Gen"));
    UNIT_ASSERT(ShouldNotFind(gzt, "веревку", "testFilterCase2Dat"));
    UNIT_ASSERT(ShouldNotFind(gzt, "веревку", "testFilterCase2Ins"));
    UNIT_ASSERT(ShouldNotFind(gzt, "веревку", "testFilterCase2Abl"));

    UNIT_ASSERT(ShouldNotFind(gzt, "веревках", "testFilterCase2Nom"));
    UNIT_ASSERT(ShouldNotFind(gzt, "веревках", "testFilterCase2Gen"));
    UNIT_ASSERT(ShouldNotFind(gzt, "веревках", "testFilterCase2Dat"));
    UNIT_ASSERT(ShouldNotFind(gzt, "веревках", "testFilterCase2Acc"));
    UNIT_ASSERT(ShouldNotFind(gzt, "веревках", "testFilterCase2Ins"));

    UNIT_ASSERT(ShouldNotFind(gzt, "веревкам", "testFilterCase2Nom"));
    UNIT_ASSERT(ShouldNotFind(gzt, "веревкам", "testFilterCase2Gen"));
    UNIT_ASSERT(ShouldNotFind(gzt, "веревкам", "testFilterCase2Acc"));
    UNIT_ASSERT(ShouldNotFind(gzt, "веревкам", "testFilterCase2Ins"));
    UNIT_ASSERT(ShouldNotFind(gzt, "веревкам", "testFilterCase2Abl"));

}

/*
UTestFilter "testCapitalizationFilter1" { key = { "2я" case = { forbid = LOWER word = 1 } } }
UTestFilter "testCapitalizationFilter2" { key = { "2я" case = { allow = LOWER word = 1 } } }
UTestFilter "testCapitalizationFilter3" { key = { "2Я" case = { allow = UPPER word = 1 } } }
UTestFilter "testCapitalizationFilter4" { key = { "2Я" case = { forbid = UPPER word = 1 } } }
UTestFilter "testCapitalizationFilter5" { key = { "2я" case = { allow = TITLE word = 1 } } }
UTestFilter "testCapitalizationFilter7a" { key = { "слово" case = { allow = LOWER word = 1 } } }
UTestFilter "testCapitalizationFilter7b" { key = { "слово" case = { forbid = LOWER word = 1 } } }
UTestFilter "testCapitalizationFilter8a" { key = { "слово" case = { allow = UPPER word = 1 } } }
UTestFilter "testCapitalizationFilter8b" { key = { "слово" case = { forbid = UPPER word = 1 } } }
UTestFilter "testCapitalizationFilter9a" { key = { "слово" case = { allow = TITLE word = 1 } } }
UTestFilter "testCapitalizationFilter9b" { key = { "слово" case = { forbid = TITLE word = 1 } } }
*/

void TGazetteerTest::TestCapitalizationFilters() {
    TAutoPtr<TGazetteer> gztPtr = CreateGazetteerFromArchive("test_filters.gzt");
    TGazetteer* gzt = gztPtr.Get();
    // LOWER
    UNIT_ASSERT(ShouldFind(gzt, "2я", "testCapitalizationFilter1"));
    UNIT_ASSERT(ShouldFind(gzt, "2Я", "testCapitalizationFilter1"));
    UNIT_ASSERT(ShouldFind(gzt, "2я", "testCapitalizationFilter2"));
    UNIT_ASSERT(ShouldNotFind(gzt, "2Я", "testCapitalizationFilter2"));
    UNIT_ASSERT(ShouldFind(gzt, "слово", "testCapitalizationFilter7a"));
    UNIT_ASSERT(ShouldNotFind(gzt, "Слово", "testCapitalizationFilter7a"));
    UNIT_ASSERT(ShouldNotFind(gzt, "слово", "testCapitalizationFilter7b"));
    UNIT_ASSERT(ShouldFind(gzt, "Слово", "testCapitalizationFilter7b"));

    // UPPER
    UNIT_ASSERT(ShouldNotFind(gzt, "2я", "testCapitalizationFilter3"));
    UNIT_ASSERT(ShouldFind(gzt, "2Я", "testCapitalizationFilter3"));
    UNIT_ASSERT(ShouldFind(gzt, "2я", "testCapitalizationFilter4"));
    UNIT_ASSERT(ShouldFind(gzt, "2Я", "testCapitalizationFilter4"));
    UNIT_ASSERT(ShouldFind(gzt, "СЛОВО", "testCapitalizationFilter8a"));
    UNIT_ASSERT(ShouldNotFind(gzt, "Слово", "testCapitalizationFilter8a"));
    UNIT_ASSERT(ShouldNotFind(gzt, "СЛОВО", "testCapitalizationFilter8b"));
    UNIT_ASSERT(ShouldFind(gzt, "Слово", "testCapitalizationFilter8b"));

    // TITLE
    UNIT_ASSERT(ShouldFind(gzt, "2я", "testCapitalizationFilter5"));
    UNIT_ASSERT(ShouldNotFind(gzt, "2Я", "testCapitalizationFilter5"));
    UNIT_ASSERT(ShouldFind(gzt, "2я", "testCapitalizationFilter6"));
    UNIT_ASSERT(ShouldFind(gzt, "2Я", "testCapitalizationFilter6"));
    UNIT_ASSERT(ShouldFind(gzt, "Слово", "testCapitalizationFilter9a"));
    UNIT_ASSERT(ShouldNotFind(gzt, "слово", "testCapitalizationFilter9a"));
    UNIT_ASSERT(ShouldNotFind(gzt, "Слово", "testCapitalizationFilter9b"));
    UNIT_ASSERT(ShouldFind(gzt, "СЛОВО", "testCapitalizationFilter9b"));
    UNIT_ASSERT(ShouldFind(gzt, "слово", "testCapitalizationFilter9b"));
}

/*
UTestMorphology "testAllLemmaForm1" { {"корова" morph=ALL_LEMMA_FORMS} }
UTestMorphology "testAllLemmaForm2" { {"коровы" morph=ALL_LEMMA_FORMS} }
UTestMorphology "testAllLemmaForm3" { {"коровой" morph=ALL_LEMMA_FORMS} }
UTestMorphology "testAllLemmaForm4" { {"коровка" morph=ALL_LEMMA_FORMS} }
UTestMorphology "testAllLemmaForm5" { {"корове" morph=ALL_LEMMA_FORMS} }
*/
void TGazetteerTest::TestAllLemmaForms() {
    TAutoPtr<TGazetteer> gztPtr = CreateGazetteerFromArchive("test_morphology.gzt");
    TGazetteer* gzt = gztPtr.Get();

    // nom
    UNIT_ASSERT(ShouldFind(gzt, "корова", "testAllLemmaForm1"));
    UNIT_ASSERT(ShouldFind(gzt, "корова", "testAllLemmaForm2"));
    UNIT_ASSERT(ShouldFind(gzt, "корова", "testAllLemmaForm3"));
    UNIT_ASSERT(ShouldFind(gzt, "корова", "testAllLemmaForm5"));

    //gen
    UNIT_ASSERT(ShouldFind(gzt, "коровы", "testAllLemmaForm1"));
    UNIT_ASSERT(ShouldFind(gzt, "коровы", "testAllLemmaForm2"));
    UNIT_ASSERT(ShouldFind(gzt, "коровы", "testAllLemmaForm3"));
    UNIT_ASSERT(ShouldFind(gzt, "коровы", "testAllLemmaForm5"));

    //ins
    UNIT_ASSERT(ShouldFind(gzt, "коровой", "testAllLemmaForm1"));
    UNIT_ASSERT(ShouldFind(gzt, "коровой", "testAllLemmaForm2"));
    UNIT_ASSERT(ShouldFind(gzt, "коровой", "testAllLemmaForm3"));
    UNIT_ASSERT(ShouldFind(gzt, "коровой", "testAllLemmaForm5"));

    //dat, abl
    UNIT_ASSERT(ShouldFind(gzt, "корове", "testAllLemmaForm1"));
    UNIT_ASSERT(ShouldFind(gzt, "корове", "testAllLemmaForm2"));
    UNIT_ASSERT(ShouldFind(gzt, "корове", "testAllLemmaForm3"));
    UNIT_ASSERT(ShouldFind(gzt, "корове", "testAllLemmaForm5"));

    //acc
    UNIT_ASSERT(ShouldFind(gzt, "корову", "testAllLemmaForm1"));
    UNIT_ASSERT(ShouldFind(gzt, "корову", "testAllLemmaForm2"));
    UNIT_ASSERT(ShouldFind(gzt, "корову", "testAllLemmaForm3"));
    UNIT_ASSERT(ShouldFind(gzt, "корову", "testAllLemmaForm5"));

    // another form
    UNIT_ASSERT(ShouldFind(gzt, "коровка", "testAllLemmaForm4"));
    UNIT_ASSERT(ShouldFind(gzt, "коровки", "testAllLemmaForm4"));
    UNIT_ASSERT(ShouldFind(gzt, "коровке", "testAllLemmaForm4"));

    UNIT_ASSERT(ShouldFind(gzt, "коровий", "testAllLemmaFormUkr1", LANG_UKR)); //nom, S
    UNIT_ASSERT(ShouldFind(gzt, "корови", "testAllLemmaFormUkr1", LANG_UKR));  //gen, S
    UNIT_ASSERT(ShouldFind(gzt, "корове", "testAllLemmaFormUkr1", LANG_UKR));
    UNIT_ASSERT(ShouldFind(gzt, "корові", "testAllLemmaFormUkr1", LANG_UKR));  //dat, S
    UNIT_ASSERT(ShouldFind(gzt, "корову", "testAllLemmaFormUkr1", LANG_UKR));  //acc, S
    UNIT_ASSERT(ShouldFind(gzt, "коровій", "testAllLemmaFormUkr1", LANG_UKR));
    UNIT_ASSERT(ShouldFind(gzt, "корово", "testAllLemmaFormUkr1", LANG_UKR));  //voc, S
    UNIT_ASSERT(ShouldFind(gzt, "коровою", "testAllLemmaFormUkr1", LANG_UKR)); //ins, S

    UNIT_ASSERT(ShouldFind(gzt, "коровий", "testAllLemmaFormUkr2", LANG_UKR));
    UNIT_ASSERT(ShouldFind(gzt, "корове", "testAllLemmaFormUkr2", LANG_UKR));
    UNIT_ASSERT(ShouldFind(gzt, "корові", "testAllLemmaFormUkr2", LANG_UKR));
    UNIT_ASSERT(ShouldFind(gzt, "корову", "testAllLemmaFormUkr2", LANG_UKR));
    UNIT_ASSERT(ShouldFind(gzt, "коровій", "testAllLemmaFormUkr2", LANG_UKR));
    UNIT_ASSERT(ShouldFind(gzt, "коровою", "testAllLemmaFormUkr2", LANG_UKR));

    //special form
    UNIT_ASSERT(ShouldFind(gzt, "ребенок", "testAllLemmaForm10"));
}

/*
UTestFilter "testFilterOR1" { key = { "веревка" gram = { "nom|gen|dat" } } }
UTestFilter "testFilterOR2" { key = { "веревка" gram = { "acc|ins|abl" } } }

UTestFilter "testFilterOR3" { key = { "дом" gram = { "nom|gen|dat|acc" word = 1} } }
UTestFilter "testFilterOR4" { key = { "дом" gram = { "ins|abl" word = 1} } }
*/
void TGazetteerTest::TestORFilters() {
    TAutoPtr<TGazetteer> gztPtr = CreateGazetteerFromArchive("test_filters.gzt");
    TGazetteer* gzt = gztPtr.Get();

    UNIT_ASSERT(ShouldFind(gzt, "веревка", "testFilterOR1")); //nom
    UNIT_ASSERT(ShouldFind(gzt, "веревки", "testFilterOR1")); //gen
    UNIT_ASSERT(ShouldFind(gzt, "веревке", "testFilterOR1")); //dat
    UNIT_ASSERT(ShouldFind(gzt, "веревку", "testFilterOR2")); //acc
    UNIT_ASSERT(ShouldFind(gzt, "веревкой", "testFilterOR2")); //ins
    UNIT_ASSERT(ShouldFind(gzt, "веревкою", "testFilterOR2")); //ins
    UNIT_ASSERT(ShouldFind(gzt, "веревке", "testFilterOR2"));  //abl

    UNIT_ASSERT(ShouldNotFind(gzt, "веревка", "testFilterOR2"));
    UNIT_ASSERT(ShouldNotFind(gzt, "веревку", "testFilterOR1"));
    UNIT_ASSERT(ShouldNotFind(gzt, "веревкой", "testFilterOR1"));
    UNIT_ASSERT(ShouldNotFind(gzt, "веревкою", "testFilterOR1"));

    UNIT_ASSERT(ShouldFind(gzt, "дом", "testFilterOR3"));  //nom
    UNIT_ASSERT(ShouldFind(gzt, "дома", "testFilterOR3")); //gen
    UNIT_ASSERT(ShouldFind(gzt, "дому", "testFilterOR3")); //dat
    UNIT_ASSERT(ShouldFind(gzt, "дом", "testFilterOR3"));  //acc
    UNIT_ASSERT(ShouldFind(gzt, "домом", "testFilterOR4"));//ins
    UNIT_ASSERT(ShouldFind(gzt, "доме", "testFilterOR4")); //abl

}

/*
UTestFilter "testANDFilterNomSg" { key = { "бокал" gram = { "nom,sg" word = 1} } }
UTestFilter "testANDFilterGenSg" { key = { "бокал" gram = { "gen,sg" } } }
UTestFilter "testANDFilterDatSg" { key = { "бокал" gram = { "dat,sg" word = 1} } }
UTestFilter "testANDFilterAccSg" { key = { "бокал" gram = { "acc,sg" } } }
UTestFilter "testANDFilterInsSg" { key = { "бокал" gram = { "ins,sg" word = 1} } }
UTestFilter "testANDFilterAblSg" { key = { "бокал" gram = { "abl,sg" } } }

UTestFilter "testANDFilterNomPl" { key = { "бокал" gram = { "nom,pl" } } }
UTestFilter "testANDFilterGenPl" { key = { "бокал" gram = { "gen,pl" word = 1 } } }
UTestFilter "testANDFilterDatPl" { key = { "бокал" gram = { "dat,pl" } } }
UTestFilter "testANDFilterAccPl" { key = { "бокал" gram = { "acc,pl" word = 1 } } }
UTestFilter "testANDFilterInsPl" { key = { "бокал" gram = { "ins,pl" } } }
UTestFilter "testANDFilterAblPl" { key = { "бокал" gram = { "abl,pl" word = 1 } } }
*/

void TGazetteerTest::TestANDFilters() {
    TAutoPtr<TGazetteer> gztPtr = CreateGazetteerFromArchive("test_filters.gzt");
    TGazetteer* gzt = gztPtr.Get();

    // test should find
    UNIT_ASSERT(ShouldFind(gzt, "бокал", "testANDFilterNomSg")); //nom,sg | acc,sg
    UNIT_ASSERT(ShouldFind(gzt, "бокал", "testANDFilterAccSg")); //nom,sg | acc,sg
    UNIT_ASSERT(ShouldFind(gzt, "бокала", "testANDFilterGenSg")); //gen,sg
    UNIT_ASSERT(ShouldFind(gzt, "бокалу", "testANDFilterDatSg")); //dat,sg
    UNIT_ASSERT(ShouldFind(gzt, "бокалом", "testANDFilterInsSg")); //ins,sg
    UNIT_ASSERT(ShouldFind(gzt, "бокале", "testANDFilterAblSg")); //abl,sg
    UNIT_ASSERT(ShouldFind(gzt, "бокалы", "testANDFilterNomPl"));  //nom,pl | acc,pl
    UNIT_ASSERT(ShouldFind(gzt, "бокалы", "testANDFilterAccPl"));  //nom,pl | acc,pl
    UNIT_ASSERT(ShouldFind(gzt, "бокалов", "testANDFilterGenPl"));  //gen,pl
    UNIT_ASSERT(ShouldFind(gzt, "бокалам", "testANDFilterDatPl"));  //dat,pl
    UNIT_ASSERT(ShouldFind(gzt, "бокалами", "testANDFilterInsPl"));  //ins,pl
    UNIT_ASSERT(ShouldFind(gzt, "бокалах", "testANDFilterAblPl"));  //abl,pl

    //test nom,sg
    UNIT_ASSERT(ShouldFind(gzt, "бокал", "testANDFilterNomSg")); //nom,sg | acc,sg
    UNIT_ASSERT(ShouldNotFind(gzt, "бокала", "testANDFilterNomSg")); //gen,sg
    UNIT_ASSERT(ShouldNotFind(gzt, "бокалу", "testANDFilterNomSg")); //dat,sg
    UNIT_ASSERT(ShouldNotFind(gzt, "бокалом", "testANDFilterNomSg")); //ins,sg
    UNIT_ASSERT(ShouldNotFind(gzt, "бокале", "testANDFilterNomSg")); //abl,sg
    UNIT_ASSERT(ShouldNotFind(gzt, "бокалы", "testANDFilterNomSg"));  //nom,pl | acc,pl
    UNIT_ASSERT(ShouldNotFind(gzt, "бокалов", "testANDFilterNomSg"));  //gen,pl
    UNIT_ASSERT(ShouldNotFind(gzt, "бокалам", "testANDFilterNomSg"));  //dat,pl
    UNIT_ASSERT(ShouldNotFind(gzt, "бокалами", "testANDFilterNomSg"));  //ins,pl
    UNIT_ASSERT(ShouldNotFind(gzt, "бокалах", "testANDFilterNomSg"));  //abl,pl

    //gen, sg
    UNIT_ASSERT(ShouldNotFind(gzt, "бокал", "testANDFilterGenSg")); //nom,sg | acc,sg
    UNIT_ASSERT(ShouldFind(gzt, "бокала", "testANDFilterGenSg")); //gen,sg
    UNIT_ASSERT(ShouldNotFind(gzt, "бокалу", "testANDFilterGenSg")); //dat,sg
    UNIT_ASSERT(ShouldNotFind(gzt, "бокалом", "testANDFilterGenSg")); //ins,sg
    UNIT_ASSERT(ShouldNotFind(gzt, "бокале", "testANDFilterGenSg")); //abl,sg
    UNIT_ASSERT(ShouldNotFind(gzt, "бокалы", "testANDFilterGenSg"));  //nom,pl | acc,pl
    UNIT_ASSERT(ShouldNotFind(gzt, "бокалов", "testANDFilterGenSg"));  //gen,pl
    UNIT_ASSERT(ShouldNotFind(gzt, "бокалам", "testANDFilterGenSg"));  //dat,pl
    UNIT_ASSERT(ShouldNotFind(gzt, "бокалами", "testANDFilterGenSg"));  //ins,pl
    UNIT_ASSERT(ShouldNotFind(gzt, "бокалах", "testANDFilterGenSg"));  //abl,pl

    //nom,pl
    UNIT_ASSERT(ShouldNotFind(gzt, "бокал", "testANDFilterNomPl")); //nom,sg | acc,sg
    UNIT_ASSERT(ShouldNotFind(gzt, "бокала", "testANDFilterNomPl")); //gen,sg
    UNIT_ASSERT(ShouldNotFind(gzt, "бокалу", "testANDFilterNomPl")); //dat,sg
    UNIT_ASSERT(ShouldNotFind(gzt, "бокалом", "testANDFilterNomPl")); //ins,sg
    UNIT_ASSERT(ShouldNotFind(gzt, "бокале", "testANDFilterNomPl")); //abl,sg
    UNIT_ASSERT(ShouldFind(gzt, "бокалы", "testANDFilterNomPl"));  //nom,pl | acc,pl
    UNIT_ASSERT(ShouldFind(gzt, "бокалы", "testANDFilterAccPl"));  //nom,pl | acc,pl
    UNIT_ASSERT(ShouldNotFind(gzt, "бокалов", "testANDFilterNomPl"));  //gen,pl
    UNIT_ASSERT(ShouldNotFind(gzt, "бокалам", "testANDFilterNomPl"));  //dat,pl
    UNIT_ASSERT(ShouldNotFind(gzt, "бокалами", "testANDFilterNomPl"));  //ins,pl
    UNIT_ASSERT(ShouldNotFind(gzt, "бокалах", "testANDFilterNomPl"));  //abl,pl

    //gen, pl
    UNIT_ASSERT(ShouldNotFind(gzt, "бокал", "testANDFilterGenPl")); //nom,sg | acc,sg
    UNIT_ASSERT(ShouldNotFind(gzt, "бокала", "testANDFilterGenPl")); //gen,sg
    UNIT_ASSERT(ShouldNotFind(gzt, "бокалу", "testANDFilterGenPl")); //dat,sg
    UNIT_ASSERT(ShouldNotFind(gzt, "бокалом", "testANDFilterGenPl")); //ins,sg
    UNIT_ASSERT(ShouldNotFind(gzt, "бокале", "testANDFilterGenPl")); //abl,sg
    UNIT_ASSERT(ShouldNotFind(gzt, "бокалы", "testANDFilterGenPl"));  //nom,pl | acc,pl
    UNIT_ASSERT(ShouldFind(gzt, "бокалов", "testANDFilterGenPl"));  //gen,pl
    UNIT_ASSERT(ShouldNotFind(gzt, "бокалам", "testANDFilterGenPl"));  //dat,pl
    UNIT_ASSERT(ShouldNotFind(gzt, "бокалами", "testANDFilterGenPl"));  //ins,pl
    UNIT_ASSERT(ShouldNotFind(gzt, "бокалах", "testANDFilterGenPl"));  //abl,pl

    //dat,pl
    UNIT_ASSERT(ShouldNotFind(gzt, "бокал", "testANDFilterDatPl")); //nom,sg | acc,sg
    UNIT_ASSERT(ShouldNotFind(gzt, "бокала", "testANDFilterDatPl")); //gen,sg
    UNIT_ASSERT(ShouldNotFind(gzt, "бокалу", "testANDFilterDatPl")); //dat,sg
    UNIT_ASSERT(ShouldNotFind(gzt, "бокалом", "testANDFilterDatPl")); //ins,sg
    UNIT_ASSERT(ShouldNotFind(gzt, "бокале", "testANDFilterDatPl")); //abl,sg
    UNIT_ASSERT(ShouldNotFind(gzt, "бокалы", "testANDFilterDatPl"));  //nom,pl | acc,pl
    UNIT_ASSERT(ShouldNotFind(gzt, "бокалов", "testANDFilterDatPl"));  //gen,pl
    UNIT_ASSERT(ShouldFind(gzt, "бокалам", "testANDFilterDatPl"));  //dat,pl
    UNIT_ASSERT(ShouldNotFind(gzt, "бокалами", "testANDFilterDatPl"));  //ins,pl
    UNIT_ASSERT(ShouldNotFind(gzt, "бокалах", "testANDFilterDatPl"));  //abl,pl

}

/*
https://st.yandex-team.ru/REQWIZARD-840
UTestFilter "testTatLang1" { key = { "нурлат" gram = { "ед" } } }
UTestFilter "testTatLang2" { key = { "нурлат" } }
UTestFilter "testTatLang3" { key = { "нурлат" gram = { "S" } } }
*/
void TGazetteerTest::TestLangTat() {
    TAutoPtr<TGazetteer> gztPtr = CreateGazetteerFromArchive("test_filters.gzt");
    TGazetteer* gzt = gztPtr.Get();

    UNIT_ASSERT(ShouldFind(gzt, "нурлат", "testTatLang1"));
    UNIT_ASSERT(ShouldFind(gzt, "азс нурлат", "testTatLang1"));
    UNIT_ASSERT(ShouldFind(gzt, "выавыаыва нурлат", "testTatLang1"));

    UNIT_ASSERT(ShouldFind(gzt, "нурлат", "testTatLang2"));
    UNIT_ASSERT(ShouldFind(gzt, "азс нурлат", "testTatLang2"));
    UNIT_ASSERT(ShouldFind(gzt, "выавыаыва нурлат", "testTatLang2"));

    UNIT_ASSERT(ShouldFind(gzt, "татритэкнефть нурлат", "testTatLang1"));
    UNIT_ASSERT(ShouldFind(gzt, "татритэкнефть нурлат", "testTatLang2"));

    UNIT_ASSERT(ShouldNotFind(gzt, "татритэкнефть нурлат", "testTatLang1", LANG_TAT)); // Error here! https://st.yandex-team.ru/REQWIZARD-840
    // UNIT_ASSERT(ShouldFind(gzt, "татритэкнефть нурлат", "testTatLang1", LANG_TAT)); - it is right !
    UNIT_ASSERT(ShouldFind(gzt, "татритэкнефть нурлат", "testTatLang3", LANG_TAT)); // works that way!!

    UNIT_ASSERT(ShouldFind(gzt, "татритэкнефть нурлат", "testTatLang2", LANG_TAT));
}

/*
https://st.yandex-team.ru/REQWIZARD-861
UTestFilter "testRumLang1" { key = { "pthc" } }
UTestFilter "testRumLang2" { key = { "pthc" lang=ENG} }
*/
void TGazetteerTest::TestLangRum() {
    TAutoPtr<TGazetteer> gztPtr = CreateGazetteerFromArchive("test_filters.gzt");
    TGazetteer* gzt = gztPtr.Get();

    UNIT_ASSERT(ShouldFind(gzt, "pthc cum", "testRumLang1", LANG_RUM));
    UNIT_ASSERT(ShouldFind(gzt, "pthc cum", "testRumLang1"));
    UNIT_ASSERT(ShouldFind(gzt, "pthc cum", "testRumLang1", LANG_ENG));

    UNIT_ASSERT(ShouldNotFind(gzt, "pthc cum", "testRumLang2", LANG_RUM));
    UNIT_ASSERT(ShouldNotFind(gzt, "pthc cum", "testRumLang2"));
    UNIT_ASSERT(ShouldFind(gzt, "pthc cum", "testRumLang2", LANG_ENG));


    UNIT_ASSERT(ShouldFind(gzt, "pthc", "testRumLang1", LANG_RUM));
    UNIT_ASSERT(ShouldFind(gzt, "pthc", "testRumLang1"));
    UNIT_ASSERT(ShouldFind(gzt, "pthc", "testRumLang1", LANG_ENG));

    UNIT_ASSERT(ShouldNotFind(gzt, "pthc", "testRumLang2", LANG_RUM));
    UNIT_ASSERT(ShouldNotFind(gzt, "pthc", "testRumLang2"));
    UNIT_ASSERT(ShouldFind(gzt, "pthc", "testRumLang2", LANG_ENG));
}

/*
UTestFilter "testLangFilterRus" { key = { "корова" lang = RUS } }
UTestFilter "testLangFilterUkr" { key = { "корова" lang = UKR } }
*/
void TGazetteerTest::TestLangFilters() {
    TAutoPtr<TGazetteer> gztPtr = CreateGazetteerFromArchive("test_filters.gzt");
    TGazetteer* gzt = gztPtr.Get();

    UNIT_ASSERT(ShouldFind(gzt, "корова", "testLangFilterRus")); //nom,sg,rus
    UNIT_ASSERT(ShouldFind(gzt, "коровы", "testLangFilterRus")); //gen,sg,rus
    UNIT_ASSERT(ShouldFind(gzt, "корове", "testLangFilterRus")); //dat,sg,rus | abl,sg
    UNIT_ASSERT(ShouldFind(gzt, "корову", "testLangFilterRus")); //acc,sg,rus
    UNIT_ASSERT(ShouldFind(gzt, "коровою", "testLangFilterRus"));//ins,sg,rus

    UNIT_ASSERT(ShouldFind(gzt, "корова", "testLangFilterUkr", LANG_UKR)); //nom,sg,ukr
    UNIT_ASSERT(ShouldFind(gzt, "корови", "testLangFilterUkr", LANG_UKR)); //gen,sg,ukr
    UNIT_ASSERT(ShouldFind(gzt, "корові", "testLangFilterUkr", LANG_UKR)); //dat,sg,ukr | abl,sg
    UNIT_ASSERT(ShouldFind(gzt, "корову", "testLangFilterUkr", LANG_UKR)); //acc,sg,ukr
    UNIT_ASSERT(ShouldFind(gzt, "коровою", "testLangFilterUkr", LANG_UKR));  //ins,sg,ukr
    UNIT_ASSERT(ShouldFind(gzt, "корово", "testLangFilterUkr", LANG_UKR));  //voc,sg,ukr

    UNIT_ASSERT(ShouldNotFind(gzt, "корови", "testLangFilterRus")); //gen,sg,ukr
    UNIT_ASSERT(ShouldNotFind(gzt, "корові", "testLangFilterRus")); //dat,sg,ukr | abl,sg
    UNIT_ASSERT(ShouldNotFind(gzt, "корово", "testLangFilterRus"));  //voc,sg,ukr

    UNIT_ASSERT(ShouldNotFind(gzt, "коровы", "testLangFilterUkr", LANG_UKR)); //gen,sg,rus
    UNIT_ASSERT(ShouldNotFind(gzt, "корове", "testLangFilterUkr", LANG_UKR)); //dat,sg,rus | abl,sg
}

/*
UTestFilter "testGleicheFilter1" { key = { "красный площадь" agr = GENDER_NUMBER_CASE } }
*/
void TGazetteerTest::TestGleicheFilters1() {
    TAutoPtr<TGazetteer> gztPtr = CreateGazetteerFromArchive("test_filters.gzt");
    TGazetteer* gzt = gztPtr.Get();

    UNIT_ASSERT(ShouldFind(gzt, "красная площадь", "testGleicheFilter1")); //nom,sg
    UNIT_ASSERT(ShouldFind(gzt, "красной площади", "testGleicheFilter1")); //dat,sg | gen,sg | abl,sg
    UNIT_ASSERT(ShouldFind(gzt, "красную площадь", "testGleicheFilter1")); //acc,sg
    UNIT_ASSERT(ShouldFind(gzt, "красной площадью", "testGleicheFilter1")); //ins,sg

    UNIT_ASSERT(ShouldFind(gzt, "красные площади", "testGleicheFilter1")); //nom,pl | acc,pl
    UNIT_ASSERT(ShouldFind(gzt, "красных площадей", "testGleicheFilter1")); //gen,pl
    UNIT_ASSERT(ShouldFind(gzt, "красным площадям", "testGleicheFilter1")); //dat,pl
    UNIT_ASSERT(ShouldFind(gzt, "красными площадями", "testGleicheFilter1")); //ins,pl
    UNIT_ASSERT(ShouldFind(gzt, "красных площадях", "testGleicheFilter1")); //abl,pl


    UNIT_ASSERT(ShouldNotFind(gzt, "красная площади", "testGleicheFilter1")); //nom,sg + gen,sg
    UNIT_ASSERT(ShouldNotFind(gzt, "красная площадям", "testGleicheFilter1")); //nom,sg + dat,pl

    UNIT_ASSERT(ShouldNotFind(gzt, "красной площадь", "testGleicheFilter1")); //gen,sg + nom,sg
    UNIT_ASSERT(ShouldNotFind(gzt, "красной площадях", "testGleicheFilter1")); //gen,sg + abl,pl

    UNIT_ASSERT(ShouldNotFind(gzt, "красные площадью", "testGleicheFilter1")); //nom,pl + ins,sg
    UNIT_ASSERT(ShouldNotFind(gzt, "красные площадей", "testGleicheFilter1")); //nom,pl + gen,pl

    UNIT_ASSERT(ShouldNotFind(gzt, "красными площадь", "testGleicheFilter1")); //ins,pl + nom,sg
    UNIT_ASSERT(ShouldNotFind(gzt, "красными площади", "testGleicheFilter1")); //ins,pl + nom,pl

}

/*
UTestFilter "testGleicheFilter2" { key = { "рогатый корова" agr = GENDER_NUMBER_CASE } }
*/
void TGazetteerTest::TestGleicheFilters2() {
    TAutoPtr<TGazetteer> gztPtr = CreateGazetteerFromArchive("test_filters.gzt");
    TGazetteer* gzt = gztPtr.Get();

    UNIT_ASSERT(ShouldFind(gzt, "рогатая корова", "testGleicheFilter2")); //nom,sg
    UNIT_ASSERT(ShouldFind(gzt, "рогатой коровы", "testGleicheFilter2")); //gen,sg
    UNIT_ASSERT(ShouldFind(gzt, "рогатой корове", "testGleicheFilter2")); //dat,sg | ins,sg
    UNIT_ASSERT(ShouldFind(gzt, "рогатую корову", "testGleicheFilter2")); //acc,sg
    UNIT_ASSERT(ShouldFind(gzt, "рогатой коровой", "testGleicheFilter2")); //abl,sg

    UNIT_ASSERT(ShouldFind(gzt, "рогатые коровы", "testGleicheFilter2")); //nom,pl
    UNIT_ASSERT(ShouldFind(gzt, "рогатых коров", "testGleicheFilter2")); //gen,pl | acc,pl
    UNIT_ASSERT(ShouldFind(gzt, "рогатым коровам", "testGleicheFilter2")); //dat,pl
    UNIT_ASSERT(ShouldFind(gzt, "рогатыми коровами", "testGleicheFilter2")); //ins,pl
    UNIT_ASSERT(ShouldFind(gzt, "рогатых коровах", "testGleicheFilter2")); //abl,pl


    UNIT_ASSERT(ShouldNotFind(gzt, "рогатая коровы", "testGleicheFilter2")); //nom,sg + gen,sg
    UNIT_ASSERT(ShouldNotFind(gzt, "рогатая коров", "testGleicheFilter2")); //nom,sg + dat,pl

    UNIT_ASSERT(ShouldNotFind(gzt, "рогатую коровой", "testGleicheFilter2")); //acc,sg + abl,sg
    UNIT_ASSERT(ShouldNotFind(gzt, "рогатую коровами", "testGleicheFilter2")); //acc,sg + ins,pl

    UNIT_ASSERT(ShouldNotFind(gzt, "рогатых корову", "testGleicheFilter2")); //gen,pl + acc,sg
    UNIT_ASSERT(ShouldNotFind(gzt, "рогатых коровам", "testGleicheFilter2")); //gen,pl + dat,pl

    UNIT_ASSERT(ShouldNotFind(gzt, "рогатым коровой", "testGleicheFilter2")); //dat,pl + abl,sg
    UNIT_ASSERT(ShouldNotFind(gzt, "рогатым корове", "testGleicheFilter2")); //dat,pl + dat,sg
}

/*
UTestFilter "testANDFilter1" { key = { "кот" gram = { "nom" word = 1 } gram = { "pl" } } }
UTestFilter "testANDFilter2" { key = { "кот" gram = { "gen" word = 1 } gram = { "sg" word = 1 } } }
*/
void TGazetteerTest::ERRORTestFiltersTwoGrams() { // этот тест фейлится из-за бага в газетире REQWIZARD-290
    TAutoPtr<TGazetteer> gztPtr = CreateGazetteerFromArchive("test_filters.gzt");
    TGazetteer* gzt = gztPtr.Get();

    UNIT_ASSERT(ShouldNotFind(gzt, "кот", "testANDFilter1")); //nom,sg
    UNIT_ASSERT(ShouldNotFind(gzt, "кота", "testANDFilter1")); //gen,sg | acc,sg
    UNIT_ASSERT(ShouldNotFind(gzt, "коту", "testANDFilter1")); //dat,sg
    UNIT_ASSERT(ShouldNotFind(gzt, "котом", "testANDFilter1")); //ins,sg
    UNIT_ASSERT(ShouldNotFind(gzt, "коте", "testANDFilter1"));  //abl,sg

    UNIT_ASSERT(ShouldFind(gzt, "коты", "testANDFilter1")); //nom,pl
    UNIT_ASSERT(ShouldNotFind(gzt, "котов", "testANDFilter1")); //gen,pl | acc,pl
    UNIT_ASSERT(ShouldNotFind(gzt, "котам", "testANDFilter1")); //dat,pl
    UNIT_ASSERT(ShouldNotFind(gzt, "котами", "testANDFilter1")); //ins,pl
    UNIT_ASSERT(ShouldNotFind(gzt, "котах", "testANDFilter1"));  //abl,pl

    UNIT_ASSERT(ShouldNotFind(gzt, "кот", "testANDFilter2")); //nom,sg
    UNIT_ASSERT(ShouldFind(gzt, "кота", "testANDFilter2")); //gen,sg | acc,sg
    UNIT_ASSERT(ShouldNotFind(gzt, "коту", "testANDFilter2")); //dat,sg
    UNIT_ASSERT(ShouldNotFind(gzt, "котом", "testANDFilter2")); //ins,sg
    UNIT_ASSERT(ShouldNotFind(gzt, "коте", "testANDFilter2"));  //abl,sg

    UNIT_ASSERT(ShouldNotFind(gzt, "коты", "testANDFilter2")); //nom,pl
    UNIT_ASSERT(ShouldNotFind(gzt, "котов", "testANDFilter2")); //gen,pl | acc,pl
    UNIT_ASSERT(ShouldNotFind(gzt, "котам", "testANDFilter2")); //dat,pl
    UNIT_ASSERT(ShouldNotFind(gzt, "котами", "testANDFilter2")); //ins,pl
    UNIT_ASSERT(ShouldNotFind(gzt, "котах", "testANDFilter2"));  //abl,pl

}

/*
UTestTokenize "testTokenizeCC0" { {"abc123-5" } }
UTestTokenize "testTokenizeCC1" { {"abc123-5" tokenize = CHAR_CLASS } }
UTestTokenize "testTokenizeCC2" { { "северо-западный мекленбург" tokenize = CHAR_CLASS gram = { "ед,муж,дат" word=2 } } }
UTestTokenize "testTokenizeCC3" { { "(северо-западно-восточный) мекленбург" tokenize = CHAR_CLASS } }
UTestTokenize "testTokenizeCC4" { { "дроид r2d2 на татуин" tokenize = CHAR_CLASS gram = { "дат" word=4 } } }
UTestTokenize "testTokenizeNA1" { {"abc123-5" tokenize = NON_ALNUM } }
UTestTokenize "testTokenizeNA2" { { "северо-западный мекленбург" tokenize = NON_ALNUM } }
UTestTokenize "testTokenizeNA3" { { "(северо-западно-восточный) мекленбург" tokenize = NON_ALNUM } }

*/
void TGazetteerTest::TestTokenize() {
    TAutoPtr<TGazetteer> gztPtr = CreateGazetteerFromArchive("test_tokenize.gzt");
    TGazetteer* gzt = gztPtr.Get();

    UNIT_ASSERT(ShouldFind(gzt, "abc 123 5", "testTokenizeCC1"));
    UNIT_ASSERT(ShouldNotFind(gzt, "abc123-5", "testTokenizeCC1")); //not found because of tokenizing

    UNIT_ASSERT(ShouldFind(gzt, "северо западному мекленбургу", "testTokenizeCC2"));
    UNIT_ASSERT(ShouldFind(gzt, "северо западный мекленбургу", "testTokenizeCC2"));
    UNIT_ASSERT(ShouldNotFind(gzt, "северо-западный мекленбург", "testTokenizeCC2"));
    UNIT_ASSERT(ShouldNotFind(gzt, "северо-западного мекленбурга", "testTokenizeCC2"));
    UNIT_ASSERT(ShouldNotFind(gzt, "северо-западным мекленбургом", "testTokenizeCC2"));
    UNIT_ASSERT(ShouldNotFind(gzt, "северо-западные мекленбурги", "testTokenizeCC2"));
    UNIT_ASSERT(ShouldNotFind(gzt, "северо западному мекленбурга", "testTokenizeCC2"));

    UNIT_ASSERT(ShouldFind(gzt, "дроид r2d2 на татуину", "testTokenizeCC4"));
    UNIT_ASSERT(ShouldFind(gzt, "дроид r 2 d 2 на татуину", "testTokenizeCC4"));
    UNIT_ASSERT(ShouldNotFind(gzt, "дроид r 2 d 2 на татуином", "testTokenizeCC4"));


    UNIT_ASSERT(ShouldFind(gzt, "северо западно восточный мекленбург", "testTokenizeCC3"));

    UNIT_ASSERT(ShouldFind(gzt, "abc123 5", "testTokenizeNA1"));
    UNIT_ASSERT(ShouldFind(gzt, "abc123-5", "testTokenizeNA1"));
    UNIT_ASSERT(ShouldNotFind(gzt, "abc 123 5", "testTokenizeNA1"));

    UNIT_ASSERT(ShouldFind(gzt, "северо западный мекленбург", "testTokenizeNA2"));
    UNIT_ASSERT(ShouldFind(gzt, "северо-западный мекленбург", "testTokenizeNA2"));

    UNIT_ASSERT(ShouldFind(gzt, "северо западно восточный мекленбург", "testTokenizeNA3"));

}

/*
TArticle "turTestPanLabirinth1" { key = { "Pan'ın Labirenti" } }
TArticle "turTestPanLabirinth2" { key = { "Panın Labirenti" } }
TArticle "turTestPanLabirinth3" { key = { "Pan ın Labirenti" } }
*/
void TGazetteerTest::TestApostrophe() {
    TAutoPtr<TGazetteer> gztPtr = CreateGazetteerFromArchive("test_tokenize.gzt");
    TGazetteer* gzt = gztPtr.Get();

    UNIT_ASSERT(ShouldFind(gzt, "Pan'ın Labirenti", "turTestPanLabirinth1"));
    UNIT_ASSERT(ShouldNotFind(gzt, "Pan'ın Labirenti", "turTestPanLabirinth2"));
    UNIT_ASSERT(ShouldNotFind(gzt, "Pan'ın Labirenti", "turTestPanLabirinth3"));
}

//class TRetokenizer;
void TGazetteerTest::TestRetokenizer() {
    TSearchKey key;
    TRetokenizer retokenizer(key);
    TUtf16String test = u"дроид R2/D2 принадлежал Йоде";

    TUtf16String test1 = test;
    retokenizer.Do(test1);
    UNIT_ASSERT_EQUAL(test1, test);

    TUtf16String test2 = test;
    key.mutable_tokenize()->set_delim(TTokenizeOptions::CHAR_CLASS);
    retokenizer.Do(test2);
    UNIT_ASSERT_EQUAL(test2, u"дроид R 2 D 2 принадлежал Йоде");

    TUtf16String test3 = u"северо-западный";
    retokenizer.Do(test3);
    UNIT_ASSERT_EQUAL(test3, u"северо западный");

    TUtf16String test4 = test;
    key.mutable_tokenize()->set_delim(TTokenizeOptions::NON_ALNUM);
    retokenizer.Do(test4);
    UNIT_ASSERT_EQUAL(test4, u"дроид R2 D2 принадлежал Йоде");

    TUtf16String test5 = u"!izmirspor-asker !hastanesi-264 . sokak";
    key.mutable_tokenize()->set_delim(TTokenizeOptions::NON_ALNUM);
    retokenizer.Do(test5);
    UNIT_ASSERT_EQUAL(test5, u"!izmirspor asker !hastanesi 264   sokak");

}
/*
UTestCompile "testCompile1" { key = {"корова" morph=EXACT_FORM }};

UTestCompile "testCompile2" { "рублёвка", "stringparam", 3, 4.4, 0, {["rr1,rr2"]} }
*/
void TGazetteerTest::TestGztTrie() {
    TAutoPtr<TGazetteer> gztPtr = CreateGazetteerFromArchive("test_compile.gzt");
    TGazetteer* gzt = gztPtr.Get();
    UNIT_ASSERT(gzt->GztTrie().HasExactWord(u"корова"));
    UNIT_ASSERT(gzt->GztTrie().HasLemma(u"рублёвка"));
}

void TGazetteerTest::TestDiacritic1() {
    TAutoPtr<TGazetteer> gztPtr = CreateGazetteerFromArchive("test_norm.gzt");
    TGazetteer* gzt = gztPtr.Get();

    UNIT_ASSERT(ShouldFindAll(gzt, "еж", LANG_RUS, "ruTestNormNumber1", "ruTestNormNumber2", "ruTestNormNumber3"));

    UNIT_ASSERT(!ShouldFind(gzt, "иог", "ruTestNormNumber4"));
    UNIT_ASSERT(ShouldFind(gzt, "иог", "ruTestNormNumber5"));
    UNIT_ASSERT(!ShouldFind(gzt, "иог", "ruTestNormNumber6"));

    UNIT_ASSERT(ShouldFindAll(gzt, "cetinkale", LANG_TUR, "trTestNormNumber1", "trTestNormNumber2", "trTestNormNumber3"));
    UNIT_ASSERT(ShouldFindAll(gzt, "dogusoy", LANG_TUR, "trTestNormNumber4", "trTestNormNumber5", "trTestNormNumber6"));
    UNIT_ASSERT(ShouldFindAll(gzt, "ozlen", LANG_TUR, "trTestNormNumber7", "trTestNormNumber8", "trTestNormNumber9"));
    UNIT_ASSERT(ShouldFindAll(gzt, "oksuzoglu", LANG_TUR, "trTestNormNumber10", "trTestNormNumber11", "trTestNormNumber12"));

    UNIT_ASSERT(ShouldFindAll(gzt, "istanbul", LANG_TUR, "trTestNormNumber13", "trTestNormNumber14", "trTestNormNumber15"));

    // 'i' with dot
    UNIT_ASSERT(ShouldFindAll(gzt, "irak", LANG_TUR, "trTestNormNumber16", "trTestNormNumber17", "trTestNormNumber18"));
    UNIT_ASSERT(ShouldFindAll(gzt, "İrak", LANG_TUR, "trTestNormNumber16", "trTestNormNumber17", "trTestNormNumber18"));

    // 'i' without dot (should find the same articles)
    UNIT_ASSERT(ShouldFindAll(gzt, "ırak", LANG_TUR, "trTestNormNumber16", "trTestNormNumber17", "trTestNormNumber18"));
    UNIT_ASSERT(ShouldFindAll(gzt, "Irak", LANG_TUR, "trTestNormNumber16", "trTestNormNumber17", "trTestNormNumber18"));

    UNIT_ASSERT(ShouldFindAll(gzt, "габлі", LANG_UKR, "ukTestNormNumber1", "ukTestNormNumber2", "ukTestNormNumber3"));
}

void TGazetteerTest::TestDiacritic2() {
    TAutoPtr<TGazetteer> gztPtr = CreateGazetteerFromArchive("test_diacritics1.gzt");
    TGazetteer* gzt = gztPtr.Get();

    UNIT_ASSERT(ShouldFind(gzt, "королев", "testDiacriticRu1"));
    UNIT_ASSERT(ShouldFind(gzt, "королёв", "testDiacriticRu1"));

    UNIT_ASSERT(ShouldFind(gzt, "королев", "testDiacriticRu2"));
    UNIT_ASSERT(ShouldFind(gzt, "королёв", "testDiacriticRu2"));

    UNIT_ASSERT(ShouldFind(gzt, "королев", "testDiacriticRu3"));
    UNIT_ASSERT(ShouldFind(gzt, "королёв", "testDiacriticRu3"));

    UNIT_ASSERT(ShouldFind(gzt, "королев", "testDiacriticRu4"));
    UNIT_ASSERT(ShouldFind(gzt, "королёв", "testDiacriticRu4"));
}

void TGazetteerTest::TestDiacritic3() {
    TAutoPtr<TGazetteer> gztPtr = CreateGazetteerFromArchive("test_diacritics2.gzt");
    TGazetteer* gzt = gztPtr.Get();

    UNIT_ASSERT(ShouldFind(gzt, "улица Намёткина", "testDiacriticRu1"));
    UNIT_ASSERT(ShouldFind(gzt, "улица Наметкина", "testDiacriticRu1"));

    UNIT_ASSERT(ShouldFind(gzt, "улица Намёткина", "testDiacriticRu2"));
    UNIT_ASSERT(ShouldFind(gzt, "улица Наметкина", "testDiacriticRu2"));

    UNIT_ASSERT(ShouldFind(gzt, "улица Намёткина", "testDiacriticRu3"));
    UNIT_ASSERT(ShouldFind(gzt, "улица Наметкина", "testDiacriticRu3"));

    UNIT_ASSERT(ShouldFind(gzt, "улица Намёткина", "testDiacriticRu4"));
    UNIT_ASSERT(ShouldFind(gzt, "улица Наметкина", "testDiacriticRu4"));
}

#define CHECK_SIMPLE_WORD(tok, text, pos) \
    UNIT_ASSERT_EQUAL(WideToUTF8(tok.GetOriginalText()), text); \
    UNIT_ASSERT_EQUAL(tok.GetPosition(), pos);

void TGazetteerTest::TestMultiTokenNone() {
    TSimpleText text(LANG_RUS, TSimpleText::None);

    text.Reset(UTF8ToWide(TStringBuf("Пушкин А.С.")));
    UNIT_ASSERT_EQUAL(text.size(), 2);
    CHECK_SIMPLE_WORD(text[0], "Пушкин", 0);
    CHECK_SIMPLE_WORD(text[1], "А.С", 7);

    text.Reset(UTF8ToWide(TStringBuf("Пушкин А. С.")));
    UNIT_ASSERT_EQUAL(text.size(), 3);
    CHECK_SIMPLE_WORD(text[0], "Пушкин", 0);
    CHECK_SIMPLE_WORD(text[1], "А", 7);
    CHECK_SIMPLE_WORD(text[2], "С", 10);

    text.Reset(UTF8ToWide(TStringBuf("mp3")));
    UNIT_ASSERT_EQUAL(text.size(), 1);
    CHECK_SIMPLE_WORD(text[0], "mp3", 0);

    text.Reset(UTF8ToWide(TStringBuf("г.Ростов-на-Дону")));
    UNIT_ASSERT_EQUAL(text.size(), 1);
    CHECK_SIMPLE_WORD(text[0], "г.Ростов-на-Дону", 0);

    text.Reset(UTF8ToWide(TStringBuf("$400")));
    UNIT_ASSERT_EQUAL(text.size(), 1);
    CHECK_SIMPLE_WORD(text[0], "$400", 0);

    text.Reset(UTF8ToWide(TStringBuf("C++")));
    UNIT_ASSERT_EQUAL(text.size(), 1);
    CHECK_SIMPLE_WORD(text[0], "C++", 0);
}

void TGazetteerTest::TestMultiTokenWizard() {
    TSimpleText text(LANG_RUS, TSimpleText::Wizard);

    text.Reset(UTF8ToWide(TStringBuf("Пушкин А.С.")));
    UNIT_ASSERT_EQUAL(text.size(), 3);
    CHECK_SIMPLE_WORD(text[0], "Пушкин", 0);
    CHECK_SIMPLE_WORD(text[1], "А", 7);
    CHECK_SIMPLE_WORD(text[2], "С", 9);

    text.Reset(UTF8ToWide(TStringBuf("Пушкин А. С.")));
    UNIT_ASSERT_EQUAL(text.size(), 3);
    CHECK_SIMPLE_WORD(text[0], "Пушкин", 0);
    CHECK_SIMPLE_WORD(text[1], "А", 7);
    CHECK_SIMPLE_WORD(text[2], "С", 10);

    text.Reset(UTF8ToWide(TStringBuf("mp3")));
    UNIT_ASSERT_EQUAL(text.size(), 1);
    CHECK_SIMPLE_WORD(text[0], "mp3", 0);

    text.Reset(UTF8ToWide(TStringBuf("г.Ростов-на-Дону")));
    UNIT_ASSERT_EQUAL(text.size(), 2);
    CHECK_SIMPLE_WORD(text[0], "г", 0);
    CHECK_SIMPLE_WORD(text[1], "Ростов-на-Дону", 2);

    text.Reset(UTF8ToWide(TStringBuf("д'Артаньян")));
    UNIT_ASSERT_EQUAL(text.size(), 1);
    CHECK_SIMPLE_WORD(text[0], "д'Артаньян", 0);

    text.Reset(UTF8ToWide(TStringBuf("$400")));
    UNIT_ASSERT_EQUAL(text.size(), 1);
    CHECK_SIMPLE_WORD(text[0], "$400", 0);

    text.Reset(UTF8ToWide(TStringBuf("C++")));
    UNIT_ASSERT_EQUAL(text.size(), 1);
    CHECK_SIMPLE_WORD(text[0], "C++", 0);
}

void TGazetteerTest::TestMultiTokenIndex() {
    TSimpleText text(LANG_RUS, TSimpleText::Index);

    text.Reset(UTF8ToWide(TStringBuf("Пушкин А.С.")));
    UNIT_ASSERT_EQUAL(text.size(), 3);
    CHECK_SIMPLE_WORD(text[0], "Пушкин", 0);
    CHECK_SIMPLE_WORD(text[1], "А", 7);
    CHECK_SIMPLE_WORD(text[2], "С", 9);

    text.Reset(UTF8ToWide(TStringBuf("Пушкин А. С.")));
    UNIT_ASSERT_EQUAL(text.size(), 3);
    CHECK_SIMPLE_WORD(text[0], "Пушкин", 0);
    CHECK_SIMPLE_WORD(text[1], "А", 7);
    CHECK_SIMPLE_WORD(text[2], "С", 10);

    text.Reset(UTF8ToWide(TStringBuf("mp3")));
    UNIT_ASSERT_EQUAL(text.size(), 2);
    CHECK_SIMPLE_WORD(text[0], "mp", 0);
    CHECK_SIMPLE_WORD(text[1], "3", 2);

    text.Reset(UTF8ToWide(TStringBuf("г.Ростов-на-Дону")));
    UNIT_ASSERT_EQUAL(text.size(), 2);
    CHECK_SIMPLE_WORD(text[0], "г", 0);
    CHECK_SIMPLE_WORD(text[1], "Ростов-на-Дону", 2);

    text.Reset(UTF8ToWide(TStringBuf("д'Артаньян")));
    UNIT_ASSERT_EQUAL(text.size(), 1);
    CHECK_SIMPLE_WORD(text[0], "д'Артаньян", 0);

    text.Reset(UTF8ToWide(TStringBuf("$400")));
    UNIT_ASSERT_EQUAL(text.size(), 1);
    CHECK_SIMPLE_WORD(text[0], "$400", 0);

    text.Reset(UTF8ToWide(TStringBuf("C++")));
    UNIT_ASSERT_EQUAL(text.size(), 1);
    CHECK_SIMPLE_WORD(text[0], "C++", 0);
}

// Punctuation
void TGazetteerTest::TestSimpleTextPunct() {
    TSimpleText text(LANG_RUS, TSimpleText::Index, true);

    text.Reset(UTF8ToWide(TStringBuf("Пушкин А.С.")));
    UNIT_ASSERT_EQUAL(text.size(), 5);
    CHECK_SIMPLE_WORD(text[0], "Пушкин", 0);
    CHECK_SIMPLE_WORD(text[1], "А", 7);
    CHECK_SIMPLE_WORD(text[2], ".", 8);
    CHECK_SIMPLE_WORD(text[3], "С", 9);
    CHECK_SIMPLE_WORD(text[4], ".", 10);

    text.Reset(UTF8ToWide(TStringBuf("Пушкин А. С.")));
    UNIT_ASSERT_EQUAL(text.size(), 5);
    CHECK_SIMPLE_WORD(text[0], "Пушкин", 0);
    CHECK_SIMPLE_WORD(text[1], "А", 7);
    CHECK_SIMPLE_WORD(text[2], ".", 8);
    CHECK_SIMPLE_WORD(text[3], "С", 10);
    CHECK_SIMPLE_WORD(text[4], ".", 11);

    text.Reset(UTF8ToWide(TStringBuf("mp3")));
    UNIT_ASSERT_EQUAL(text.size(), 2);
    CHECK_SIMPLE_WORD(text[0], "mp", 0);
    CHECK_SIMPLE_WORD(text[1], "3", 2);

    text.Reset(UTF8ToWide(TStringBuf("г.Ростов-на-Дону")));
    UNIT_ASSERT_EQUAL(text.size(), 3);
    CHECK_SIMPLE_WORD(text[0], "г", 0);
    CHECK_SIMPLE_WORD(text[1], ".", 1);
    CHECK_SIMPLE_WORD(text[2], "Ростов-на-Дону", 2);

    text.Reset(UTF8ToWide(TStringBuf("д'Артаньян")));
    UNIT_ASSERT_EQUAL(text.size(), 1);
    CHECK_SIMPLE_WORD(text[0], "д'Артаньян", 0);

    text.Reset(UTF8ToWide(TStringBuf("$400")));
    UNIT_ASSERT_EQUAL(text.size(), 2);
    CHECK_SIMPLE_WORD(text[0], "$", 0);
    CHECK_SIMPLE_WORD(text[1], "400", 1);

    text.Reset(UTF8ToWide(TStringBuf("C++")));
    UNIT_ASSERT_EQUAL(text.size(), 3);
    CHECK_SIMPLE_WORD(text[0], "C", 0);
    CHECK_SIMPLE_WORD(text[1], "+", 1);
    CHECK_SIMPLE_WORD(text[2], "+", 2);
}

void TGazetteerTest::TestArticlePoolIter() {
    TAutoPtr<TGazetteer> gztPtr = CreateGazetteerFromArchive("test_import.gzt");
    UNIT_ASSERT(gztPtr.Get() != nullptr);

    TArticlePool::TIterator it(gztPtr->ArticlePool());
    UNIT_ASSERT(it.Ok());
    UNIT_ASSERT_EQUAL(*it, 0);

    TArticlePtr art = it.GetArticle();
    UNIT_ASSERT(art.IsInstance<NGztUt::TTestImportArticle1>());
    UNIT_ASSERT_EQUAL(WideToUTF8(art.GetTitle()), "AAA");
    UNIT_ASSERT_EQUAL(art.As<NGztUt::TTestImportArticle1>()->field1(), "AAA");

    ++it;
    UNIT_ASSERT(it.Ok());
    art = it.GetArticle();
    UNIT_ASSERT(art.IsInstance<NGztUt::TTestImportArticle2>());
    UNIT_ASSERT_EQUAL(WideToUTF8(art.GetTitle()), "BBB");
    UNIT_ASSERT_EQUAL(art.As<NGztUt::TTestImportArticle2>()->field1(), "BBB");
    UNIT_ASSERT_EQUAL(art.As<NGztUt::TTestImportArticle2>()->field2(), 123);

    ++it;
    UNIT_ASSERT(!it.Ok());
}


UNIT_TEST_SUITE_REGISTRATION(TGazetteerTest);
