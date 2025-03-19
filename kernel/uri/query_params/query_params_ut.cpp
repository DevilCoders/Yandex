#include <library/cpp/testing/unittest/registar.h>

#include "query_params.h"

template<bool withPath>
bool TestEmpty(const TString& url) {
    ::NUri::TUri uri;
    Y_ENSURE(uri.Parse(url, ::NUri::TUri::TFeature::FeaturesRecommended) == ::NUri::TUri::ParsedOK,
           "Parse failed " << url);

    Nydx::NUri::TQueryParams params(withPath, true, true);
    params.Parse(uri);

    return params.Begin() == params.End();
}

template<class ... Types>
bool TestSplit(const TString& url, bool sort, bool removeDups, bool parsePath, Types ... pValues) {
    TStringBuf values[] { pValues... };

    Nydx::NUri::TQueryParams params(parsePath, !sort, !removeDups);
    params.Parse(url);

    auto it1 = params.Begin();
    auto it2 = std::begin(values);
    for (; it1 != params.End() && it2 != std::end(values); ++it1, ++it2) {
        if (it1->Str() != *it2)
            return false;
    }
    return it1 == params.End() && it2 == std::end(values);
}

bool TestSave(const TString& url, bool withPath) {
    ::NUri::TUri uri1, uri2;
    Y_ENSURE(uri1.Parse(url, ::NUri::TUri::TFeature::FeaturesRecommended) == ::NUri::TUri::ParsedOK,
           "Parse failed " << url);
    uri2 = uri1;

    Nydx::NUri::TQueryParams params(withPath, true, true);
    params.Parse(uri1);
    params.Save(uri1);

    return uri1 == uri2;
}

template<class ... Types>
bool TestRemove(const TString& url, const TString& expected, bool withPath, Types ... pValues) {
    TStringBuf paramsToRemove[] { pValues... };

    ::NUri::TUri uri1, uri2;
    Y_ENSURE(uri1.Parse(url, ::NUri::TUri::TFeature::FeaturesRecommended) == ::NUri::TUri::ParsedOK,
           "Parse failed " << url);
    Y_ENSURE(uri2.Parse(expected, ::NUri::TUri::TFeature::FeaturesRecommended) == ::NUri::TUri::ParsedOK,
           "Parse failed " << expected);

    Nydx::NUri::TQueryParams params(withPath, true, true);

    params.Parse(uri1);
    for (auto it = std::begin(paramsToRemove); it != std::end(paramsToRemove); ++it) {
        params.EraseAll(*it);
    }
    params.Save(uri1);

    return uri1 == uri2;
}

template <bool withPath>
bool TestSetValue(const TString& url, const TString& key, const TString& newValue, const TString& expected) {
    ::NUri::TUri uri1, uri2;
    Y_ENSURE(uri1.Parse(url, ::NUri::TUri::TFeature::FeaturesRecommended) == ::NUri::TUri::ParsedOK,
           "Parse failed " << url);
    Y_ENSURE(uri2.Parse(expected, ::NUri::TUri::TFeature::FeaturesRecommended) == ::NUri::TUri::ParsedOK,
           "Parse failed " << expected);

    Nydx::NUri::TQueryParams params(withPath, true, true);
    params.Parse(uri1);
    params.SetValue(key, newValue);
    params.Save(uri1);

    return uri1 == uri2;
}

#define SPLIT_NO_SORT(url, ...) TestSplit(url, false, false, false, __VA_ARGS__)
#define SPLIT_SORT(url, ...) TestSplit(url, true, false, false, __VA_ARGS__)
#define SPLIT_REMOVE(url, ...) TestSplit(url, false, true, false, __VA_ARGS__)
#define SPLIT_PATH(url, ...) TestSplit(url, false, false, true, __VA_ARGS__)
#define SAVE_WITHOUT_PATH(url) TestSave(url, false)
#define SAVE_WITH_PATH(url) TestSave(url, true)
#define REMOVE_WITHOUT_PATH(url, expected, ...) TestRemove(url, expected, false, __VA_ARGS__)
#define REMOVE_WITH_PATH(url, expected, ...) TestRemove(url, expected, true, __VA_ARGS__)

Y_UNIT_TEST_SUITE(TQueryParamsTest)
{
    Y_UNIT_TEST(testNoParams)
    {
        UNIT_ASSERT(TestEmpty<false>("http://ya.ru/"));
        UNIT_ASSERT(TestEmpty<false>("http://ya.ru/path/"));
        UNIT_ASSERT(TestEmpty<false>("http://ya.ru/#asd"));
        UNIT_ASSERT(TestEmpty<false>("http://ya.ru/;x=1"));
        UNIT_ASSERT(TestEmpty<true>("http://ya.ru/asd&x=1"));
    }
    Y_UNIT_TEST(testEnumerateParams)
    {
        UNIT_ASSERT(SPLIT_NO_SORT("http://ya.ru/?x=a", "x=a"));
        UNIT_ASSERT(SPLIT_NO_SORT("http://ya.ru/?x=a&&", "", "", "x=a"));
        UNIT_ASSERT(SPLIT_NO_SORT("http://ya.ru/?x=a&b", "b", "x=a"));
        UNIT_ASSERT(SPLIT_NO_SORT("http://ya.ru/?x=a%26b", "x=a%26b"));
        UNIT_ASSERT(SPLIT_NO_SORT("http://ya.ru/?x=a&x=b", "x=a", "x=b"));
        UNIT_ASSERT(SPLIT_NO_SORT("http://ya.ru/?x=b&x=a", "x=a", "x=b"));
        UNIT_ASSERT(SPLIT_NO_SORT("http://ya.ru/?x=a&x=a", "x=a", "x=a"));
        UNIT_ASSERT(SPLIT_NO_SORT("http://ya.ru/path;sessid=42?x=a", "x=a"));
        UNIT_ASSERT(SPLIT_NO_SORT("http://ya.ru/path;sessid=42?x=a;x=b", "x=a", "x=b"));
        UNIT_ASSERT(SPLIT_NO_SORT("http://ya.ru/path;sessid=42?x=a;x=b&", "", "x=a;x=b"));
        UNIT_ASSERT(SPLIT_SORT("http://ya.ru/?x=a", "x=a"));
        UNIT_ASSERT(SPLIT_SORT("http://ya.ru/?x=a&&", "", "", "x=a"));
        UNIT_ASSERT(SPLIT_SORT("http://ya.ru/?x=a&b", "b", "x=a"));
        UNIT_ASSERT(SPLIT_SORT("http://ya.ru/?x=a%26b", "x=a%26b"));
        UNIT_ASSERT(SPLIT_SORT("http://ya.ru/?x=a&x=b", "x=a", "x=b"));
        UNIT_ASSERT(SPLIT_SORT("http://ya.ru/?x=b&x=a", "x=a", "x=b"));
        UNIT_ASSERT(SPLIT_SORT("http://ya.ru/?x=a&x=a", "x=a", "x=a"));
        UNIT_ASSERT(SPLIT_SORT("http://ya.ru/path;sessid=42?x=a", "x=a"));
        UNIT_ASSERT(SPLIT_SORT("http://ya.ru/path;sessid=42?x=a;x=b", "x=a", "x=b"));
        UNIT_ASSERT(SPLIT_SORT("http://ya.ru/path;sessid=42?x=a;x=b&", "", "x=a;x=b"));
        UNIT_ASSERT(SPLIT_REMOVE("http://ya.ru/?x=a", "x=a"));
        UNIT_ASSERT(SPLIT_REMOVE("http://ya.ru/?x=a&&", "", "x=a"));
        UNIT_ASSERT(SPLIT_REMOVE("http://ya.ru/?x=a&b", "b", "x=a"));
        UNIT_ASSERT(SPLIT_REMOVE("http://ya.ru/?x=a%26b", "x=a%26b"));
        UNIT_ASSERT(SPLIT_REMOVE("http://ya.ru/?x=a&x=b", "x=a", "x=b"));
        UNIT_ASSERT(SPLIT_REMOVE("http://ya.ru/?x=b&x=a", "x=a", "x=b"));
        UNIT_ASSERT(SPLIT_REMOVE("http://ya.ru/?x=a&x=a", "x=a"));
        UNIT_ASSERT(SPLIT_REMOVE("http://ya.ru/path;sessid=42?x=a", "x=a"));
        UNIT_ASSERT(SPLIT_REMOVE("http://ya.ru/path;sessid=42?x=a;x=b", "x=a", "x=b"));
        UNIT_ASSERT(SPLIT_PATH("http://ya.ru/path;sessid=42?x=a", "sessid=42", "x=a"));
        UNIT_ASSERT(SPLIT_PATH("http://ya.ru/path;sessid=42?x=a&sessid=42", "sessid=42", "sessid=42", "x=a"));
        UNIT_ASSERT(SPLIT_PATH("http://ya.ru/path;sessid=42?x=a;x=b&", "", "sessid=42", "x=a;x=b"));
    }
    Y_UNIT_TEST(testSaveParams)
    {
        UNIT_ASSERT(SAVE_WITHOUT_PATH("http://ya.ru/?x=a&"));
        UNIT_ASSERT(SAVE_WITHOUT_PATH("http://ya.ru/?x=a&&"));
        UNIT_ASSERT(SAVE_WITHOUT_PATH("http://ya.ru/?x=a&b"));
        UNIT_ASSERT(SAVE_WITHOUT_PATH("http://ya.ru/?x=a%26b"));
        UNIT_ASSERT(SAVE_WITHOUT_PATH("http://ya.ru/?x=a&x=b"));
        UNIT_ASSERT(SAVE_WITHOUT_PATH("http://ya.ru/?x=b&x=a"));
        UNIT_ASSERT(SAVE_WITHOUT_PATH("http://ya.ru/?x=a&x=a"));
        UNIT_ASSERT(SAVE_WITHOUT_PATH("http://ya.ru/path;sessid=42?x=a"));
        UNIT_ASSERT(SAVE_WITHOUT_PATH("http://ya.ru/path;a=1;b=2?x=a"));
        UNIT_ASSERT(SAVE_WITHOUT_PATH("http://ya.ru/path;a=1;;b=2?x=a"));
        UNIT_ASSERT(SAVE_WITHOUT_PATH("http://ya.ru/?x=a%26b"));
        UNIT_ASSERT(SAVE_WITHOUT_PATH("http://ya.ru/path;sessid=42?x=a;x=b"));
        UNIT_ASSERT(SAVE_WITH_PATH("http://ya.ru/?x=a&"));
        UNIT_ASSERT(SAVE_WITH_PATH("http://ya.ru/?x=a&&"));
        UNIT_ASSERT(SAVE_WITH_PATH("http://ya.ru/?x=a&b"));
        UNIT_ASSERT(SAVE_WITH_PATH("http://ya.ru/?x=a%26b"));
        UNIT_ASSERT(SAVE_WITH_PATH("http://ya.ru/?x=a&x=b"));
        UNIT_ASSERT(SAVE_WITH_PATH("http://ya.ru/?x=b&x=a"));
        UNIT_ASSERT(SAVE_WITH_PATH("http://ya.ru/?x=a&x=a"));
        UNIT_ASSERT(SAVE_WITH_PATH("http://ya.ru/path;sessid=42?x=a"));
        UNIT_ASSERT(SAVE_WITH_PATH("http://ya.ru/path;a=1;b=2?x=a"));
        UNIT_ASSERT(SAVE_WITH_PATH("http://ya.ru/path;a=1;;b=2?x=a"));
        UNIT_ASSERT(SAVE_WITH_PATH("http://ya.ru/?x=a%26b"));
        UNIT_ASSERT(SAVE_WITH_PATH("http://ya.ru/path;sessid=42?x=a;x=b"));
    }
    Y_UNIT_TEST(testRemoveParams)
    {
        UNIT_ASSERT(REMOVE_WITHOUT_PATH("http://ya.ru/?x=a&", "http://ya.ru/?x=a", ""));
        UNIT_ASSERT(REMOVE_WITHOUT_PATH("http://ya.ru/?x=a&&", "http://ya.ru/?x=a", ""));
        UNIT_ASSERT(REMOVE_WITHOUT_PATH("http://ya.ru/?x=a&b", "http://ya.ru/?b", "x"));
        UNIT_ASSERT(REMOVE_WITHOUT_PATH("http://ya.ru/?x=a&b", "http://ya.ru/?x=a", "b"));
        UNIT_ASSERT(REMOVE_WITHOUT_PATH("http://ya.ru/?x=a%26b", "http://ya.ru/?x=a%26b", "b"));
        UNIT_ASSERT(REMOVE_WITHOUT_PATH("http://ya.ru/?x=a&x=b", "http://ya.ru/", "x"));
        UNIT_ASSERT(REMOVE_WITHOUT_PATH("http://ya.ru/?x=b&x=a", "http://ya.ru/", "x"));
        UNIT_ASSERT(REMOVE_WITHOUT_PATH("http://ya.ru/?x=a&x=a", "http://ya.ru/", "x"));
        UNIT_ASSERT(REMOVE_WITHOUT_PATH("http://ya.ru/path;sessid=42?x=a;x=b", "http://ya.ru/path;sessid=42?x=a;x=b", "sessid"));
        UNIT_ASSERT(REMOVE_WITH_PATH("http://ya.ru/path;sessid=42?x=a", "http://ya.ru/path?x=a", "sessid"));
        UNIT_ASSERT(REMOVE_WITH_PATH("http://ya.ru/path;a=1;b=2", "http://ya.ru/path;b=2", "a"));
        UNIT_ASSERT(REMOVE_WITH_PATH("http://ya.ru/path;a=1;b=2", "http://ya.ru/path", "a", "b"));
        UNIT_ASSERT(REMOVE_WITH_PATH("http://ya.ru/path;a=1;b=2?x=1&y=2", "http://ya.ru/path", "a", "b", "x", "y"));
        UNIT_ASSERT(REMOVE_WITH_PATH("http://ya.ru/path;sessid=42?x=a&sessid=42", "http://ya.ru/path?x=a","sessid"));
        UNIT_ASSERT(REMOVE_WITH_PATH("http://ya.ru/path;sessid=42?x=a;x=b", "http://ya.ru/path?x=a;x=b", "sessid"));
    }
    Y_UNIT_TEST(testSetValue)
    {
        UNIT_ASSERT(TestSetValue<false>("http://ya.ru/?x=a", "x", "b", "http://ya.ru/?x=b"));
        UNIT_ASSERT(TestSetValue<false>("http://ya.ru/?xxx=a", "x", "b", "http://ya.ru/?xxx=a&x=b"));
        UNIT_ASSERT(TestSetValue<false>("http://ya.ru/?x=a&b=z", "x", "b", "http://ya.ru/?x=b&b=z"));
        UNIT_ASSERT(TestSetValue<false>("http://ya.ru/?x=a&x=1&b=z", "x", "b", "http://ya.ru/?x=a&x=b&b=z"));
        UNIT_ASSERT(TestSetValue<false>("http://ya.ru/;x=12?x=a", "x", "b", "http://ya.ru/;x=12?x=b"));
        UNIT_ASSERT(TestSetValue<true>("http://ya.ru/;x=12?x=a", "x", "b", "http://ya.ru/;x=b?x=a"));
        UNIT_ASSERT(TestSetValue<true>("http://ya.ru/;c&a=12;b=a?c;x=q", "c", "42", "http://ya.ru/;c&a=12;b=a?c=42;x=q"));
        UNIT_ASSERT(TestSetValue<true>("http://ya.ru/;a=12;b=a?c;x=q&", "c", "42", "http://ya.ru/;a=12;b=a?c;x=q&&c=42"));
    }
    Y_UNIT_TEST(testReal)
    {
        UNIT_ASSERT(SAVE_WITHOUT_PATH("http://ingrossoagroalimentarezs.it/ingrosso-agroalimentare-napoli/index.php/;focus=ITNET_cm4all_com_widgets_Shop_159966&path=show?subAction=showProduct&productId=2393&categoryId="));
        UNIT_ASSERT(SAVE_WITH_PATH(   "http://ingrossoagroalimentarezs.it/ingrosso-agroalimentare-napoli/index.php/;focus=ITNET_cm4all_com_widgets_Shop_159966&path=show?subAction=showProduct&productId=2393&categoryId="));
        UNIT_ASSERT(SPLIT_NO_SORT(    "http://ingrossoagroalimentarezs.it/ingrosso-agroalimentare-napoli/index.php/;focus=ITNET_cm4all_com_widgets_Shop_159966&path=show?subAction=showProduct&productId=2393&categoryId=",
                                      "categoryId=",
                                      "productId=2393",
                                      "subAction=showProduct"));
        UNIT_ASSERT(SPLIT_PATH(       "http://ingrossoagroalimentarezs.it/ingrosso-agroalimentare-napoli/index.php/;focus=ITNET_cm4all_com_widgets_Shop_159966&path=show?subAction=showProduct&productId=2393&categoryId=",
                                      "categoryId=",
                                      "focus=ITNET_cm4all_com_widgets_Shop_159966&path=show", // & is not a separator in path params
                                      "productId=2393",
                                      "subAction=showProduct"));
        UNIT_ASSERT(REMOVE_WITH_PATH( "http://ingrossoagroalimentarezs.it/ingrosso-agroalimentare-napoli/index.php/;focus=ITNET_cm4all_com_widgets_Shop_159966&path=show?subAction=showProduct&productId=2393&categoryId=",
                                      "http://ingrossoagroalimentarezs.it/ingrosso-agroalimentare-napoli/index.php/;focus=ITNET_cm4all_com_widgets_Shop_159966&path=show?subAction=showProduct&categoryId=",
                                      "productId"));

        UNIT_ASSERT(SAVE_WITHOUT_PATH("http://m1.gimpo.go.kr/council/member/login.do;jsessionid=1JQgVzuku1fYIRxN43ewPD8iC4fea2IAh0TgInr1WUn9PCytq1LKg0grOgxQazYH.new-gimpo-was1_servlet_engine2?menu_cd=100062"));
        UNIT_ASSERT(SAVE_WITH_PATH(   "http://m1.gimpo.go.kr/council/member/login.do;jsessionid=1JQgVzuku1fYIRxN43ewPD8iC4fea2IAh0TgInr1WUn9PCytq1LKg0grOgxQazYH.new-gimpo-was1_servlet_engine2?menu_cd=100062"));
        UNIT_ASSERT(SPLIT_NO_SORT(    "http://m1.gimpo.go.kr/council/member/login.do;jsessionid=1JQgVzuku1fYIRxN43ewPD8iC4fea2IAh0TgInr1WUn9PCytq1LKg0grOgxQazYH.new-gimpo-was1_servlet_engine2?menu_cd=100062",
                                      "menu_cd=100062"));
        UNIT_ASSERT(SPLIT_PATH(       "http://m1.gimpo.go.kr/council/member/login.do;jsessionid=1JQgVzuku1fYIRxN43ewPD8iC4fea2IAh0TgInr1WUn9PCytq1LKg0grOgxQazYH.new-gimpo-was1_servlet_engine2?menu_cd=100062",
                                      "jsessionid=1JQgVzuku1fYIRxN43ewPD8iC4fea2IAh0TgInr1WUn9PCytq1LKg0grOgxQazYH.new-gimpo-was1_servlet_engine2",
                                      "menu_cd=100062"));
        UNIT_ASSERT(REMOVE_WITH_PATH( "http://m1.gimpo.go.kr/council/member/login.do;jsessionid=1JQgVzuku1fYIRxN43ewPD8iC4fea2IAh0TgInr1WUn9PCytq1LKg0grOgxQazYH.new-gimpo-was1_servlet_engine2?menu_cd=100062",
                                      "http://m1.gimpo.go.kr/council/member/login.do?menu_cd=100062",
                                      "jsessionid"));

        UNIT_ASSERT(SAVE_WITHOUT_PATH("http://pomogoon.com/detskiy-mir/Igrushki/?filter=21=37,28,32,34;22=47,46&page=8"));
        UNIT_ASSERT(SAVE_WITH_PATH(   "http://pomogoon.com/detskiy-mir/Igrushki/?filter=21=37,28,32,34;22=47,46&page=8"));
        UNIT_ASSERT(SPLIT_NO_SORT(    "http://pomogoon.com/detskiy-mir/Igrushki/?filter=21=37,28,32,34;22=47,46&page=8",
                                      "filter=21=37,28,32,34;22=47,46",
                                      "page=8"));
        UNIT_ASSERT(SPLIT_PATH(       "http://pomogoon.com/detskiy-mir/Igrushki/?filter=21=37,28,32,34;22=47,46&page=8",
                                      "filter=21=37,28,32,34;22=47,46",
                                      "page=8"));
    }
}
