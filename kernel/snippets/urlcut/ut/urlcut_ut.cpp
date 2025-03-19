#include <library/cpp/testing/unittest/registar.h>

#include <kernel/snippets/urlcut/urlcut.h>
#include <kernel/snippets/urlcut/url_wanderer.h>
#include <kernel/qtree/richrequest/richnode.h>
#include <util/charset/wide.h>
#include <util/generic/ptr.h>

class TUrlCutterTest: public TTestBase {
    UNIT_TEST_SUITE(TUrlCutterTest);
    UNIT_TEST(TestHiliteAndCutUrl);
    UNIT_TEST(TestHiliteAndCutUrlMenuHost);
    UNIT_TEST(TestHiliteAndCutUrlMenuPath);
    UNIT_TEST(TestHiliteAndCutMarketUrl);
    UNIT_TEST(TestHiliteAndCutAdsUrl);
    UNIT_TEST_SUITE_END();

public:
    void TestHiliteAndCutUrl();
    void TestHiliteAndCutUrlMenuHost();
    void TestHiliteAndCutUrlMenuPath();
    void TestHiliteAndCutMarketUrl();
    void TestHiliteAndCutAdsUrl();
};

UNIT_TEST_SUITE_REGISTRATION(TUrlCutterTest);

static THolder<NUrlCutter::TRichTreeWanderer> CreateRichTreeWanderer(const TString& query) {
    TCreateTreeOptions opts(LI_DEFAULT_REQUEST_LANGUAGES);
    return MakeHolder<NUrlCutter::TRichTreeWanderer>(CreateRichTree(UTF8ToWide(query), opts));
}

static void AssertHilitedEqual(const NUrlCutter::THilitedString& hilited, const TString& expected) {
    TString actual = WideToUTF8(hilited.Merge(u"[", u"]"));
    UNIT_ASSERT_STRINGS_EQUAL(actual, expected);
}

static void DoHiliteAndCutUrl(const TString& url, const TString& query, const TString& expected, bool cutPrefix = true) {
    auto rTreeWanderer = CreateRichTreeWanderer(query);
    auto hilited = NUrlCutter::HiliteAndCutUrl(url, 50, 40, *rTreeWanderer, LANG_RUS, CODES_UNKNOWN, cutPrefix);
    AssertHilitedEqual(hilited, expected);
}

void TUrlCutterTest::TestHiliteAndCutUrl() {
    // translit
    DoHiliteAndCutUrl("adonay-forum.com/diskoteka_magov_prekrasnaya_muzyika_dlya_prekrasnyih_dush/velikie_kompozitoryi_i_muzyikantyi/msg7887/?PHPSESSID=3fd6cd256602936c3645553304159e87",
                      "в контакте",
                      "adonay-forum.com/diskoteka_magov…muzyika…velikie…");
    DoHiliteAndCutUrl("www.kleo.ru/items/planetarium/stas_pjiekha_v_tikhom_omute.comments.shtml?14",
                      "стас пьеха в тихом скачать",
                      "kleo.ru/items/planetarium/[stas]_pjiekha_[v]_[tikhom]…");
    DoHiliteAndCutUrl("krasview.ru/video/582087-Transformery_Praiym:_Ohotniki_na_chudovisch._Vosstanie_predakonov_Transformers_Prime_Beast_Hunters:_Predacons_Rising_-_Film",
                      "трансформер",
                      "krasview.ru/video…[Transformer]y…[Transformer]s_Prime…");

    // cut prefix
    DoHiliteAndCutUrl("https://www.google.ru/chrome", "google chrome", "[google].ru/[chrome]", true);
    DoHiliteAndCutUrl("https://www.google.ru/chrome", "google chrome", "https://www.[google].ru/[chrome]", false);
    DoHiliteAndCutUrl("http://www.google.ru/chrome", "google chrome", "[google].ru/[chrome]", true);
    DoHiliteAndCutUrl("http://www.google.ru/chrome", "google chrome", "http://www.[google].ru/[chrome]", false);
    DoHiliteAndCutUrl("www.google.ru/chrome", "google chrome", "[google].ru/[chrome]", true);
    DoHiliteAndCutUrl("www.google.ru/chrome", "google chrome", "www.[google].ru/[chrome]", false);
    DoHiliteAndCutUrl("google.ru/chrome", "google chrome", "[google].ru/[chrome]", true);
    DoHiliteAndCutUrl("google.ru/chrome", "google chrome", "[google].ru/[chrome]", false);

    // decode utf-8
    DoHiliteAndCutUrl("https://ru.wikipedia.org/wiki/%D0%9D%D0%B8%D0%B6%D0%BD%D0%B8%D0%B9_%D0%A2%D0%B0%D0%B3%D0%B8%D0%BB",
                      "нижний тагил",
                      "ru.wikipedia.org/wiki/[Нижний]_[Тагил]");

    // decode cp1251
    DoHiliteAndCutUrl("http://ru.wikipedia.org/wiki/%D8%E0%E1%EB%EE%ED:%D5%EE%F0%E2%E0%F2%E8%FF_%ED%E0_%D7%CC_1998",
                      "хорватия wikipedia",
                      "ru.[wikipedia].org/wiki/Шаблон:[Хорватия]_на_ЧМ_1998");

    // decode punycode
    DoHiliteAndCutUrl("www.xn--d1acpjx3f.xn--p1ai/search?text=%D0%BF%D0%BE%D0%B8%D1%81%D0%BA",
                      "яндекс поиск",
                      "[яндекс].рф/search?text=[поиск]");

    // cgi parameters decoding
    DoHiliteAndCutUrl("foo.ru/index.php?a=&ID=21180&pg=79&s=%C4&w=%C4%D0%CE%D2%C8%CA",
                      "дротик",
                      "foo.ru/index.php?a=&ID=21180&pg=79&s=Д&w=[ДРОТИК]");

    // escaped fragment
    DoHiliteAndCutUrl("avantika.by/store/?_escaped_fragment_=/~/product/category%3D2768232%26id%3D12105779",
                      "store category",
                      "avantika.by/[store]/#!/~/product/[category]=2768232…");
    DoHiliteAndCutUrl("avantika.by/store/?_escaped_fragment_=/~/product/category%3D2768232%26id%3D12105779",
                      "category 2768232 id 12105779",
                      "avantika.by/store…[category]=[2768232]&[id]=[12105779]");

    // cutting
    DoHiliteAndCutUrl("example.com/blacksmith?estimation=forefinger",
                      "forefinger",
                      "example.com/blacksmith?estimation=[forefinger]");
    DoHiliteAndCutUrl("example.com/blacksmith?estimation=forefinger",
                      "example",
                      "[example].com/blacksmith…");
    DoHiliteAndCutUrl("example.com/?estimation=forefinger",
                      "forefinger",
                      "example.com/?estimation=[forefinger]");
    DoHiliteAndCutUrl("example.com/?estimation=forefinger",
                      "example",
                      "[example].com/?estimation=forefinger");

    TString longPath1 = "wideexample.com/assortment/blacksmith/chemically/department";
    DoHiliteAndCutUrl(longPath1, "foo", "wideexample.com/assortment/blacksmith/chemically/…");
    DoHiliteAndCutUrl(longPath1, "wideexample", "[wideexample].com/assortment/blacksmith/chemically/…");
    DoHiliteAndCutUrl(longPath1, "assortment", "wideexample.com/[assortment]/blacksmith/chemically…");
    DoHiliteAndCutUrl(longPath1, "department", "wideexample.com/assortment/blacksmith…[department]");

    TString longPath2 = "wideexample.com/assortment/blacksmith/chemically/department?estimation=forefinger";
    DoHiliteAndCutUrl(longPath2, "foo", "wideexample.com/assortment/blacksmith/chemically/…");
    DoHiliteAndCutUrl(longPath2, "wideexample", "[wideexample].com/assortment/blacksmith/chemically/…");
    DoHiliteAndCutUrl(longPath2, "assortment", "wideexample.com/[assortment]/blacksmith/chemically…");
    DoHiliteAndCutUrl(longPath2, "department", "wideexample.com/assortment/blacksmith…[department]…");
    DoHiliteAndCutUrl(longPath2, "estimation", "wideexample.com/assortment/blacksmith/chemically/…");
    DoHiliteAndCutUrl(longPath2, "forefinger", "wideexample.com/assortment/blacksmith/chemically/…");
}


void TUrlCutterTest::TestHiliteAndCutMarketUrl() {
    TString marketUrl = "//market.yandex.ru/search?text="
        "%D0%BB%D0%B8%D0%BD%D0%BE%D0%BB%D0%B5%D1%83%D0%BC%20%D0%BA%D0%BE%D0%BC%D0%BC%D0%B5%D1%80%D1%87%D0%B5%D1%81%D0"
        "%BA%D0%B8%D0%B9%20%D0%BA%D0%BB%D0%B0%D1%81%D1%81%2034%5C%5C%5C%5C%5C%5C%5C/43%20%D1%86%D0%B5%D0%BD%D0%B0&clid=545";
    TString expected = "market.yandex.ru/search?…[линолеум] [коммерческий]…[43]…";

    //const TString query = "линолеум коммерческий класс 34\\\\\\\\\\\\\\/43 цена";
    //QTree from production
    TString qtreeStr = "cHic7VbdaxxVFD_nzux2vE3suGl0O1pcFqWjIlncPBSlRmMfitAaaxW7rtikWdyYpJUEIQTBWaM2rWirPihaQq0EI0jYtgb"
        "TfPn1In3p7IPSP8CHguCbPhXxzJ2PzMdN1qgFESeQvTP3nHN_5_e795zLH-UtGuiQhRwzWQEyYEAe7ob74H7_O5hQgIdSe1I98DQcSlfxFMKHC"
        "GcRziMsI9DzPYKN_caXjA964TRyc8Lp9qI9by_bK_Q715iwlwzMBfG1UHzYA0786tSJm_34Cd_YegXYid3LqKEORsI2DyaFKmDPJ4xQQxVHTiJ"
        "PWGXF4jthvzL59aUSm5ktsenZvjQFBLOtj_AZzK6bijdaCkbzYsQM1njdH9lzNOI6GiqFPy--YuDjWE743xpvmkoVxnCcaWghULrGZwp_OcZc1"
        "l6gQEv0N9ewGsfof42-zNsXHQYx51HYIqHwl4-ZT-GaQWRUTjFB5Zo-RCmFDlN6Ffma1gG1aJVSM7PzxK86TT99LcRG2q7bF-yVgJuTAYvzAcd"
        "zAYcXhRa4rgIYibIitADSIrRKJKISj-jp40R8W3gzQ21MOOOYVlOMPxfTilPKi3a9UWvUIuqkJOrMHPDFCXnJ5Ki7OztkRQLMHAjzfxx5aPrfs"
        "pkt5PtiBLFiJ9WVgBcmeMEIL9cO-ryQsYyPOxw3gybzmwv-U-w0rx0kRsAlZBw8CO8kIWA5VNlQIsxrv83-vsXHgOUkBKW7SI5oYJmEQFr1FHo"
        "6bONYzjrfHPbBp7yqjcE4HMLriSn138P0LvLHkpicYH8HFLseoGhLl__fUU0xdWwIU8c64nVIMHU0xTSB_PF4Qeoshm8isoIUQUX269SkzmKoJ"
        "nUWzavPyGoS46UYCq3xhj1H95F6uDiqEoYWMj6QwEUGZ8ZtGYENsbWQCTcMYiKYDLWLry55rQKJPSVoFW0bbhVBXxU9tGmryLdqr-gWZllOMVM"
        "FyLdoiq6F3lJ6OnhLmDKdiTf1H3nDJgs6_JZ-Qm3SsrAdYHy3cRn5s3E9hwaOHB2qvDRs4F15NNe55l75fHFrIKnvJZO031XUN_H2_3u-oA_zY"
        "Gr1wjWZbPw7RkmFTaQHugMwcIdQg5EazFNDdZIxoXQlnOZ3yKuxNFsHjw4PV0YOVyujgwMvRHJtbXa8o76yjPeKjKN28bTv5NF5aQmIpkZCU2q"
        "XlVBqCwp_KpZaanCod3Q0kpLsEvfqN6qfkOshS-QH9z7tGsQzeJC737PoId-AajjGRu6J-7PJ2RJOB_4R464_vZg4spXEao8kAhwvsTMXSnguG"
        "qPSLzzRBXwDBVMGjjzvBcklglilVb3kFrWoRURRVVdJ0S9Qsyyt3eHeOIP8Se6eaV9QtUxP-PYgK6-RHSo8ZHruEnKKaVJT-IRb0W1cTK3VjQL"
        "Ymn4jwa6HYJ9G_kQMtlIud4SvF0oz1I6DDPQDArQzK8N8K3dmmkLW9a0E-TTTas7Z-ejH7G7jGEuenRdHBg5Xmp6dDz5dPTvCQwb7rNvJXAPJ2"
        "RHfs_iXKt5Id8I_sZuF_VqbOV41s3rWVOntBN7OVS2tpzO3aGi0aaz9rW9_3bX9_b29XTm418nQtSB7z2Jbe-3nm7q2A1irFm3cKVZqZrOm7d-"
        "kYUbZ19NL6_wBuNGqjQ,,";
    TBinaryRichTree qtreeBlob = DecodeRichTreeBase64(qtreeStr);
    const auto qTree = DeserializeRichTree(qtreeBlob);

    NUrlCutter::TRichTreeWanderer rTreeWanderer(qTree);
    auto hilited = NUrlCutter::HiliteAndCutUrl(marketUrl, 50, 40, rTreeWanderer, LANG_RUS, CODES_UNKNOWN, true);
    AssertHilitedEqual(hilited, expected);
}

void TUrlCutterTest::TestHiliteAndCutAdsUrl() {
    TString adsUrl = "yandex.ru/search/ads?mw=1&lr=43&text=%D0%9E%D1%82%D0%BC%D0%B5%D1%82%D1%8C%D1%82%D0%B5%2C+%D0"
        "%BA%D0%B0%D0%BA%D0%BE%D0%B5+%D0%B7%D0%BD%D0%B0%D1%87%D0%B5%D0%BD%D0%B8%D0%B5+%D0%BF%D1%80%D0%B8%D0%B4%D0%B0"
        "%D1%8E%D1%82+%D1%81%D1%83%D1%89%D0%B5%D1%81%D1%82%D0%B2%D0%B8%D1%82%D0%B5%D0%BB%D1%8C%D0%BD%D1%8B%D0%BC+%D1"
        "%81%D1%83%D1%84%D1%84%D0%B8%D0%BA%D1%81%D1%8B+-%D1%87%D0%B8%D0%BA-/-%D1%89%D0%B8%D0%BA-:+%D1%81%D0%BE%D0%B1"
        "%D0%B8%D1%80%D0%B0%D1%82%D0%B5%D0%BB%D1%8C%D0%BD%D0%BE%D0%B5+%D0%B7%D0%BD%D0%B0%D1%87%D0%B5%D0%BD%D0%B8%D0"
        "%B5+%D0%BD%D0%B0%D0%B7%D0%B2%D0%B0%D0%BD%D0%B8%D0%B5+%D0%BB%D0%B8%D1%86%D0%B0+%D0%BF%D0%BE+%D0%BF%D1%80%D0"
        "%B8%D0%BD%D0%B0%D0%B4%D0%BB%D0%B5%D0%B6%D0%BD%D0%BE%D1%81%D1%82%D0%B8+%D0%BA+%D1%82%D0%BE%D0%B9+%D0%B8%D0%BB"
        "%D0%B8+%D0%B8%D0%BD%D0%BE%D0%B9+%D0%BD%D0%B0%D1%80%D0%BE%D0%B4%D0%BD%D0%BE%D1%81%D1%82%D0%B8+%D0%B8%D0%BB%D0"
        "%B8+%D0%BC%D0%B5%D1%81%D1%82%D1%83+%D0%B6%D0%B8%D1%82%D0%B5%D0%BB%D1%8C%D1%81%D1%82%D0%B2%D0%B0+%D0%BD%D0%B0"
        "%D0%B7%D0%B2%D0%B0%D0%BD%D0%B8%D0%B5+%D0%BB%D0%B8%D1%86%D0%B0+%D0%BF%D0%BE+%D0%B5%D0%B3%D0%BE+%D0%BF%D1%80"
        "%D0%B8%D0%BD%D0%B0%D0%B4%D0%BB%D0%B5%D0%B6%D0%BD%D0%BE%D1%81%D1%82%D0%B8+%D0%BA+%D0%BF%D0%BE%D0%BB%D0%B8%D1"
        "%82%D0%B8%D1%87%D0%B5%D1%81%D0%BA%D0%BE%D0%BC%D1%83+%D0%B8%D0%BB%D0%B8+%D0%BD%D0%B0%D1%83%D1%87%D0%BD%D0%BE"
        "%D0%BC%D1%83+%D0%BD%D0%B0%D0%BF%D1%80%D0%B0%D0%B2%D0%BB%D0%B5%D0%BD%D0%B8%D1%8E+%D0%BD%D0%B0%D0%B7%D0%B2%D0"
        "%B0%D0%BD%D0%B8%D0%B5+%D0%BB%D0%B8%D1%86%D0%B0+%D0%BF%D0%BE+%D1%85%D0%B0%D1%80%D0%B0%D0%BA%D1%82%D0%B5%D1%80"
        "%D0%BD%D1%8B%D0%BC+%D0%B4%D0%BB%D1%8F+%D0%BD%D0%B5%D0%B3%D0%BE+%D0%B4%D0%B5%D1%8F%D1%82%D0%B5%D0%BB%D1%8C%D0"
        "%BD%D0%BE%D1%81%D1%82%D0%B8+%D0%B8%D0%BB%D0%B8+%D0%BF%D1%80%D0%BE%D1%84%D0%B5%D1%81%D1%81%D0%B8%D0%B8";
    TString expected = "yandex.ru/search/ads?…[чик]-/-[щик]-: [собирательное]…[к]…";
    //QTree from production
    TString qtreeStr = "cHic7Vx_bF1VHb_n3tu317PS3rUrds--USuaZllZg4TMuVazkLAQomMSrE8MY1FSIogukExItGs7ae1YlU2cQog6xroJr"
        "92Pjreur-tw6FgqeW9KCMSYEBQEJEEIRI0yv-fcc88959xz7339tWZL98dy372f7_d8f53v-X7PPbf4BlyRNByjzmgwm8wWo9pIGY3GCuNqY"
        "41332gyWowvlK0v22C0G5sSHejHyPgFMn6NjMPIyCMD_p1BRgF9I_XcYnwXY5cEMsLOKYwXuwpjhZFiV3EHXI2kzJUNfICkMICx3iADdLzwR"
        "z5AgFgZsMVYjdadxUnkGKkAttFoAlYtaMNeE8Q2OtCWJhwA1aEmBFyMjcbmBDAxmmo60Fa85d1yAfpgYZjA66iYADX7_pCxh47mf7u5wrFTC"
        "UDlCoebrM2VwCBZ7C0MF44UxuF30jFTZnEArirgKlEYEe8XRugVgqvj_GqMXhlA1c3pd3KKnPc0mpbhCO127ymVAsM9u9jNONqAPMFphjmfH"
        "L_yZR2j1AZQd7GxLeDZFZSnuIOP00uvXeQOib4Gri24HqAcKuA6QWx80dkQgw1tkGhMsiLVMyC9O_q47vmUtJB9SO72z1AjnS8DHqQcAxFQQ"
        "vy4nLoYJ98XBhuzEpDJYt8c-JuODDNzluaKfxdkykt6Ux0K-VAdxmfF2-51_0y8zKRAPGosd8QOYyu6306iJO5EBuTe1G4Lb1LyOC6MgoKjo"
        "MxICjU0QhJnKbxMk8J71noZXCAL5m573T6T5m4BBVm7Z62btNEmE5L2p7DwlOZgtNrciHoyfsYG8bc0KjA1qychqx9CAug4pH7Gqz9jDWZPZ"
        "ax92VPMxcOCu8UQije03uU103F5wC0HLc3y-gyJPMifEJVEVOIc1BCxvP6rz_SXV4VYt7wOmWx5VbDgKGAlLq8vIhxAUSMTT1i9p85mzMFsx"
        "tyX9ZzCTDmmZGfJUGwG-nFsRLjBXVd2MIeF8J4KRxpaTVq1NGWD5KzJxbhDcVZl4WSxE8iPAqudxS7JVYs0rnqkUOG5SiHVOepFtw5SkOAmY"
        "CO66Xw5rvYxhcOa6qbsV0NHxk-Dahao5i1ZCwv0fC3Q9qwuwAPUi-ZF40VaZEOo5gJlqDvdxetuWlTQa-53NIfljV8eu_cujcLA5biN2dKPH"
        "9luajFXczEUczShfxorSbKEdP5wAvciJZ8vBxt1g74jxFaQSHMkegonYAHKgz_GpPxepcnvf_rI9vJ7DCtdvv-PTfN9DCXkfxhGzP8_QziCh"
        "ldZ_qK9fyhjHsiCFVFoXpT95s7Fful5TbSPqF_eMqMkA4bHuWSoM2PtH8pl7APZ3AT9_9TmJqcq1Vg4CX17uHYjSgg1OEtSKT2e1fk0dClt-"
        "OLh9QVqeM7XklKKsa-NsfVYCZPiNRPfrRaklGlPsQdm1yhc96fMhuboivTwB1_nFalKrQv8N5BbkapYCHVgJYb6QwhXyiihHp0g9SiE9r4hp"
        "p-c7KPqTN-H7qQYp6WR6EWTe9FNqjVeJakKrWlVVCufM3G7YmWyYQIMUkYjWJeZ1tKY9nfnNnimZSQ6g77tGpQhwIxAJpqxC2H2jJvP7n104"
        "nlNLT8V2_kLaowVqe0-wWXgOUDtAZcLEE3_Jxn1Jwh_UTEqaha2PpGurf0w-1GVZ0_UHDSlte4zQIhSqLmRSIDA-MyEyzBq1kpl3G9sQhEyr"
        "ZqSTKuCMpV5Mq3SyLRqWjLN2E7mzOx0zMJfCUyIPjYhjIjpMPno6wl_PvSFzYeCyeZDH5sPhE6cENvJhOhjEwKJ-UTX387NnEAQ8Nu0cly4e"
        "YnYpOu7iCfdwwh_SZVpDSndShdqTUSEr9EItSZWqJdtvE2tNqFMAVccgmW6k7TsYsEmVZqXaeTdsesVXmpG8NFNhjMWnQwRVKAh4S9OkKtxB"
        "D5kd-5lM4yIVn7Ir_zsQSj3MvY-UvRd6VyWWu4Wfdrx1KKsHorEy4PYOS72prX_Nzv7Bwu7iBfzLuKghb8VcBbZUXiGbNyV6KzHtvq-Umh1v"
        "sp6vlKw4KvHtgZcpYAuFVcF1YpzVdbCX1NclYTEkiv-sDAsucjWuOiJNs9FnEbnmtOuazgGXPJEm-iSBsyfhbza_gFHjKu-gmboySxriJDUE"
        "NXE-EEtHsaD_uCF_AN6AZ6_EINLDptA-MuKw2yykkjOMnUNKy8iKV73Yus26if6mLSlCfGN1hWY3vffZT2o9DH1HBBXUO2ycI9aJ9SzvS0Sw"
        "UfpynaMrLu0yc9JulXqSuTdPLFHMtJF53m3j4ykI9X0binhX4MjCfQNMmntI8iKO4TQOgmxfTDrxvf-LA8zt7gdEVbvnLdOsx1OQ1qTB6Jyh"
        "uSVY8GS0oTeRDS9rqh8j7_tMXV9ib3uq9S8Jm1J3qsQQyqNTdYYawMqxR7HhdPEEk1j1UVelkrC61qr3VV-Y0UpdAHyisMaK4oALXZXeVqQS"
        "Hi1ErNHfrXXK60fbm11VGhnjtOqDaUShTEoE5_hScDdibfYVQUZtXCIIeiRE4rPe7_IVoxPfW5vE3vvwO-7HMh-cSdF2Rwl3sdOImWf20VrN"
        "vcJCc48rxo9yRNiXcf4kMrzkcJo8RFvbAhpj6uqdd5rzmZtrDFWAXtvE2RLyjYqzRbKyIG3cEiQUXrzINbiXC7XZ5KGxX46qrfb7Et2iOtia"
        "eymVNU-j3O7GLIs8CZHtLpvW-qXWA9gZxGXtRrGqSB2PLdXthN9Gmed-bIIi2p6XQHWufDR4s5wb_6KWrhcAnOZ2dOOjStPg2iLTkcjarUQL"
        "-isVrpvwjOWJhcxflYgJ-qiOypPYvB80OpRnvclY9E0hZlSmo66uVNqrPjRMfOIiPTB3MZAjOeFaygF8JYh5K6xxS5WLhj0_NPB7MmMtZ_89"
        "yT570D2JKuJ_Dffao8VVojr90CO8z0Q3c5HF89z_r6H1y80MIEHhALG6xlJCVMmlTBnUbCEgRrxhFL66kqYx6t5CeNS6Gqwb7oVjAuACubxa"
        "q-CsTroSwr3CTOutRF1B19SMEh8S_lvE98aaClzdBfteGxL-ctB0-8pGZGuKNvp9ZQMA0oBqVihn0aYP_UrM23UkL25ibPkKN2EdJQu7sBDV"
        "HDEH3goaROM9sa-FnG2_4eJ71Nsv5Ru33QCg6MhHRXW-OHMO93cEVoOOqf83W2ltHhwEOEpemgl1iJDeqftSAef357pkpq3R0x8W-D87Jj7o"
        "r3YLSmkOz77gnB8llPpouQ5xI7PchT56GGtGBnfw_wp23IQDnZkA3stpaX4uL0W_0guF6wEq71r4nsDM-6YfyyBHVMYjp1xf31zqT_hNAx0p"
        "nzHm3AaPBgVWKrzTQMM2XLrR1r4_Oy_LWwvC-tCjFkXtpcXtpenPbcuge3lw5qDFu4pzthF-Ylyf1GmFLp4m2QHLVwEibZycT-QLLn0SUisd"
        "SJMTifkeSdjP5Qdn8igwQx6UjpyWsNq0TH_eCI7TkwPQtPXxEcCB3ql44z8-LFFT0TkWbT1QDtFfnJ7u0Kc4kJYcyCEy5WOqom7hXcCC-8EZ"
        "vGdwN8suFKPopD8QbMsOIO8zy5uo99UjSnFre4oyltn_JMo4Wy0H7i42SKCCmwA7MVAegPhOi0-Jx8mtvfTc8T7yZESN3MED7HPzqcNMzkVX"
        "KPviKk_r8YRhimhvrikGrAuW3PGhXS83WCXvCZSdXXtwV5TKmxFYl18Pmj5ha2IBWWBlfohugIKWeKeNnGlD1WPwItR63304m-qXtgT6yGxq"
        "W5eTnPW1DmLUtU0vn27KdIHUu5pC29VguByujCQJYJ8-HbCPYhT3CmFwmJNKAzv5aEQwkIXEHk3YYVQQFgAWzEsXkJYiw1-keG1PBFtidTy1"
        "Gye35bnKr1ixZ0LR6AWetSFHnWhR51-j_paWTDLF7fTze3hwijZcoOr4Pd4uiz_cHYJz_J6FrqA_LP7HV4IBagObMX4_NDCemz0120NzmK39"
        "A3SBtbxlHNZamkAN7PaYO4-_J7Nv9RRNu36JSDRrH9xd1WI38dK-AZMW51Dg1sciK3O99h-dU4poqpzCoCY3WOL05VU5_RJ-IRdziHxy8Sob"
        "pnIazaWtMtEub9M5MO3lgreMpEP2Vwiy0R-draXrBJ3dmrmcntp9oSI3l76r4kfUJxXB54fIX_Oyf_KQ7OzVKGLzXbPmaE8dM59z91UCqUhA"
        "dwu5twWHIoN2UvqQ2EkC-9JZ61N_6eJ71GUcf80yXixh7443AascinD16tco9fonjJPMQ21Ln7ed-NHgwaNgZ0YOiuwBhbyCdNPURBMM-KUw"
        "kU6BuMHzvT6AzWAGp3k-fOW09lp15kNdlNZy4W9Q6pJE35ZxH7Kr6nTz68uC3cuuTskO2SGULKzM1lLckbqUYQ3YjfovAxlNa9qjvnOWPo8l"
        "OB1SehzNAeRp-RvnhEK8TPRj2PyJOxDUTaX7WSlU91kZJ4SJN6D8E2KxGaz9EGrrp-TJAa8TuDPuq8amtfo5E1heBAr7uVOXRP59SMEHSCRs"
        "fpjSZSqSS6rfX5ysi1tGJ2fbzCaybgewuaI8pf2tqb_9_M6GWELiG1vFzU8Ek6CI566_tXW9J3bV8iICqeCIazah579oDX99F1VbRJiqbOUI"
        "yiP2w8ukxH1Tj0f5cD1r2rkWO4s54i3bsq1pn_z0fdlRIPTwBBLaptuHWhN_2XwOzLiSudKhki4kg7eeUcYYlktfumTa9OHr7pVRqxwVoiI1"
        "vQ179fLiJXOSq7t8PW3t6XrxutlbVuclhge1zrXxmi72lnNtd357H0abducthhtfUSYttc518VIut5ZzxBm7e13vN6Wvrd9tYy4wbkhRpcbn"
        "RtjPLfB2ShH4Vrj8zJio4CYnLyxNb319ftVxM0Msah24NkP1qbveXPz6hAEdu2x9O52GXEzR1iuLviK6-RRbnFuYYgy0KWqLf37b39XRrQ77"
        "aE2hXn-f2QL-Yw,";
    TBinaryRichTree qtreeBlob = DecodeRichTreeBase64(qtreeStr);
    const auto qTree = DeserializeRichTree(qtreeBlob);

    NUrlCutter::TRichTreeWanderer rTreeWanderer(qTree);
    auto hilited = NUrlCutter::HiliteAndCutUrl(adsUrl, 50, 40, rTreeWanderer, LANG_RUS, CODES_UNKNOWN, true);
    AssertHilitedEqual(hilited, expected);
}

static void DoHiliteAndCutUrlMenuHost(const TString& host, const TString& query, const TString& expected) {
    auto rTreeWanderer = CreateRichTreeWanderer(query);
    auto hilited = NUrlCutter::HiliteAndCutUrlMenuHost(host, 30, *rTreeWanderer, LANG_RUS);
    AssertHilitedEqual(hilited, expected);
}

void TUrlCutterTest::TestHiliteAndCutUrlMenuHost() {
    DoHiliteAndCutUrlMenuHost("www.yandex.ru", "", "yandex.ru");
    DoHiliteAndCutUrlMenuHost("yandex.ru", "", "yandex.ru");
    DoHiliteAndCutUrlMenuHost("yandex.ru", "yandex", "[yandex].ru");
    DoHiliteAndCutUrlMenuHost("www1.yandex.ru", "yandex", "www1.[yandex].ru");
    DoHiliteAndCutUrlMenuHost("www1.yandex.ru", "яндекс", "www1.[yandex].ru");
    DoHiliteAndCutUrlMenuHost("www.яндекс.рф", "яндекс", "[яндекс].рф");
    DoHiliteAndCutUrlMenuHost("www.xn--d1acpjx3f.xn--p1ai", "яндекс", "[яндекс].рф");
    DoHiliteAndCutUrlMenuHost("yandexteam.ru", "yandex", "[yandex]team.ru");
    DoHiliteAndCutUrlMenuHost("yandexteam.ru", "yandex team", "[yandexteam].ru");
    DoHiliteAndCutUrlMenuHost("yandex-team.ru", "yandex team", "[yandex]-[team].ru");
    DoHiliteAndCutUrlMenuHost("veryveryveryveryveryverylongsubdomain.yandex.ru", "yandex", "….[yandex].ru");
    DoHiliteAndCutUrlMenuHost("onemoreveryveryveryverylongsubdomain.yandex.ru", "one yandex", "….[yandex].ru");
    DoHiliteAndCutUrlMenuHost("one.more.veryveryveryverylongsubdomain.yandex.ru", "one", "….yandex.ru");
}

static void DoHiliteAndCutUrlMenuPath(const TString& urlPath, const TString& query, const TString& expected) {
    auto rTreeWanderer = CreateRichTreeWanderer(query);
    auto hilited = NUrlCutter::HiliteAndCutUrlMenuPath(urlPath, 40, *rTreeWanderer, LANG_RUS);
    AssertHilitedEqual(hilited, expected);
}

void TUrlCutterTest::TestHiliteAndCutUrlMenuPath() {
    DoHiliteAndCutUrlMenuPath("/aaa/bbb/ccc/ddd", "bbb", "/aaa/[bbb]/ccc/ddd");
    DoHiliteAndCutUrlMenuPath("aaa/bbb/ccc/ddd", "ccc", "aaa/bbb/[ccc]/ddd");
    DoHiliteAndCutUrlMenuPath("foo.html", "foo", "[foo].html");
    DoHiliteAndCutUrlMenuPath("/foo/foo.html", "foo", "/[foo]/[foo].html");
    DoHiliteAndCutUrlMenuPath("/index/index.html", "index", "/index/index.html"); // index is a stopword
    DoHiliteAndCutUrlMenuPath("", "foo", "");
    DoHiliteAndCutUrlMenuPath("/", "foo", "…");
    DoHiliteAndCutUrlMenuPath(TString(40, 'z'), "foo", TString(40, 'z'));
    DoHiliteAndCutUrlMenuPath(TString(41, 'z'), "foo", "…");
    DoHiliteAndCutUrlMenuPath("/a", "foo", "/a");
    DoHiliteAndCutUrlMenuPath("b", "foo", "b");
    DoHiliteAndCutUrlMenuPath("/aaa/bbb?ccc=111&eee=222", "foo", "/aaa/bbb?ccc=111&eee=222");
    DoHiliteAndCutUrlMenuPath("aaa/bbb?ccc=111&eee=222", "foo", "aaa/bbb?ccc=111&eee=222");
    DoHiliteAndCutUrlMenuPath("/aaa/bbb?ccc=111&eee=222", "ccc", "/aaa/bbb?[ccc]=111&eee=222");
    DoHiliteAndCutUrlMenuPath("/aaa/bbb?ccc=111&eee=222", "eee 222", "/aaa/bbb?ccc=111&[eee]=[222]");
    DoHiliteAndCutUrlMenuPath("/aaa/?ccc=111&eee=222", "foo", "/aaa/?ccc=111&eee=222");
    DoHiliteAndCutUrlMenuPath("/aaa/?ccc=111&eee=222", "ccc", "/aaa/?[ccc]=111&eee=222");
    DoHiliteAndCutUrlMenuPath("12202970/?page=30", "foo", "12202970/?page=30");

    TString longPath1 = "/assortment/blacksmith/chemically/department";
    DoHiliteAndCutUrlMenuPath(longPath1, "foo", "…/blacksmith/chemically/department");
    DoHiliteAndCutUrlMenuPath(longPath1, "assortment", "…/blacksmith/chemically/department");
    DoHiliteAndCutUrlMenuPath(longPath1, "assortment::12345678", "/[assortment]/blacksmith/chemically…");

    TString longPath2 = "/assortment/blacksmith/chemically/department?estimation=forefinger";
    DoHiliteAndCutUrlMenuPath(longPath2, "foo", "…/blacksmith/chemically/department…");
    DoHiliteAndCutUrlMenuPath(longPath2, "chemically", "…/blacksmith/[chemically]/department…");
    DoHiliteAndCutUrlMenuPath(longPath2, "chemically estimation", "…/[chemically]/department?[estimation]…");
}
