#include "remorph_api_ut_gzt.pb.h"

#include <kernel/remorph/api/remorph_wrap.h>

#include <library/cpp/archive/yarchive.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_value.h>

#include <util/folder/path.h>
#include <util/folder/tempdir.h>
#include <util/generic/ptr.h>
#include <util/generic/singleton.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/memory/blob.h>
#include <util/stream/file.h>
#include <util/stream/str.h>
#include <util/system/file.h>

#include <google/protobuf/io/coded_stream.h>

using namespace NRemorphAPI;

namespace {

static const char* FACTS_FILE_NAME = "facts.rmr";
static const char* RULES_FILE_NAME = "rules.rmr";
static const char* GAZETTEER_FILE_NAME = "gazetteer.gzt.bin";

static const unsigned char RESOURCES[] = {
    #include "resources.inc"
};

struct TResources: public TArchiveReader {
    TResources()
        : TArchiveReader(TBlob::NoCopy(RESOURCES, sizeof(RESOURCES)))
    {
    }
};

struct TGazetteerBlob: TBlob {
    TGazetteerBlob()
        : TBlob(Default<TResources>().ObjectBlobByKey("/remorph_api_ut_gzt.gzt.bin"))
    {
    }
};

inline TProcessorWrap CreateProcessor(TFactoryWrap& factory) {
    TTempDir dir;
    TFsPath factsPath = TFsPath(dir()) / FACTS_FILE_NAME;
    TFsPath rulesPath = TFsPath(dir()) / RULES_FILE_NAME;
    {
        TOFStream factsStream(factsPath.c_str());
        factsStream << "\
import \"factmeta.proto\";\n\
\n\
package NFact;\n\
\n\
message TestFact {\n\
    option (matcher) = \"" << RULES_FILE_NAME << "\";\n\
    option (matcher_type) = REMORPH;\n\
\n\
    repeated string in_par = 1 [(prime) = true];\n\
}\n\
";
    }
    {
        TOFStream rulesStream(rulesPath.c_str());
        rulesStream << "\
rule test1 = [?reg=\"^[(]$\"] (<in_par> .*) [?reg=\"^[)]$\"];\n\
";
    }
    return factory.CreateProcessor(factsPath.c_str());
}

inline TProcessorWrap CreateProcessorWithGzt(TFactoryWrap& factory) {
    TTempDir dir;
    TFsPath factsPath = TFsPath(dir()) / FACTS_FILE_NAME;
    TFsPath rulesPath = TFsPath(dir()) / RULES_FILE_NAME;
    TFsPath gazetteerPath = TFsPath(dir()) / GAZETTEER_FILE_NAME;
    {
        TOFStream factsStream(factsPath.c_str());
        factsStream << "\
import \"factmeta.proto\";\n\
\n\
package NFact;\n\
\n\
message ArticleFact {\n\
    option (matcher) = \"" << RULES_FILE_NAME << "\";\n\
    option (matcher_type) = REMORPH;\n\
    option (gazetteer) = \"" << GAZETTEER_FILE_NAME << "\";\n\
\n\
    repeated string art = 1 [(prime) = true];\n\
}\n\
";
    }
    {
        TOFStream rulesStream(rulesPath.c_str());
        rulesStream << "\
rule rule = (<art> [?gzt=article]);\n\
";
    }
    {
        const TGazetteerBlob& gazetteerBlob = Default<TGazetteerBlob>();
        TFile gazetteerFile(gazetteerPath.c_str(), CreateNew);
        gazetteerFile.Write(gazetteerBlob.AsCharPtr(), gazetteerBlob.Size());
    }
    return factory.CreateProcessor(factsPath.c_str());
}

inline TSentenceWrap GetParseResult(const char* text, bool withGzt = false) {
    TFactoryWrap factory = GetRemorphFactoryWrap();
    UNIT_ASSERT(factory.IsValid());
    UNIT_ASSERT_C(factory.SetLangs("ru,en"), factory.GetLastError());
    TProcessorWrap proc;
    if (withGzt) {
        proc = CreateProcessorWithGzt(factory);
    } else {
        proc = CreateProcessor(factory);
    }
    UNIT_ASSERT_C(proc.IsValid(), factory.GetLastError());
    factory.Reset(); // Test dependency tracking
    TResultsWrap res = proc.Process(text);
    UNIT_ASSERT(res.IsValid());
    proc.Reset(); // Test dependency tracking
    UNIT_ASSERT_EQUAL(res.GetSentenceCount(), 1);
    TSentenceWrap sent = res.GetSentence(0);
    UNIT_ASSERT(sent.IsValid());
    res.Reset(); // Test dependency tracking
    return sent;
}

}

Y_UNIT_TEST_SUITE(RemorphApi) {
    Y_UNIT_TEST(FactoryGet) {
        TFactoryWrap factory = GetRemorphFactoryWrap();
        UNIT_ASSERT(factory.IsValid());
        UNIT_ASSERT_EQUAL(factory.GetLastError(), NULL);
    }

    Y_UNIT_TEST(FactoryLangs) {
        TFactoryWrap factory = GetRemorphFactoryWrap();
        UNIT_ASSERT(factory.IsValid());
        UNIT_ASSERT_C(factory.SetLangs("rus"), factory.GetLastError());
        UNIT_ASSERT_UNEQUAL(factory.GetLangs(), NULL);
        UNIT_ASSERT_VALUES_EQUAL(factory.GetLangs(), "rus");
    }

    Y_UNIT_TEST(FactoryLangsDefault) {
        TFactoryWrap factory = GetRemorphFactoryWrap();
        UNIT_ASSERT(factory.IsValid());
        UNIT_ASSERT_UNEQUAL(factory.GetLangs(), NULL);
        UNIT_ASSERT_VALUES_EQUAL(factory.GetLangs(), "rus,eng,ukr");
    }

    Y_UNIT_TEST(FactoryLangsUnknown) {
        TFactoryWrap factory = GetRemorphFactoryWrap();
        UNIT_ASSERT(factory.IsValid());
        UNIT_ASSERT_C(factory.SetLangs("unk"), factory.GetLastError());
        UNIT_ASSERT_UNEQUAL(factory.GetLangs(), NULL);
        UNIT_ASSERT_VALUES_EQUAL(factory.GetLangs(), "unk");
        UNIT_ASSERT_C(factory.SetLangs(""), factory.GetLastError());
        UNIT_ASSERT_UNEQUAL(factory.GetLangs(), NULL);
        UNIT_ASSERT_VALUES_EQUAL(factory.GetLangs(), "unk");
        UNIT_ASSERT_C(factory.SetLangs(",,,"), factory.GetLastError());
        UNIT_ASSERT_UNEQUAL(factory.GetLangs(), NULL);
        UNIT_ASSERT_VALUES_EQUAL(factory.GetLangs(), "unk");
    }

    Y_UNIT_TEST(FactoryLangsIncorrect) {
        TFactoryWrap factory = GetRemorphFactoryWrap();
        UNIT_ASSERT(factory.IsValid());
        UNIT_ASSERT_C(factory.SetLangs("rus"), factory.GetLastError());
        UNIT_ASSERT_EQUAL(factory.SetLangs("totally-incorrect-language"), false);
        UNIT_ASSERT_UNEQUAL(factory.GetLastError(), NULL);
        UNIT_ASSERT_UNEQUAL(factory.GetLangs(), NULL);
        UNIT_ASSERT_VALUES_EQUAL(factory.GetLangs(), "rus");
        UNIT_ASSERT_EQUAL(factory.SetLangs("eng,totally-incorrect-language"), false);
        UNIT_ASSERT_UNEQUAL(factory.GetLastError(), NULL);
        UNIT_ASSERT_UNEQUAL(factory.GetLangs(), NULL);
        UNIT_ASSERT_VALUES_EQUAL(factory.GetLangs(), "rus");
        UNIT_ASSERT_EQUAL(factory.SetLangs(" "), false);
        UNIT_ASSERT_UNEQUAL(factory.GetLastError(), NULL);
        UNIT_ASSERT_UNEQUAL(factory.GetLangs(), NULL);
        UNIT_ASSERT_VALUES_EQUAL(factory.GetLangs(), "rus");
    }

    Y_UNIT_TEST(FactoryDetectSentences) {
        TFactoryWrap factory = GetRemorphFactoryWrap();
        UNIT_ASSERT(factory.IsValid());
        factory.SetDetectSentences(true);
        UNIT_ASSERT_EQUAL(factory.GetDetectSentences(), true);
        factory.SetDetectSentences(false);
        UNIT_ASSERT_EQUAL(factory.GetDetectSentences(), false);
    }

    Y_UNIT_TEST(FactoryDetectSentencesDefault) {
        TFactoryWrap factory = GetRemorphFactoryWrap();
        UNIT_ASSERT(factory.IsValid());
        UNIT_ASSERT_EQUAL(factory.GetDetectSentences(), true);
    }

    Y_UNIT_TEST(FactoryMaxSentenceTokens) {
        TFactoryWrap factory = GetRemorphFactoryWrap();
        UNIT_ASSERT(factory.IsValid());
        factory.SetMaxSentenceTokens(0);
        UNIT_ASSERT_EQUAL(factory.GetMaxSentenceTokens(), 0);
        factory.SetMaxSentenceTokens(1u);
        UNIT_ASSERT_EQUAL(factory.GetMaxSentenceTokens(), 1u);
        factory.SetMaxSentenceTokens(200u);
        UNIT_ASSERT_EQUAL(factory.GetMaxSentenceTokens(), 200u);
        unsigned long min = Min();
        factory.SetMaxSentenceTokens(min);
        UNIT_ASSERT_EQUAL(factory.GetMaxSentenceTokens(), min);
        unsigned long max = Max();
        factory.SetMaxSentenceTokens(max);
        UNIT_ASSERT_EQUAL(factory.GetMaxSentenceTokens(), max);
    }

    Y_UNIT_TEST(FactoryMaxSentenceTokensDefault) {
        TFactoryWrap factory = GetRemorphFactoryWrap();
        UNIT_ASSERT(factory.IsValid());
        UNIT_ASSERT_EQUAL(factory.GetMaxSentenceTokens(), 100u);
    }

    Y_UNIT_TEST(FactoryMultitokenSplitMode) {
        TFactoryWrap factory = GetRemorphFactoryWrap();
        UNIT_ASSERT(factory.IsValid());
        UNIT_ASSERT_C(factory.SetMultitokenSplitMode(MSM_MINIMAL), factory.GetLastError());
        UNIT_ASSERT_EQUAL(factory.GetMultitokenSplitMode(), MSM_MINIMAL);
        UNIT_ASSERT_C(factory.SetMultitokenSplitMode(MSM_SMART), factory.GetLastError());
        UNIT_ASSERT_EQUAL(factory.GetMultitokenSplitMode(), MSM_SMART);
        UNIT_ASSERT_C(factory.SetMultitokenSplitMode(MSM_ALL), factory.GetLastError());
        UNIT_ASSERT_EQUAL(factory.GetMultitokenSplitMode(), MSM_ALL);
    }

    Y_UNIT_TEST(FactoryMultitokenSplitModeDefault) {
        TFactoryWrap factory = GetRemorphFactoryWrap();
        UNIT_ASSERT(factory.IsValid());
        UNIT_ASSERT_EQUAL(factory.GetMultitokenSplitMode(), MSM_MINIMAL);
    }

#ifndef _ubsan_enabled_
    Y_UNIT_TEST(FactoryMultitokenSplitModeIncorrect) {
        TFactoryWrap factory = GetRemorphFactoryWrap();
        UNIT_ASSERT(factory.IsValid());
        UNIT_ASSERT_C(factory.SetMultitokenSplitMode(MSM_MINIMAL), factory.GetLastError());
        UNIT_ASSERT_EQUAL(factory.SetMultitokenSplitMode(static_cast<EMultitokenSplitMode>(-1)), false);
        UNIT_ASSERT_UNEQUAL(factory.GetLastError(), NULL);
        UNIT_ASSERT_EQUAL(factory.GetMultitokenSplitMode(), MSM_MINIMAL);
    }
#endif

    Y_UNIT_TEST(Load) {
        TFactoryWrap factory = GetRemorphFactoryWrap();
        UNIT_ASSERT(factory.IsValid());
        UNIT_ASSERT_C(factory.SetLangs("ru,en"), factory.GetLastError());
        factory.SetDetectSentences(false);
        factory.SetMaxSentenceTokens(0);
        TProcessorWrap proc = CreateProcessor(factory);
        UNIT_ASSERT_C(proc.IsValid(), factory.GetLastError());
        UNIT_ASSERT(factory.GetLastError() == nullptr);
    }

    Y_UNIT_TEST(ParseSplitSentences) {
        TFactoryWrap factory = GetRemorphFactoryWrap();
        UNIT_ASSERT(factory.IsValid());
        UNIT_ASSERT_C(factory.SetLangs("ru,en"), factory.GetLastError());
        TProcessorWrap proc = CreateProcessor(factory);
        UNIT_ASSERT_C(proc.IsValid(), factory.GetLastError());
        TResultsWrap res = proc.Process("Some text. Another text");
        UNIT_ASSERT(res.IsValid());
        UNIT_ASSERT_EQUAL(res.GetSentenceCount(), 2);
        factory.Reset(); // Test dependency tracking
        proc.Reset();

        TSentenceWrap sent = res.GetSentence(0);
        UNIT_ASSERT(sent.IsValid());
        UNIT_ASSERT_EQUAL_C(sent.GetText(), TStringBuf("Some text. "), sent.GetText());
        UNIT_ASSERT_EQUAL(sent.GetAllFacts().GetFactCount(), 0);
        UNIT_ASSERT_EQUAL(sent.GetTokens().GetTokenCount(), 3);
        UNIT_ASSERT_EQUAL(sent.GetTokens().GetToken(0).GetText(), TStringBuf("Some"));
        UNIT_ASSERT_EQUAL(sent.GetTokens().GetToken(1).GetText(), TStringBuf("text"));
        UNIT_ASSERT_EQUAL(sent.GetTokens().GetToken(2).GetText(), TStringBuf("."));

        sent = res.GetSentence(1);
        UNIT_ASSERT(sent.IsValid());
        UNIT_ASSERT_EQUAL_C(sent.GetText(), TStringBuf("Another text"), sent.GetText());
        UNIT_ASSERT_EQUAL(sent.GetAllFacts().GetFactCount(), 0);
        TTokensWrap tokens = sent.GetTokens();
        sent.Reset(); // Test dependency tracking
        UNIT_ASSERT_EQUAL(tokens.GetTokenCount(), 2);
        UNIT_ASSERT_EQUAL(tokens.GetToken(0).GetText(), TStringBuf("Another"));
        TTokenWrap token = tokens.GetToken(1);
        tokens.Reset(); // Test dependency tracking
        UNIT_ASSERT_EQUAL(token.GetText(), TStringBuf("text"));
    }

    Y_UNIT_TEST(ParseFact) {
        TSentenceWrap sent = GetParseResult("(some text)");
        UNIT_ASSERT_EQUAL(sent.GetAllFacts().GetFactCount(), 1);

        TFactWrap fact = sent.GetAllFacts().GetFact(0);
        UNIT_ASSERT(fact.IsValid());
        sent.Reset(); // Test dependency tracking
        UNIT_ASSERT_EQUAL_C(fact.GetName(), TStringBuf("TestFact"), ", actual: \'" << fact.GetName() << "'");
        UNIT_ASSERT_EQUAL_C(fact.GetType(), TStringBuf("TestFact"), ", actual: \'" << fact.GetType() << "'");
        UNIT_ASSERT_EQUAL_C(fact.GetValue(), TStringBuf("some text"), ", actual: \'" << fact.GetValue() << "'");
        UNIT_ASSERT_EQUAL(fact.GetStartToken(), 1);
        UNIT_ASSERT_EQUAL(fact.GetEndToken(), 3);
        UNIT_ASSERT_EQUAL(fact.GetStartSentPos(), 1);
        UNIT_ASSERT_EQUAL(fact.GetEndSentPos(), 10);
        UNIT_ASSERT_EQUAL(fact.GetTokens().GetTokenCount(), 2);
        UNIT_ASSERT_EQUAL(fact.GetTokens().GetToken(0).GetText(), TStringBuf("some"));
        UNIT_ASSERT_EQUAL(fact.GetTokens().GetToken(1).GetText(), TStringBuf("text"));

        UNIT_ASSERT_EQUAL(fact.GetFields("in_par").GetFieldCount(), 1);
        UNIT_ASSERT_EQUAL(fact.GetFields("in_par").GetField(0).GetValue(), TStringBuf("some text"));

        UNIT_ASSERT_EQUAL(fact.GetCompoundFields().GetCompoundFieldCount(), 0);

        UNIT_ASSERT_EQUAL(fact.GetFields().GetFieldCount(), 1);
        TFieldWrap field = fact.GetFields().GetField(0);
        UNIT_ASSERT(field.IsValid());
        fact.Reset(); // Test dependency tracking
        UNIT_ASSERT_EQUAL(field.GetName(), TStringBuf("in_par"));
        UNIT_ASSERT_EQUAL(field.GetValue(), TStringBuf("some text"));
        UNIT_ASSERT_EQUAL(field.GetStartToken(), 1);
        UNIT_ASSERT_EQUAL(field.GetEndToken(), 3);
        UNIT_ASSERT_EQUAL(field.GetStartSentPos(), 1);
        UNIT_ASSERT_EQUAL(field.GetEndSentPos(), 10);
        UNIT_ASSERT_EQUAL(field.GetTokens().GetTokenCount(), 2);
        UNIT_ASSERT_EQUAL(field.GetTokens().GetToken(0).GetText(), TStringBuf("some"));
        UNIT_ASSERT_EQUAL(field.GetTokens().GetToken(1).GetText(), TStringBuf("text"));
    }

    Y_UNIT_TEST(ParseBestSolution) {
        TSentenceWrap sent = GetParseResult("(one two (three four)");
        UNIT_ASSERT_EQUAL(sent.GetAllFacts().GetFactCount(), 2);

        TFactsWrap sol = sent.FindBestSolution();
        sent.Reset(); // Test dependency tracking
        UNIT_ASSERT_EQUAL(sol.GetFactCount(), 1);
        UNIT_ASSERT_EQUAL(sol.GetFact(0).GetValue(), TStringBuf("one two (three four"));
    }

    Y_UNIT_TEST(ParseBestSolutionExplicitRanking) {
        TSentenceWrap sent = GetParseResult("(one two (three four)");
        UNIT_ASSERT_EQUAL(sent.GetAllFacts().GetFactCount(), 2);

        const ERankCheck weightRankMethod[] = {RC_G_WEIGHT};
        TFactsWrap sol = sent.FindBestSolution(weightRankMethod, 1u);
        UNIT_ASSERT_EQUAL(sol.GetFactCount(), 1);
        UNIT_ASSERT_EQUAL(sol.GetFact(0).GetValue(), TStringBuf("three four"));
    }

    Y_UNIT_TEST(ParseAllSolutions) {
        TSentenceWrap sent = GetParseResult("(one two) (three) four)");
        UNIT_ASSERT_EQUAL(sent.GetAllFacts().GetFactCount(), 10);

        TSolutionsWrap sols = sent.FindAllSolutions(50);
        sent.Reset(); // Test dependency tracking
        UNIT_ASSERT_EQUAL(sols.GetSolutionCount(), 4);

        TFactsWrap sol = sols.GetSolution(0);
        UNIT_ASSERT_EQUAL(sol.GetFactCount(), 1);
        UNIT_ASSERT_EQUAL(sol.GetFact(0).GetValue(), TStringBuf("one two) (three) four"));

        sol = sols.GetSolution(1);
        UNIT_ASSERT_EQUAL(sol.GetFactCount(), 1);
        UNIT_ASSERT_EQUAL(sol.GetFact(0).GetValue(), TStringBuf("one two) (three"));

        sol = sols.GetSolution(2);
        UNIT_ASSERT_EQUAL(sol.GetFactCount(), 2);
        UNIT_ASSERT_EQUAL(sol.GetFact(0).GetValue(), TStringBuf("one two"));
        UNIT_ASSERT_EQUAL(sol.GetFact(1).GetValue(), TStringBuf("three) four"));

        sol = sols.GetSolution(3);
        sols.Reset(); // Test dependency tracking
        UNIT_ASSERT_EQUAL(sol.GetFactCount(), 2);
        UNIT_ASSERT_EQUAL(sol.GetFact(0).GetValue(), TStringBuf("one two"));
        TFactWrap fact = sol.GetFact(1);
        sol.Reset(); // Test dependency tracking
        UNIT_ASSERT_EQUAL(fact.GetValue(), TStringBuf("three"));
    }

    Y_UNIT_TEST(ParseAllSolutionsExplicitRanking) {
        TSentenceWrap sent = GetParseResult("(one two) (three) four)");
        UNIT_ASSERT_EQUAL(sent.GetAllFacts().GetFactCount(), 10);

        const ERankCheck weightRankMethod[] = {RC_G_WEIGHT};
        TSolutionsWrap sols = sent.FindAllSolutions(50, weightRankMethod, 1u);
        UNIT_ASSERT_EQUAL(sols.GetSolutionCount(), 4);

        TFactsWrap sol = sols.GetSolution(0);
        UNIT_ASSERT_EQUAL(sol.GetFactCount(), 2);
        UNIT_ASSERT_EQUAL(sol.GetFact(0).GetValue(), TStringBuf("one two"));
        UNIT_ASSERT_EQUAL(sol.GetFact(1).GetValue(), TStringBuf("three"));

        sol = sols.GetSolution(1);
        UNIT_ASSERT_EQUAL(sol.GetFactCount(), 2);
        UNIT_ASSERT_EQUAL(sol.GetFact(0).GetValue(), TStringBuf("one two"));
        UNIT_ASSERT_EQUAL(sol.GetFact(1).GetValue(), TStringBuf("three) four"));

        sol = sols.GetSolution(2);
        UNIT_ASSERT_EQUAL(sol.GetFactCount(), 1);
        UNIT_ASSERT_EQUAL(sol.GetFact(0).GetValue(), TStringBuf("one two) (three"));

        sol = sols.GetSolution(3);
        UNIT_ASSERT_EQUAL(sol.GetFactCount(), 1);
        UNIT_ASSERT_EQUAL(sol.GetFact(0).GetValue(), TStringBuf("one two) (three) four"));
    }

    Y_UNIT_TEST(ParseAllSolutionsNoCountRanking) {
        TSentenceWrap sent = GetParseResult("(one two) (three) four)");
        UNIT_ASSERT_EQUAL(sent.GetAllFacts().GetFactCount(), 10);

        const ERankCheck noCountRankMethod[] = {RC_G_COVERAGE, RC_G_WEIGHT};
        TSolutionsWrap sols = sent.FindAllSolutions(50, noCountRankMethod, 2u);
        UNIT_ASSERT_EQUAL(sols.GetSolutionCount(), 4);

        TFactsWrap sol = sols.GetSolution(0);
        UNIT_ASSERT_EQUAL(sol.GetFactCount(), 1);
        UNIT_ASSERT_EQUAL(sol.GetFact(0).GetValue(), TStringBuf("one two) (three) four"));

        sol = sols.GetSolution(1);
        UNIT_ASSERT_EQUAL(sol.GetFactCount(), 2);
        UNIT_ASSERT_EQUAL(sol.GetFact(0).GetValue(), TStringBuf("one two"));
        UNIT_ASSERT_EQUAL(sol.GetFact(1).GetValue(), TStringBuf("three) four"));

        sol = sols.GetSolution(2);
        UNIT_ASSERT_EQUAL(sol.GetFactCount(), 1);
        UNIT_ASSERT_EQUAL(sol.GetFact(0).GetValue(), TStringBuf("one two) (three"));

        sol = sols.GetSolution(3);
        UNIT_ASSERT_EQUAL(sol.GetFactCount(), 2);
        UNIT_ASSERT_EQUAL(sol.GetFact(0).GetValue(), TStringBuf("one two"));
        UNIT_ASSERT_EQUAL(sol.GetFact(1).GetValue(), TStringBuf("three"));
    }

    Y_UNIT_TEST(GazetteerLoad) {
        TFactoryWrap factory = GetRemorphFactoryWrap();
        UNIT_ASSERT(factory.IsValid());
        TProcessorWrap proc = CreateProcessor(factory);
        UNIT_ASSERT_C(proc.IsValid(), factory.GetLastError());
        UNIT_ASSERT(factory.GetLastError() == nullptr);
    }

    Y_UNIT_TEST(GazetteerParse) {
        TSentenceWrap sent = GetParseResult("text key text", true);
        UNIT_ASSERT_EQUAL(sent.GetAllFacts().GetFactCount(), 1);

        TFactWrap fact = sent.GetAllFacts().GetFact(0);
        UNIT_ASSERT(fact.IsValid());
        UNIT_ASSERT_EQUAL_C(fact.GetName(), TStringBuf("ArticleFact"), ", actual: \'" << fact.GetName() << "'");
        UNIT_ASSERT_EQUAL_C(fact.GetType(), TStringBuf("ArticleFact"), ", actual: \'" << fact.GetType() << "'");
        UNIT_ASSERT_EQUAL_C(fact.GetValue(), TStringBuf("key"), ", actual: \'" << fact.GetValue() << "'");
        UNIT_ASSERT_EQUAL(fact.GetStartToken(), 1);
        UNIT_ASSERT_EQUAL(fact.GetEndToken(), 2);
        UNIT_ASSERT_EQUAL(fact.GetStartSentPos(), 5);
        UNIT_ASSERT_EQUAL(fact.GetEndSentPos(), 8);
        UNIT_ASSERT_EQUAL(fact.GetTokens().GetTokenCount(), 1);
        UNIT_ASSERT_EQUAL(fact.GetTokens().GetToken(0).GetText(), TStringBuf("key"));

        UNIT_ASSERT_EQUAL(fact.GetFields("art").GetFieldCount(), 1);
        UNIT_ASSERT_EQUAL(fact.GetFields("art").GetField(0).GetValue(), TStringBuf("key"));

        UNIT_ASSERT_EQUAL(fact.GetCompoundFields().GetCompoundFieldCount(), 0);

        UNIT_ASSERT_EQUAL(fact.GetFields().GetFieldCount(), 1);
        TFieldWrap field = fact.GetFields().GetField(0);
        UNIT_ASSERT(field.IsValid());
        UNIT_ASSERT_EQUAL(field.GetName(), TStringBuf("art"));
        UNIT_ASSERT_EQUAL(field.GetValue(), TStringBuf("key"));
        UNIT_ASSERT_EQUAL(field.GetStartToken(), 1);
        UNIT_ASSERT_EQUAL(field.GetEndToken(), 2);
        UNIT_ASSERT_EQUAL(field.GetStartSentPos(), 5);
        UNIT_ASSERT_EQUAL(field.GetEndSentPos(), 8);
        UNIT_ASSERT_EQUAL(field.GetTokens().GetTokenCount(), 1);
        UNIT_ASSERT_EQUAL(field.GetTokens().GetToken(0).GetText(), TStringBuf("key"));
    }

    Y_UNIT_TEST(GazetteerArticle) {
        TSentenceWrap sent = GetParseResult("text key text", true);
        UNIT_ASSERT_EQUAL(sent.GetAllFacts().GetFactCount(), 1);

        TFactWrap fact = sent.GetAllFacts().GetFact(0);
        UNIT_ASSERT(fact.IsValid());
        UNIT_ASSERT_EQUAL(fact.GetFields("art").GetFieldCount(), 1);

        TFieldWrap field = fact.GetFields("art").GetField(0);
        UNIT_ASSERT(field.IsValid());

        TArticlesWrap articles = field.GetArticles();
        UNIT_ASSERT(articles.IsValid());
        UNIT_ASSERT_EQUAL(articles.GetArticleCount(), 1);

        TArticleWrap article = articles.GetArticle(0);
        UNIT_ASSERT(article.IsValid());
        UNIT_ASSERT_EQUAL(article.GetType(), TStringBuf("TTestArticle"));
        UNIT_ASSERT_EQUAL(article.GetName(), TStringBuf("article"));
    }

    Y_UNIT_TEST(GazetteerArticleBlob) {
        TSentenceWrap sent = GetParseResult("text key text", true);
        UNIT_ASSERT_EQUAL(sent.GetAllFacts().GetFactCount(), 1);

        TFactWrap fact = sent.GetAllFacts().GetFact(0);
        UNIT_ASSERT(fact.IsValid());
        UNIT_ASSERT_EQUAL(fact.GetFields("art").GetFieldCount(), 1);

        TFieldWrap field = fact.GetFields("art").GetField(0);
        UNIT_ASSERT(field.IsValid());

        TArticlesWrap articles = field.GetArticles();
        UNIT_ASSERT(articles.IsValid());
        UNIT_ASSERT_EQUAL(articles.GetArticleCount(), 1);

        TArticleWrap article = articles.GetArticle(0);
        UNIT_ASSERT(article.IsValid());

        TBlobWrap blob = article.GetBlob();
        UNIT_ASSERT(blob.IsValid());
        UNIT_ASSERT(blob.GetData());

        TTestArticle testArticle;
        NProtoBuf::io::CodedInputStream blobStream(reinterpret_cast<const ui8*>(blob.GetData()), blob.GetSize());
        Y_PROTOBUF_SUPPRESS_NODISCARD testArticle.MergePartialFromCodedStream(&blobStream);
        UNIT_ASSERT(testArticle.Getlemma().Hastext());
        UNIT_ASSERT_EQUAL(testArticle.Getlemma().Gettext(), TStringBuf("k"));
    }

    Y_UNIT_TEST(GazetteerArticleJsonBlob) {
        TSentenceWrap sent = GetParseResult("text key text", true);
        UNIT_ASSERT_EQUAL(sent.GetAllFacts().GetFactCount(), 1);

        TFactWrap fact = sent.GetAllFacts().GetFact(0);
        UNIT_ASSERT(fact.IsValid());
        UNIT_ASSERT_EQUAL(fact.GetFields("art").GetFieldCount(), 1);

        TFieldWrap field = fact.GetFields("art").GetField(0);
        UNIT_ASSERT(field.IsValid());

        TArticlesWrap articles = field.GetArticles();
        UNIT_ASSERT(articles.IsValid());
        UNIT_ASSERT_EQUAL(articles.GetArticleCount(), 1);

        TArticleWrap article = articles.GetArticle(0);
        UNIT_ASSERT(article.IsValid());

        TBlobWrap jsonBlob = article.GetJsonBlob();
        UNIT_ASSERT(jsonBlob.IsValid());
        UNIT_ASSERT(jsonBlob.GetData());

        TString jsonData(jsonBlob.GetData(), jsonBlob.GetSize());
        TStringInput jsonStream(jsonData);

        NJson::TJsonValue json;
        NJson::TJsonReaderConfig jsonConfig;
        jsonConfig.AllowComments = false;
        jsonConfig.DontValidateUtf8 = false;
        UNIT_ASSERT(NJson::ReadJsonTree(&jsonStream, &jsonConfig, &json, true));

        const NJson::TJsonValue::TMapType& jsonMap = json.GetMap();
        UNIT_ASSERT_EQUAL(jsonMap.size(), 1);
        NJson::TJsonValue::TMapType::const_iterator jsonMapElem = jsonMap.find("lemma");
        UNIT_ASSERT_UNEQUAL(jsonMapElem, jsonMap.end());
        const NJson::TJsonValue& jsonLemma = jsonMapElem->second;

        const NJson::TJsonValue::TMapType& jsonLemmaMap = jsonLemma.GetMap();
        UNIT_ASSERT_EQUAL(jsonLemmaMap.size(), 1);
        NJson::TJsonValue::TMapType::const_iterator jsonLemmaMapElem = jsonLemmaMap.find("text");
        UNIT_ASSERT_UNEQUAL(jsonLemmaMapElem, jsonLemmaMap.end());
        const NJson::TJsonValue& jsonLemmaText = jsonLemmaMapElem->second;

        TString jsonLemmaTextString = jsonLemmaText.GetString();
        UNIT_ASSERT_EQUAL(jsonLemmaTextString, TStringBuf("k"));
    }

    Y_UNIT_TEST(Info) {
        TInfoWrap info = GetRemorphInfoWrap();
        UNIT_ASSERT(info.IsValid());

        UNIT_ASSERT_EQUAL(info.GetMajorVersion(), REMORPH_CODEBASE_VERSION_MAJOR);
        UNIT_ASSERT_EQUAL(info.GetMinorVersion(), REMORPH_CODEBASE_VERSION_MINOR);
        UNIT_ASSERT_EQUAL(info.GetPatchVersion(), REMORPH_CODEBASE_VERSION_PATCH);
    }

    Y_UNIT_TEST(WrapAssign) {
        TInfoWrap info = GetRemorphInfoWrap();
        UNIT_ASSERT(info.IsValid());
        {
            TInfoWrap info2 = GetRemorphInfoWrap();
            info2 = info;
            UNIT_ASSERT(info.IsValid());
            UNIT_ASSERT(info2.IsValid());
        }
        UNIT_ASSERT(info.IsValid());
    }

    Y_UNIT_TEST(WrapCopyConstruct) {
        TInfoWrap info = GetRemorphInfoWrap();
        UNIT_ASSERT(info.IsValid());
        {
            TInfoWrap info2 = info;
            UNIT_ASSERT(info.IsValid());
            UNIT_ASSERT(info2.IsValid());
        }
        UNIT_ASSERT(info.IsValid());
    }
}
