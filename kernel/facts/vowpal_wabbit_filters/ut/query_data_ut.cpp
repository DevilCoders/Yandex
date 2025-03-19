#include <library/cpp/testing/unittest/registar.h>

#include <kernel/facts/vowpal_wabbit_filters/vowpal_wabbit_filters.h>
#include <kernel/facts/vowpal_wabbit_filters/vowpal_wabbit_config.sc.h>

using namespace NVwFilters;


Y_UNIT_TEST_SUITE(TVowpalWabbitFilterQueryData) {
    Y_UNIT_TEST(TestUsage) {
        const TString query = "кто такой стример братишкин";
        const TString answer = "Стример Братишкин — талантливый блоггер, да и просто замечательный человек!";
        TQueryData queryData(query, answer);

        UNIT_ASSERT_STRINGS_EQUAL(queryData.GetQuery(), query);
        UNIT_ASSERT_STRINGS_EQUAL(queryData.GetAnswer(), answer);

        TVector<TString> queryTokens = {"кто", "такой", "стример", "братишкин"};
        UNIT_ASSERT_EQUAL(queryData.GetQueryTokens(), queryTokens);
        TVector<TString> answerTokens = {"Стример", "Братишкин", "—", "талантливый", "блоггер,", "да", "и", "просто", "замечательный", "человек!"};
        UNIT_ASSERT_EQUAL(queryData.GetAnswerTokens(), answerTokens);
    }

    Y_UNIT_TEST(TestReservedSymbols) {
        const TString query = "кто такой |стример| братишкин?";
        const TString answer = "Стример Братишкин : ||талантливый|| блоггер, да и ::просто замечательный:: человек|";
        TQueryData queryData(query, answer);

        TVector<TString> queryTokens = {"кто", "такой", "стример", "братишкин?"};
        UNIT_ASSERT_EQUAL(queryData.GetQueryTokens(), queryTokens);
        TVector<TString> answerTokens = {"Стример", "Братишкин", "", "талантливый", "блоггер,", "да", "и", "просто", "замечательный", "человек"};
        UNIT_ASSERT_EQUAL(queryData.GetAnswerTokens(), answerTokens);
    }
}
