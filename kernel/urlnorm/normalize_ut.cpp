#include "normalize.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/maybe.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace {

    void Equal(const char* in, const char* out, const TMaybe<bool>& okValue = TMaybe<bool>()) {
        bool ok = false;
        TString normalizedUrl = DoWeakUrlNormalization(in, &ok);
        UNIT_ASSERT_VALUES_EQUAL(normalizedUrl, out);
        if (okValue.Defined()) {
            UNIT_ASSERT_VALUES_EQUAL(ok, *okValue.Get());
        }
    }

    void Unequal(const char* in, const char* out, const TMaybe<bool>& okValue = TMaybe<bool>()) {
        bool ok = false;
        TString normalizedUrl = DoWeakUrlNormalization(in, &ok);
        UNIT_ASSERT_VALUES_UNEQUAL(normalizedUrl, out);
        if (okValue.Defined()) {
            UNIT_ASSERT_VALUES_EQUAL(ok, *okValue.Get());
        }
    }

    Y_UNIT_TEST_SUITE(TDoWeakUrlNormalizationTest) {

        const TMaybe<bool> EXPECT_SUCCESS(true);
        const TMaybe<bool> EXPECT_FAILURE(false);

        Y_UNIT_TEST(TestOkFlag) {
            Equal("htax://ya.ru", "htax://ya.ru", EXPECT_FAILURE);
            Unequal("sad", "http://g.co", EXPECT_SUCCESS);
            Equal("http://ya.ru/", "http://ya.ru/", EXPECT_SUCCESS);
            Equal("http:g", "http:g", EXPECT_FAILURE);
            Equal("g:h", "g:h", EXPECT_FAILURE);
        }

        // Slash will be added, if path is empty
        Y_UNIT_TEST(TestSlash) {
            Equal("http://ya.ru", "http://ya.ru/");
            Equal("http://www.ya.ru/", "http://www.ya.ru/");
            Equal("http://example.com?k1=v1&k2=v2", "http://example.com/?k1=v1&k2=v2");
        }

        // Scheme wouldn't be added
        Y_UNIT_TEST(TestScheme) {
            Unequal("ya.ru", "http://ya.ru/");
            Equal("http://ya.ru", "http://ya.ru/");
            Equal("ftp://ya.ru", "ftp://ya.ru/");
            Equal("https://ya.ru", "https://ya.ru/");
        }

        // Host will be lowercased
        Y_UNIT_TEST(TestHostLowercasing) {
            Equal("http://ya.ru", "http://ya.ru/");
            Equal("http://YA.ru", "http://ya.ru/");
            Equal("http://YaNdEx.CoM", "http://yandex.com/");
            Equal("http://Ya.Ru/HELLO", "http://ya.ru/HELLO");
        }

        // Cur fragment ('#...')
        Y_UNIT_TEST(TestCutFragment) {
            Equal("http://ya.ru/asdf#ddd", "http://ya.ru/asdf");
            Equal("http://ya.ru#aa", "http://ya.ru/");
            Equal("http://ya.ru/#aa", "http://ya.ru/");
            Equal("http://ya.ru/aa#", "http://ya.ru/aa");
        }

        // Host will be punycoded
        Y_UNIT_TEST(TestPunycode) {
            Equal("http://президент.рф", "http://xn--d1abbgf6aiiy.xn--p1ai/");
            Equal("http://президент.рф", "http://xn--d1abbgf6aiiy.xn--p1ai/");
            Equal("http://президент.рф/junk?junk", "http://xn--d1abbgf6aiiy.xn--p1ai/junk?junk");
            Equal("http://хостинг-беларуси.рф", "http://xn----8sbcgmofwni4adjeyt.xn--p1ai/");
            Equal("http://хостинг-беларуси.рф/a?b=b&c=c", "http://xn----8sbcgmofwni4adjeyt.xn--p1ai/a?b=b&c=c");
        }

        // Path will be percent-encoded
        Y_UNIT_TEST(TestPercentEncoding) {
            Equal("http://ya.ru/%D0%A2%D0%B5%D0%BE%D1%80%D0%B8%D1%8F", "http://ya.ru/%D0%A2%D0%B5%D0%BE%D1%80%D0%B8%D1%8F");
            Equal("http://ru.wikipedia.org/wiki/Hello how are you", "http://ru.wikipedia.org/wiki/Hello%20how%20are%20you");
            Equal("http://www.ru/Теория_вероятностей", "http://www.ru/%D0%A2%D0%B5%D0%BE%D1%80%D0%B8%D1%8F_%D0%B2%D0%B5%D1%80%D0%BE%D1%8F%D1%82%D0%BD%D0%BE%D1%81%D1%82%D0%B5%D0%B9");
            Equal("http://www.ru/Теория вероятностей", "http://www.ru/%D0%A2%D0%B5%D0%BE%D1%80%D0%B8%D1%8F%20%D0%B2%D0%B5%D1%80%D0%BE%D1%8F%D1%82%D0%BD%D0%BE%D1%81%D1%82%D0%B5%D0%B9");
            Equal("http://xn--d1abbgf6aiiy.xn--p1ai/Теория", "http://xn--d1abbgf6aiiy.xn--p1ai/%D0%A2%D0%B5%D0%BE%D1%80%D0%B8%D1%8F");
        }

        // Look at specification
        Y_UNIT_TEST(TestEscapeFragment) {
            Equal("http://www.ru/aaa#!ooo", "http://www.ru/aaa?_escaped_fragment_=ooo");
            Equal("http://www.example.com?myquery#!k1=v1&k2=v2", "http://www.example.com/?myquery&_escaped_fragment_=k1=v1%26k2=v2");
            Equal("http://kinochi.org/#!/view/695", "http://kinochi.org/?_escaped_fragment_=/view/695");
            Equal("https://groups.google.com/forum/#!/fido7.ru.anime", "https://groups.google.com/forum/?_escaped_fragment_=/fido7.ru.anime");
            // There is only one '#' symbol in url (see specification)
            // Doubtful tests
            /*
            Equal("http://prusskiy.blogspot.com/#!#!#!#!#!#!#!#!#!#!#!", "http://prusskiy.blogspot.com/?_escaped_fragment_=&_escaped_fragment_=&_escaped_fragment_=&_escaped_fragment_=&_escaped_fragment_=&_escaped_fragment_=&_escaped_fragment_=&_escaped_fragment_=&_escaped_fragment_=&_escaped_fragment_=&_escaped_fragment_=");
            Equal("http://ya.ru/aa#!bb#!cc", "http://ya.ru/aa?_escaped_fragment_=bb&_escaped_fragment_=cc");
            Equal("http://ya.ru/aa?bb#!cc#!dd", "http://ya.ru/aa?bb&_escaped_fragment_=cc&_escaped_fragment_=dd");
            */
        }

        // IPv6 works
        Y_UNIT_TEST(TestIPv6) {
            Equal("http://[2a02:6b8::408:426c:8fff:fe26:5a1e]:8080", "http://[2a02:6b8::408:426c:8fff:fe26:5a1e]:8080/");
            Equal("http://[1080:0:0:0:8:800:200C:417A]/index.html", "http://[1080:0:0:0:8:800:200c:417a]/index.html");
        }

        // Test HTTP(S) ports
        Y_UNIT_TEST(TestDefaultPorts) {
            Equal("http://ya.ru:80/", "http://ya.ru/");
            Equal("https://ya.ru:443/", "https://ya.ru/");
        }

        // Some other tests
        Y_UNIT_TEST(Misc) {
            // Without changes
            Equal("http://ya.ru/a&b&c", "http://ya.ru/a&b&c");
            Equal("http://ya.ru/a?b&c", "http://ya.ru/a?b&c");
            // Port, percent-encoding for spaces, punycode
            Equal("http://президент.рф:1010/hello hello", "http://xn--d1abbgf6aiiy.xn--p1ai:1010/hello%20hello");
            // Lowercase, punycode, percent-encoding
            Equal("http://ПРезидент.рф/Привет bonjour hello", "http://xn--d1abbgf6aiiy.xn--p1ai/%D0%9F%D1%80%D0%B8%D0%B2%D0%B5%D1%82%20bonjour%20hello");
            // Empty path
            Equal("http://www.ru:1010", "http://www.ru:1010/");
            // 'index.html' wouldn't be deleted
            Equal("http://www.ru/index.html", "http://www.ru/index.html");
            // Punycode, percent-encoding
            Equal("http://я.рф/%D0%A2 z", "http://xn--41a.xn--p1ai/%D0%A2%20z");
            // Query args wouldn't be sorted
            Equal("http://www.ru/aaa?bb=b&aa=a", "http://www.ru/aaa?bb=b&aa=a");
            // Trail slash in path
            Equal("http://ya.ru/path", "http://ya.ru/path");
            Equal("http://ya.ru/path/", "http://ya.ru/path/");
            // Path wouldn't be lowercased
            Equal("http://ya.ru/BlaBla?BlaBla=Ooo", "http://ya.ru/BlaBla?BlaBla=Ooo");
            // Duplicate args wouldn't be deleted
            Equal("http://ya.ru/oo?aa=a&aa=a&aa=b", "http://ya.ru/oo?aa=a&aa=a&aa=b");
            // 'www' wouldn't be deleted
            Equal("http://www.ya.ru", "http://www.ya.ru/");
            // '#' will be deleted
            Equal("http://ya.ru/a?b=b#c", "http://ya.ru/a?b=b");
            // Slash in host, '#!', '#'
            Equal("http://ya.ru?aa&bb#!cc#dd", "http://ya.ru/?aa&bb&_escaped_fragment_=cc%23dd");
        }
    }

    Y_UNIT_TEST_SUITE(TUrlToRobotUrlTest) {

        void checkUrlToRobotUrl(const TString& rawUrl, bool reqGlobal, bool valid,
                                       const TString& goodResult = "")
        {
            bool state;
            TString result;
            UNIT_ASSERT_VALUES_EQUAL(state = UrlToRobotUrl_DONT_USE_THIS_FUNCTION(rawUrl.data(), result, reqGlobal), valid);
            if (valid) {
                UNIT_ASSERT_VALUES_EQUAL(result, goodResult);
            }
        }

        Y_UNIT_TEST(UrlToRobotUrl_DONT_USE_THIS_FUNCTION) {
            checkUrlToRobotUrl("google.com", false, true, "google.com/");
            checkUrlToRobotUrl("google.com/1", false, true, "google.com/1");
            checkUrlToRobotUrl("google.com/1/", false, true, "google.com/1/");

            checkUrlToRobotUrl("google.com:77", false, true, "google.com:77/");
            checkUrlToRobotUrl("google.com:77/1", false, true, "google.com:77/1");
            checkUrlToRobotUrl("google.com:77/1/", false, true, "google.com:77/1/");

            checkUrlToRobotUrl("google.com:80", false, true, "google.com/");
            checkUrlToRobotUrl("google.com:80/1", false, true, "google.com/1");
            checkUrlToRobotUrl("google.com:80/1/", false, true, "google.com/1/");

            checkUrlToRobotUrl("http://google.com", false, true, "google.com/");
            checkUrlToRobotUrl("http://google.com/1", false, true, "google.com/1");
            checkUrlToRobotUrl("http://google.com/1/", false, true, "google.com/1/");

            checkUrlToRobotUrl("http://google.com/1/", true, true, "google.com/1/");
            checkUrlToRobotUrl("google.com/1/", true, false, "");

            checkUrlToRobotUrl("https://google.com", false, true, "https://google.com/");
            checkUrlToRobotUrl("https://google.com/1", false, true, "https://google.com/1");
            checkUrlToRobotUrl("https://google.com/1/", false, true, "https://google.com/1/");

            checkUrlToRobotUrl("https://google.com", true, true, "https://google.com/");
            checkUrlToRobotUrl("https://google.com/1", true, true, "https://google.com/1");
            checkUrlToRobotUrl("https://google.com/1/", true, true, "https://google.com/1/");

            // check lowercasing host
            checkUrlToRobotUrl("GOOGLE.com:77/1", false, true, "google.com:77/1");
            checkUrlToRobotUrl("google.COM:77/1/", false, true, "google.com:77/1/");
            checkUrlToRobotUrl("google.COM:77/ABC/", false, true, "google.com:77/ABC/");

            // check uppercasing escaped characters
            checkUrlToRobotUrl(
                    "http://yandex.ru/yandsearch?text=%d0%BA%d0%B0%d0%B0&clid=139040&yasoft=chrome&lr=213",
                    true,
                    true,
                    "yandex.ru/yandsearch?text=%D0%BA%D0%B0%D0%B0&clid=139040&yasoft=chrome&lr=213");

            // punycode
            const unsigned char prezidentRfUtf8[] = { 0xD0, 0xBF, 0xD1, 0x80, 0xD0, 0xB5, 0xD0,
                                             0xB7, 0xD0, 0xB8, 0xD0, 0xB4, 0xD0, 0xB5,
                                             0xD0, 0xBD, 0xD1, 0x82, 0x2E, 0xD1, 0x80,
                                             0xD1, 0x84, '\0' };
            checkUrlToRobotUrl(reinterpret_cast<const char*>(prezidentRfUtf8), false, true, "xn--d1abbgf6aiiy.xn--p1ai/");
        }

    }

    Y_UNIT_TEST_SUITE(TUrlNormTest) {
        void Check(const TStringBuf& rawUrl, bool valid, const TString& good = "") {
            bool state;
            TString result;

            UNIT_ASSERT_VALUES_EQUAL(state = NUrlNorm::NormalizeUrl(rawUrl, result), valid);

            if (valid) {
                UNIT_ASSERT_VALUES_EQUAL(result, good);
            }
        }

        Y_UNIT_TEST(NormalizeUrl) {
            Check("google.com", true, "http://google.com/");
            Check("http://google.com/1/", true, "http://google.com/1/");
            Check("GOOGLE.com:77/1", true, "http://google.com:77/1");
            Check("//google.com", true, "http://google.com/");

            Check("yandex://google", false);

            UNIT_ASSERT_VALUES_EQUAL(NUrlNorm::NormalizeUrl("google.com"), "http://google.com/");
            UNIT_ASSERT_VALUES_EQUAL(NUrlNorm::NormalizeUrl("http://google.com/1/"), "http://google.com/1/");
            UNIT_ASSERT_VALUES_EQUAL(NUrlNorm::NormalizeUrl("GOOGLE.com:77/1"), "http://google.com:77/1");

            UNIT_ASSERT_EXCEPTION(NUrlNorm::NormalizeUrl("yandex://google"), yexception);
        }
    }

    Y_UNIT_TEST_SUITE(TUrlDenormTest) {
        Y_UNIT_TEST(NormalizeUrl) {
            UNIT_ASSERT_STRINGS_EQUAL(NUrlNorm::DenormalizeUrlForWalrus("http://google.com/"), "google.com");
            UNIT_ASSERT_STRINGS_EQUAL(NUrlNorm::DenormalizeUrlForWalrus("http://google.com/1"), "google.com/1");
            UNIT_ASSERT_STRINGS_EQUAL(NUrlNorm::DenormalizeUrlForWalrus("https://google.com/"), "https://google.com");
            UNIT_ASSERT_STRINGS_EQUAL(NUrlNorm::DenormalizeUrlForWalrus("https://google.com/some+path/"), "https://google.com/some+path");
        }
    }

    Y_UNIT_TEST_SUITE(TDirectDraftTest) {
        Y_UNIT_TEST(NormalizeUrl) {
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://pixel.everesttech.net/129/c?ev_sid=90&ev_lx={PARAM127}&ev_crx=192618604&ev_ln={PHRASE}&ev_mt={STYPE}&ev_ltx=&ev_src={SRC}&ev_pos={POS}&ev_pt={PTYPE}&url=!http://apteka.ru/?utm_campaign=Gomeopatia_msc&utm_medium=Yandex_CPC&utm_source=Yandex_CPC&utm_term={PHRASE}.{PARAM127}&utm_content=__v3||192618604||{PARAM127}||{PHRASE}||{POS}||{PTYPE}||{SRC}||{STYPE}||{BM}&utm_id=ic|7121789|{GBID}|192618604|{PARAM127}|{PARAM126}|{STYPE}|{PTYPE}"),
                "http://apteka.ru/"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://ad.admitad.com/g/00f79afbb65f9daa52708f3d8a9702/?i=5&ulp=http%3A%2F%2Fwww.konik.ru%2Fcatalog%2Ftoys%2FAdora-20926.html&k5001&keyword={PHRASE}&source=yandex"),
                "http://www.konik.ru/catalog/toys/Adora-20926.html"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("https://ad.admitad.com/g/00f79afbb65f9daa52708f3d8a9702/?i=5&ulp=http%3A%2F%2Fwww.konik.ru%2Fcatalog%2Ftoys%2FAdora-20926.html&k5001&keyword={PHRASE}&source=yandex"),
                "http://www.konik.ru/catalog/toys/Adora-20926.html"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://ad.atdmt.com/s/go?adv=11087202444537&c.a=101__15321731&p.a=11087202444537-101__15321731&as.a=101__15321731_{GBID}&a.a=1421009191&bidkw={PHRASE}&bidmt=&ntwk={STYPE}&adpos={PTYPE}_{POS}&pnt1=yandex_&pnt2=xc2q63svi0&h=http%3A%2F%2Fwww.microsoft.com%2Fru-ru%2Fmobile%2Fphones%2Flumia%2F%3Forder_by%3DLatest%26dcmpid%3Dbmc-src-yandex.Nonbrand"),
                "http://www.microsoft.com/ru-ru/mobile/phones/lumia/?dcmpid=bmc-src-yandex.Nonbrand&order_by=Latest"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://clckto.ru/rd?sid=1&kid=10024136&ql=0&kw=-1&to=http://marte.ru/catalog/sportivno-balnye_1/"),
                "http://marte.ru/catalog/sportivno-balnye_1/"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://org-exam.ru/{PARAM126}xxx"),
                "http://org-exam.ru/xxx"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://pixel.everesttech.net/2724/cq?ev_sid=90&ev_lx={PARAM127}&ev_crx=1147195707&ev_ln={PHRASE}&ev_mt={STYPE}&ev_ltx=&ev_src={SRC}&ev_pos={POS}&ev_pt={PTYPE}&url=http%3A%2F%2Fwww.Lamoda.ru%2Fbg%2F1%2Fbrand-group-adidas%2F%3Futm_source%3DYDirect%26utm_medium%3Dcpc%26utm_campaign%3D14077046.%5Bbrand_00001%7Cloc_05%7Cn_000%5D%3A%20adidas%20regions%26utm_content%3D{PTYPE}_{POS}_{PHRASE}%26utm_term%3DPHR.{PARAM127}.1147195707"),
                "http://www.Lamoda.ru/bg/1/brand-group-adidas/"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://bestshellac.ru/collection/gel-laki-2?rs=direct1_%7BSTYPE%7D_1630183911_%7BPHRASE%7D"),
                "http://bestshellac.ru/collection/gel-laki-2"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://blackmilk.bz/skidki-i-akcii/?roistat=direct1_%7BSTYPE%7D_1867545139_%7BPHRASE%7D"),
                "http://blackmilk.bz/skidki-i-akcii/"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://532.xg4ken.com/media/redir.php?affcode=cr65810&camp=398084&cid=&k_inner_url_encoded=1&prof=1271&url%5B%5D=http%3A//www.novotel.com/lien_externe.svlt%3Fgoto%3Dliste_hotel%26nom_ville%3DNanjing%26code_langue%3Dru%26merchantid%3Dppc-nov-mar-yan-ww-ru-sear%26sourceid%3Dbp-cen%26xtor%3DSEC-706-yan-%5Bppc-nov-mar-yan-ww-ru-cn-exa-sear-bp-cen%5D-%5Bnov-V5407-Nanjing%5D-%7BifContent%3AC%7D%7BifSearch%3AS%7D-%5B%7BPHRASE%7D%5D"),
                "http://www.novotel.com/lien_externe.svlt?code_langue=ru&goto=liste_hotel&merchantid=ppc-nov-mar-yan-ww-ru-sear&nom_ville=Nanjing&sourceid=bp-cen"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://3dmarker.ru/moskva_rabochee_vremya/?utm_source=yandex&utm_medium=cpc&utm_campaign=cid|14524398|context&utm_content=gid|926577313|aid|1248047234|4267957232_&utm_term=3d%20%D1%80%D1%83%D1%87%D0%BA%D0%B0&pm_source=www.avito.ru&pm_block=none&pm_position=0&yclid=4227292947272961376"),
                "http://3dmarker.ru/moskva_rabochee_vremya/"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://av.ru/food/10343/?utm_source=google&utm_medium=cpc&utm_content=dev_c|dmod_|src_patriot-su-rf.ru|adid_106819939923|fid_|regid_9040985|&utm_campaign=|cn_msk_remarketing_g_seti|&utm_term=ph_aud-188733924003|key_|&gclid=CLrW5s_Vqc4CFWIW0wodBVICtw"),
                "http://av.ru/food/10343/"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://www.carnet.gr/MyCarsResults/Default.asp?p=3&brand_id=5&dealerid=&model_id=*&Color=&equipment=&cartype=&ym_min=0&ym_max=2016&Price_min=0&Price_max=*&km_min=0&km_max=*&ev_min=0&ev_max=1600&sortorder=3&county=&dealer=&isnew=&fueltype=&action=search&HasCoupon=&last="),
                "http://www.carnet.gr/MyCarsResults/Default.asp?Color=&HasCoupon=&Price_max=*&Price_min=0&action=search&brand_id=5&cartype=&county=&dealer=&dealerid=&equipment=&fueltype=&isnew=&km_max=*&km_min=0&last=&model_id=*&p=3&sortorder=3&ym_max=2016&ym_min=0"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://astudioauto.ru/catalog/parkovka/zerakala-s-monitorom/filter/nalichie_videoregistratora-is-a7abf4142ac6b44460e4ea666d4c7f1d/apply/?utm_source=yandex_direct&utm_medium=cpc&utm_campaign=18233149&utm_content=2077774894&utm_term=6565855402&region=213&region_name=%D0%9C%D0%BE%D1%81%D0%BA%D0%B2%D0%B0&block=other&position=3&roistat=direct2_search_2077774894_%D0%B2%D0%B8%D0%B4%D0%B5%D0%BE%D1%80%D0%B5%D0%B3%D0%B8%D1%81%D1%82%D1%80%D0%B0%D1%82%D0%BE%D1%80%20%D0%B7%D0%B5%D1%80%D0%BA%D0%B0%D0%BB%D0%BE%20%D0%BA%D1%83%D0%BF%D0%B8%D1%82%D1%8C&roistat_referrer=none&roistat_pos=other_3&yclid=4221982741783121041"),
                "http://astudioauto.ru/catalog/parkovka/zerakala-s-monitorom/filter/nalichie_videoregistratora-is-a7abf4142ac6b44460e4ea666d4c7f1d/apply/"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://akkord-smart.ru/?utm_source=yandex_tp&utm_medium=cpc&utm_campaign=Studii_RSYA_15644012&utm_term=%D0%BD%D0%BE%D0%B2%D0%B0%D1%8F%20%D0%BA%D0%B2%D0%B0%D1%80%D1%82%D0%B8%D1%80%D0%B0%20%D1%81%D1%82%D1%83%D0%B4%D0%B8%D1%8F&utm_content=novaja_kvartira_studija&cm_id=15644012_1089466788_2400008178_4706923513&_openstat=ZGlyZWN0LnlhbmRleC5ydTsxNTY0NDAxMjsyNDAwMDA4MTc4O3d3dy5taXNzZml0LnJ1Om5h&yclid=4132378614365425861"),
                "http://akkord-smart.ru/"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://xn--d1abbscb0aqe0a.xn--p1ai/fasad_dekor/lepnina_info.html?utm_source=yandex&utm_medium=cpc&utm_campaign=Aggo|SV|15269889|{STYPE}&utm_content=v1|{GBID}|aid|1409556944|_&utm_term={PHRASE}&pm_source={SRC}&pm_block={PTYPE}&pm_position={POS}&_openstat=dGVzdDsxOzE7&yclid=411049677757546"),
                "http://xn--d1abbscb0aqe0a.xn--p1ai/fasad_dekor/lepnina_info.html"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://ssl.hurra.com/TrackIt?tid=10031155C3841PPC&url=[[http://www.footballstore.ru/catalog/butsi/?br=1&tab=&perpage=&page=0&brands%5B%5D=4&collections%5B%5D=63&collections%5B%5D=367&collections%5B%5D=209&collections%5B%5D=319&collections%5B%5D=360&collections%5B%5D=75&collections%5B%5D=87&filter%5Bcovers%5D%5B%5D=1&filter%5Bprice1%5D=2557&filter%5Bprice2%5D=8990&utm_source=yandex&utm_medium=display&utm_campaign=Hurra_Remarketing]]&mc=575201"),
                "http://www.footballstore.ru/catalog/butsi/?br=1&brands%5B%5D=4&collections%5B%5D=63&collections%5B%5D=367&collections%5B%5D=209&collections%5B%5D=319&collections%5B%5D=360&collections%5B%5D=75&collections%5B%5D=87&filter%5Bcovers%5D%5B%5D=1&filter%5Bprice1%5D=2557&filter%5Bprice2%5D=8990&page=0&perpage=&tab="
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://adsplus.ru/g/c9f1ad68bc41aea550b03a3184f61a/?i=5&ulp=http%3A%2F%2Fwww.mvideo.ru%2Fproducts%2Fkuhonnyi-kombain-braun-fp5160bk-20031612%3FcityId%3DCityCZ_975&subid1=20213517_2498704911_{PARAM127}_{STYPE}&subid=FP5160BK&subid2={PTYPE}:{POS}:{GBID}:2498704911:{PARAM127}{PARAM126}&subid4=20213517_{REGN}&subid3=20213517"),
                "http://www.mvideo.ru/products/kuhonnyi-kombain-braun-fp5160bk-20031612?cityId=CityCZ_975"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://adsplus.ru/g/95d8174bcc41aea550b0e20a1b9cd2/?i=5&ulp=http%3A%2F%2Fwww.tehnosila.ru%2Fcatalog%2Fdom_i_dacha%2Fsilovaya_i_sadovaya_tehnika%2Fnasosnoe_oborudovanie%2Faksessuary_dkya_kommunikatsiy%2F-%2F216360&subid=20293391:{GBID}:2519901555:{PARAM127}:{STYPE}:{SRC}:{DEVICE_TYPE}:{POS}:{PTYPE}:{REG}:{PHRASE}&subid1=predvaritelnoy+ochistki+do+3000+l+ch&subid2={REGN}&subid4=20293391"),
                "http://www.tehnosila.ru/catalog/dom_i_dacha/silovaya_i_sadovaya_tehnika/nasosnoe_oborudovanie/aksessuary_dkya_kommunikatsiy/-/216360"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://alterprice.ru/ad/56fe9e1ecb72d84702b8e4c6?utm_campaign=http%3A%2F%2Foboi-store.ru&utm_medium=cpc&utm_source=GARPUN"),
                "http://oboi-store.ru"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://ad.doubleclick.net/ddm/clk/285351812;112269761;f?http://www.google.com/intx/ru_ru/enterprise/apps/business/products/gmail/index.html?utm_campaign=emea-smb-apps-bkws-ru-ru&utm_medium=cpc&utm_source=yandex&utm_term=adid462957879"),
                "http://www.google.com/intx/ru_ru/enterprise/apps/business/products/gmail/index.html"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://n.actionpay.ru/click/575171a38b30a85b138b456e/134372/direct/{REG}/{PARAM127}/url=http%3A%2F%2Fwww.akusherstvo.ru%2Fmagaz.php%3Faction%3Dshow_tovar%26tovar_id%3D107194"),
                "http://www.akusherstvo.ru/magaz.php?action=show_tovar&tovar_id=107194"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://ucl.mixmarket.biz/uni/clk.php?id=1294928045&zid=1294934337&prid=1294933344&stat_id=0&sub_id=sy1_p003_c-s1a1.g1a__t007___static_gr.id-{GBID}___dynamic_ban.id-2216736714_ph.id-{PARAM127}_dev-{DEVICE_TYPE}_s-{SRC}_g-{REG}&redir=http://my-shop.ru/shop/catalogue/2665/sort/a/page/1.html"),
                "http://my-shop.ru/shop/catalogue/2665/sort/a/page/1.html"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://clckto.ru/rd?sid=1&kid=7635687&ql=0&kw=-1&to=http://www.holodilova.ru/kupit/iskusstvennye_derevya/"),
                "http://www.holodilova.ru/kupit/iskusstvennye_derevya/"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://xn--24-6kctasmcexjxzc0c2h.xn--p1ai/g/c9f1ad68bca5b9881a803a3184f61a/?i=5&ulp=http%3A%2F%2Fwww.mvideo.ru%2Fproducts%2Fsabvufer-monitor-audio-radius-380-gloss-black-10005612%3FcityId%3DCityCZ_975&subid1=getdirect_18605654_2173065828_{PARAM127}_{STYPE}"),
                "http://www.mvideo.ru/products/sabvufer-monitor-audio-radius-380-gloss-black-10005612?cityId=CityCZ_975"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://y.actionpay.ru/click/5735b7728b30a8a36c8b45a8/87668/D2/3/75076/3516491/{PTYPE}{POS}/url=http://www.ulmart.ru/goods/3516491?from=actionpay_msk&utm_medium=affiliate&utm_source=actionpay&utm_content=79086&utm_campaign=remarketing&utm_term=3516491"),
                "http://www.ulmart.ru/goods/3516491"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://elektroveschi.ru/g/cf1399ae96a5b9881a80dd61df5557/?i=5&ulp=http%3A%2F%2Fwww.003.ru%2F0031323103%2Fsistemnyj_blok_asus_m32ad_ru007s&subid=003ru56&subid3={REGN}_{REG}&subid4={DEVICE_TYPE}_{PTYPE}_{POS}&subid1=getdirect_20135155_2480515738_{PARAM127}_{STYPE}"),
                "http://www.003.ru/0031323103/sistemnyj_blok_asus_m32ad_ru007s"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://k.laredoute.ru/kack/1/?kaPt=yandex&kaTckM=p&kaPcId=58244&kaTckId=1314639742&kaAcId=263&kaCpnId=&kaCpnPtId={сampaign_id}&kaAdgPtId={GBID}&kaCrPtId={PARAM127}&kaNtw={STYPE}&kaKwd={PHRASE}&kaAdPos={PTYPE}&kaAdPosId={POS}&kaDev={DEVICE_TYPE}&kaDmn={SRC}&kaRtgId={PARAM126}&kaTgtId={PARAM127}&kaTgtNm={PARAM126}&kaAddPh={BM}&kaAddPhT={PHRASE_BM}&kaRegId={REG}&kaRegNm={REGN}&kaRdt=http://www.laredoute.ru/brnd/cimarron.aspx?brndid=cimarron&from=40discount&omniturecode=PSN00084128RU&utm_campaign=K-PS_Invited_Marks_Moscow_Search|14828312&utm_medium=Yandex_CPC&utm_source=Yandex_CPC&position={PTYPE}_{POS}&utm_term={PARAM127}_{PHRASE}&utm_content=v2|1314639742|{PARAM127}|{PHRASE}|{POS}|{PTYPE}|{SRC}|{STYPE}|{BM}&utm_id=ic|14828312|{GBID}|1314639742|{PARAM127}|{PARAM126}|{STYPE}|{PTYPE}"),
                "http://www.laredoute.ru/brnd/cimarron.aspx?brndid=cimarron&omniturecode=PSN00084128RU"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://xn----8sbemayndh9b6e4af.xn--p1ai/g/dd8aeb9ade530064c104748f778371/?i=5&subid=S-NEW&ulp=http%3A%2F%2Fwww.akusherstvo.ru%2Fmagaz.php%3Faction%3Dshow_tovar%26tovar_id%3D28688&subid1=getdirect_15225543_1397343984_{PARAM127}_{STYPE}"),
                "http://www.akusherstvo.ru/magaz.php?action=show_tovar&tovar_id=28688"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("https://www.agoda.com/ru-ru/pages/agoda/default/DestinationSearchResult.aspx?asq=8aPafRgtsnzqHQFZwymw%2bSWG7sbQZNeVSt%2fvCcGqqT1A3uwXWlgSnJFiHgEZVi7YCj2sAvZL6hcZ6XYoIzruD2wS%2fQvcE26tSFpAxu65H2wqpIZ6T1fVUmUi03AGZdYquTfeLgY405H995FU19zfJw%3d%3d&altdt=1&type=1&site_id=1641965&url=http%3a%2f%2fwww.agoda.com%2fasia%2fthailand%2fbangkok%2fthanapa_mansion_don_muang_airport.html&utm_source=yandex&utm_medium=cpc&utm_campaign=18713742%2c2194118195%2c%7bSRC%7d%2c%7bPTYPE%7d%7bPOS%7d&utm_content=%7bPARAM127%7d%2c%7bBM%7d&utm_term=%7bkeywords%7d&tag=%7bPARAM2%7d&cklg=1"),
                "http://www.agoda.com/asia/thailand/bangkok/thanapa_mansion_don_muang_airport.html"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("https://www.agoda.com/ru-ru/city/augusta-au.html?type=1&site_id=1641967&url=http%3a%2f%2fwww.agoda.com%2fru-ru%2fcity%2faugusta-au.html&utm_source=yandex&utm_medium=cpc&utm_campaign=17596354%2c1914574543%2c%7bSRC%7d%2c%7bPTYPE%7d%7bPOS%7d&utm_content=%7bPARAM127%7d%2c%7bBM%7d&utm_term=%7bkeywords%7d&tag=%7bPARAM2%7d&cklg=1"),
                "http://www.agoda.com/ru-ru/city/augusta-au.html"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://track.himba.ru/13tj.html?url=http://www.pleer.ru/_263461_lowepro_hardside_300_photo_82750.html&utm_source=direct&utm_medium=cpc&utm_term={PHRASE}&utm_campaign=21-04-foto-aksessuary-studiinoe-oborudovanie-1-18218779&utm_content=__v3%7C%7C2074394240%7C%7C{PARAM127}%7C%7C{PHRASE}%7C%7C{POS}%7C%7C{PTYPE}%7C%7C{SRC}%7C%7C{STYPE}%7C%7C{BM}&sub_id=direct%7C%7Ccpc%7C%7C__v3%7C%7C2074394240%7C%7C{PARAM127}%7C%7C{PHRASE}%7C%7C{POS}%7C%7C{PTYPE}%7C%7C{SRC}%7C%7C{STYPE}%7C%7C{BM}"),
                "http://www.pleer.ru/_263461_lowepro_hardside_300_photo_82750.html"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://www.domotdiha.ru/ru/gostinitsa-streletskaya-sloboda.html/?how_to_get"),
                "http://www.domotdiha.ru/ru/gostinitsa-streletskaya-sloboda.html/?how_to_get="
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://pixel.everesttech.net/2724/cq?ev_sid=90&ev_lx={PARAM127}&ev_crx=1166519652&ev_ln={PHRASE}&ev_mt={STYPE}&ev_ltx=&ev_src={SRC}&ev_pos={POS}&ev_pt={PTYPE}&url=%0Ahttp%3A%2F%2Fwww.lamoda.ru%2Fc%2F129%2Fshoes-kedy%2F%3Fcolors%3D615%26types_of_heels%3D%25D0%25A1%25D0%25BA%25D1%2580%25D1%258B%25D1%2582%25D0%25B0%25D1%258F%2B%25D1%2582%25D0%25B0%25D0%25BD%25D0%25BA%25D0%25B5%25D1%2582%25D0%25BA%25D0%25B0%252C%25D0%25A2%25D0%25B0%25D0%25BD%25D0%25BA%25D0%25B5%25D1%2582%25D0%25BA%25D0%25B0%26utm_source%3DYDirect%26utm_medium%3Dcpc%26utm_campaign%3D14149176.%255Bcat_00212%257Cloc_05%257Cn_000%255D%253A%2520snikersy%2520regions%26utm_content%3D{PTYPE}_{POS}_{PHRASE}%26utm_term%3DPHR.{PARAM127}.1166519652%26utm_dm%3Dcat_00212%257Cec-buy_00007%257Catt-col_00002%257Cloc_05.{PARAM1}}"),
                "http://www.lamoda.ru/c/129/shoes-kedy/?colors=615&types_of_heels=%D0%A1%D0%BA%D1%80%D1%8B%D1%82%D0%B0%D1%8F+%D1%82%D0%B0%D0%BD%D0%BA%D0%B5%D1%82%D0%BA%D0%B0%2C%D0%A2%D0%B0%D0%BD%D0%BA%D0%B5%D1%82%D0%BA%D0%B0"
            );

            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://dachnyi.com/store/sistema-poliva?ad_id=2837365411&campaign_id=21479444&phrase_id="),
                "http://dachnyi.com/store/sistema-poliva"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://s.click.aliexpress.com/deep_link.htm?af=endoscope&aff_short_key=UN3JUrB&cn=37&cv=yaElectronic&dl_target_url=http%3A//ru.aliexpress.com/af/endoscope-%D0%BF%D0%BE%D0%B4%D1%81%D0%B2%D0%B5%D1%82%D0%BA%D0%BE%D0%B9-.html%3FSortType%3Ddefault&dp=ya"),
                "http://ru.aliexpress.com/af/endoscope-подсветкой-.html?SortType=default"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://3dmarket.by/?utm_source=yandex&utm_medium=cpc&utm_campaign=22163952&utm_content=3027304826&utm_term={PHRASE}ya_kvartir||http://3dmarket.by?utm_source=yandex&utm_medium=cpc&utm_campaign=dlya_kafe||http://3dmarket.by?utm_source=yandex&utm_medium=cpc&utm_campaign=dlya_restoranov"),
                "http://3dmarket.by/"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://ad-emea.doubleclick.net/clk;278196954;105429447;w?https://www.google.com.ua/intl/uk/chrome/browser/?installdataindex=nosearch&hl=ru&brand=CHHP&utm_campaign=ru&utm_source=ua-oa-yandex_contextual_skws&utm_medium=oa&utm_term=yandex"),
                "https://www.google.com.ua/intl/uk/chrome/browser/?brand=CHHP&hl=ru&installdataindex=nosearch"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://aff.optionbit.com/l.aspx?A=2895&SubAffiliateID=YNDXS11_SEARCH_besplatnaya_binarnaya_pomoshh&TargetURL=http://nlp.optionbit.com/ru/lps/trade-with-comfort"),
                "http://nlp.optionbit.com/ru/lps/trade-with-comfort"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://app.appsflyer.com/com.jaumo-track?pid=yandex_int&af_r=http://jaumo.tk/track/direkt/yadirect/campaign-ads/?source_type={STYPE}&source={SRC}&position_type={PTYPE}&position={POS}&keyword={PHRASE}&campaign_id=16036539&ad_id=1577770925&clickid=%AWAPS_REQUEST_ID%&gaid=%AWAPS_IFA%&androidid=%AWAPS_ANDROID_ID%"),
                "http://jaumo.tk/track/direkt/yadirect/campaign-ads/?androidid=%25AWAPS_ANDROID_ID%25&clickid=%25AWAPS_REQUEST_ID%25&gaid=%25AWAPS_IFA%25"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://clickserve.dartsearch.net/link/click?lid=43700013884297208&ds_s_kwgid=58700000651232223&ds_url_v=2&ds_dest_url=http://www.hilton.ru/hotels/?search=Варшава&WT.mc_id=zELWAAA0EU1WW2PSH3Cluster4DGGenericx&WT.srch=1&utm_source=Yandex&utm_medium=ppc&utm_campaign=paidsearch"),
                "http://www.hilton.ru/hotels/?WT.mc_id=zELWAAA0EU1WW2PSH3Cluster4DGGenericx&WT.srch=1&search=%D0%92%D0%B0%D1%80%D1%88%D0%B0%D0%B2%D0%B0"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://clk.tradedoubler.com/click?p=254948&a=2449779&g=22194048&epi=5-10781-__-msoffgen&url=https://www.microsoftstore.com/store/msmea/uk_UA/cat/%D0%9F%D0%BE%D1%80%D1%96%D0%B2%D0%BD%D1%8F%D0%BD%D0%BD%D1%8F-%D0%BF%D1%80%D0%BE%D0%B3%D1%80%D0%B0%D0%BC%D0%BD%D0%B8%D1%85-%D0%BA%D0%BE%D0%BC%D0%BF%D0%BB%D0%B5%D0%BA%D1%81%D1%96%D0%B2-Office-/categoryID.70246500/mktp.UA/Currency.UAH"),
                "https://www.microsoftstore.com/store/msmea/uk_UA/cat/%D0%9F%D0%BE%D1%80%D1%96%D0%B2%D0%BD%D1%8F%D0%BD%D0%BD%D1%8F-%D0%BF%D1%80%D0%BE%D0%B3%D1%80%D0%B0%D0%BC%D0%BD%D0%B8%D1%85-%D0%BA%D0%BE%D0%BC%D0%BF%D0%BB%D0%B5%D0%BA%D1%81%D1%96%D0%B2-Office-/categoryID.70246500/mktp.UA/Currency.UAH"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://go.sellaction.net/go.php?id=6bf89d0cb11c7adc9890d0&utm_source=yandex&utm_medium=cpc&utm_campaign=aromat-discount-ru-tovarnaja-xml-rsja-10001&utm_term={PHRASE}&utm_content=ad-10-price-kopija-kopija&wutm=1&url=http%3A%2F%2Faromat-discount.ru%2Fproducts%2Fsalvatore-ferragamo-incanto-bloom"),
                "http://aromat-discount.ru/products/salvatore-ferragamo-incanto-bloom"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("https://href.li/?http://izobility.com/catalog/supershub?source={SRC}&keyword={PHRASE}&position={POS}&added={BM}&type={STYPE}&block={PTYPE}&utm_source=yandex.ximik&utm_medium=cpc&utm_campaign=21837343_cat_iskusstvennie_shubi_s_russia&utm_content=2936654101_{PARAM127}{PARAM126}_{PHRASE}&utm_term={PHRASE}&admitad_uid=1e152891b424b430500a7e5e9dfe3965"),
                "http://izobility.com/catalog/supershub?admitad_uid=1e152891b424b430500a7e5e9dfe3965"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("https://m.vk.com/away.php?to=http%3A%2F%2Fi-want.pro%2F%25D0%25B8%25D0%25B2%25D0%25B5%25D0%25BD%25D1%2582%25D1%258B%2F%25D0%25BE%25D1%2580%25D0%25B3%25D0%25B0%25D0%25BD%25D0%25B8%25D0%25B7%25D0%25B0%25D1%2586%25D0%25B8%25D1%258F-%25D0%25BF%25D1%2580%25D0%25B0%25D0%25B7%25D0%25B4%25D0%25BD%25D0%25B8%25D0%25BA%25D0%25BE%25D0%25B2-%25D0%25BF%25D0%25BE%25D0%25B4-%25D0%25BA%25D0%25BB%25D1%258E%25D1%2587%2F%25D0%25B7%25D0%25B0%25D0%25BA%25D0%25B0%25D0%25B7%25D0%25B0%25D1%2582%25D1%258C-%25D0%25BF%25D1%2580%25D0%25B0%25D0%25B7%25D0%25B4%25D0%25BD%25D0%25B8%25D0%25BA-%25D0%25B4%25D0%25BB%25D1%258F-%25D0%25B2%25D1%2581%25D0%25B5%25D0%25B9-%25D1%2581%25D0%25B5%25D0%25BC%25D1%258C%25D0%25B8%2F/?utm_source=yd&utm_medium=cpc&utm_campaign=iwant_yd_msk_s_general_birthday_broad&utm_term={PHRASE}&utm_content=2|2442872704|block={PTYPE}|position={POS}"),
                "http://i-want.pro/%D0%B8%D0%B2%D0%B5%D0%BD%D1%82%D1%8B/%D0%BE%D1%80%D0%B3%D0%B0%D0%BD%D0%B8%D0%B7%D0%B0%D1%86%D0%B8%D1%8F-%D0%BF%D1%80%D0%B0%D0%B7%D0%B4%D0%BD%D0%B8%D0%BA%D0%BE%D0%B2-%D0%BF%D0%BE%D0%B4-%D0%BA%D0%BB%D1%8E%D1%87/%D0%B7%D0%B0%D0%BA%D0%B0%D0%B7%D0%B0%D1%82%D1%8C-%D0%BF%D1%80%D0%B0%D0%B7%D0%B4%D0%BD%D0%B8%D0%BA-%D0%B4%D0%BB%D1%8F-%D0%B2%D1%81%D0%B5%D0%B9-%D1%81%D0%B5%D0%BC%D1%8C%D0%B8//"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("https://pxl.leads.su/click/0178f72425339bdff18ecd6a03175a0f?deeplink=http%3A%2F%2Fbankspro.ru%2Fcategory%2F14-mikrokrediti&aff_sub1=marketpsk-89"),
                "http://bankspro.ru/category/14-mikrokrediti"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("https://rdr.salesdoubler.com.ua/in/offer/180?aid=7926&dlink=http%3A%2F%2Fwww.eldorado.com.ua%2Facer-aspire-f5-573g-573z-nxgfjeu013%2Fp1413156%2F%3Futm_source%3DSalesDoubler%26utm_medium%3Dcpc%26utm_term%3D71213140%26utm_campaign%3Dnotebooks"),
                "http://www.eldorado.com.ua/acer-aspire-f5-573g-573z-nxgfjeu013/p1413156/"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("https://redirect.appmetrica.yandex.com/serve/24798507072324806?ad_id=3174975845&keyword={PHRASE}&back_url=https://play.google.com/store/apps/details?id=com.zrgiu.antivirus&utm_source={SRC}&utm_medium=cpc&utm_campaign=22706233&utm_content={PTYPE}.{POS}&utm_term={PHRASE}"),
                "https://play.google.com/store/apps/details?id=com.zrgiu.antivirus"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://n.apclick.ru/click/582251c08b30a8690a8b460c/98162/pirkons2015/ulmart/spb/url=http%3A%2F%2Fwww.ulmart.ru%2Fgoods%2F4064626%3Ffrom%3Dactionpay_spb%26amp%3Butm_medium%3Daffiliate%26amp%3Butm_source%3Dactionpay%26amp%3Butm_content%3D99781%26amp%3Butm_campaign%3Dremarketing%26amp%3Butm_term%3D4064626"),
                "http://www.ulmart.ru/goods/4064626"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://nkck.ru/katalog/krovlya/profnastil-krovlya/?utm_source=yandex&utm_medium=cpc&utm_campaign=15936282&utm_content=1556625348&utm_term={PHRASE}||http://nkck.ru/katalog/napolnye-pokrytiya/?utm_source=yandex&utm_medium=cpc&utm_campaign=15936282&utm_content=1556625348&utm_term={PHRASE}||http://nkck.ru/katalog/napolnye-pokrytiya/linoleum/?utm_source=yandex&utm_medium=cpc&utm_campaign=15936282&utm_content=1556625348&utm_term={PHRASE}||http://nkck.ru/gde-kupit/?utm_source=yandex&utm_medium=cpc&utm_campaign=15936282&utm_content=1556625348&utm_term={PHRASE}"),
                "http://nkck.ru/katalog/krovlya/profnastil-krovlya/"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://yadcounter.ru/count/?from={PARAM127}&stext=694xmVcEgLOPiFu6vxLx5sGnYxOf19321874D68SMvuo0MRdzctmgaQOG7ETlFmy&lang=ru&mc={REG}&state=df277aca4b7e8a9ba4cd970bdf15bea02270821143AbJDJz1eUbklAOi2keQQ8BF2IEDl&url=https%3A%2F%2Fpuzzle-english.com&yagla={PARAM2}"),
                "https://puzzle-english.com"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://ssl.hurra.com/TrackIt?tid=10067544C577PPC&url=[[http%3A%2F%2Fwww.bonprix.ru%2Fprodukty%2Fkurtka-slim-fit-chiernyi-971504%2F%3Flandmark%3DEntry%26wkz%3D98%26iwl%3D218%26typ%3DRET%26anbieter%3DYandexRET_dyn%26aktion%3D%D0%9A%D1%83%D1%80%D1%82%D0%BA%D0%B8]]"),
                "http://www.bonprix.ru/produkty/kurtka-slim-fit-chiernyi-971504/?aktion=%D0%9A%D1%83%D1%80%D1%82%D0%BA%D0%B8&anbieter=YandexRET_dyn&iwl=218&landmark=Entry&typ=RET&wkz=98"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://pricesguru.ru/in.php?ad_id=2035203500&id=3a55e3ad8b18ed73ae4b30b5028bfffc&retargeting_id=&url=aHR0cDovL3ByaWNlc2d1cnUucnUvcmV0YWlsZXIvZGVzaXJlKzUxNitkdWFsK3NpbS9waWQtMTA5NTU5MDUvY2lkLTkxNDkxLw%3D%3D"),
                "http://pricesguru.ru/retailer/desire+516+dual+sim/pid-10955905/cid-91491/"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://rosurist.com/dengi-dtp/nnov/pc/direct/?ad=1909655891&phrase=&retargeting="),
                "http://rosurist.com/dengi-dtp/nnov/pc/direct/?phrase=&retargeting="
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://sales.ubrr.ru/acquiring?advertising=2870255826&campaign=21603151&design=food&form=baseterminal&ldg=contextcontext&ldg=context&phrase="),
                "http://sales.ubrr.ru/acquiring?design=food&form=baseterminal&ldg=contextcontext&ldg=context&phrase="
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://xn-----elchd6askbcgiijefw9b.xn--p1ai/?ad_id=3299840384&compaign_id=23171914"),
                "http://xn-----elchd6askbcgiijefw9b.xn--p1ai/"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("https://binpartner.com/ru?ac=YandexPartnerka&ref=fd1863b980d8&sa=okolocelevie1&yd_ad_id=2623875363&yd_adtarget_id=&yd_adtarget_name=&yd_campaign_id=20692180&yd_campaign_type=type1&yd_phrase_id=&yd_retargeting_id="),
                "https://binpartner.com/ru?ac=YandexPartnerka&ref=fd1863b980d8&sa=okolocelevie1&yd_adtarget_id=&yd_adtarget_name=&yd_campaign_type=type1&yd_phrase_id=&yd_retargeting_id="
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://www.roskosmetika.ru/category/kosmetika-dlja-lica/himicheskie-pilingi?utm_medium=cpc&utm_source=yandex&utm_campaign=[Regionyi]Piling_teplaya-|18040458&utm_term={PHRASE}&utm_content=k50id|01000000_|cid|18040458|gid|{GBID}|aid|2025219129|adp|{BM}|pos|{PTYPE}{POS}|src|{STYPE}_{SRC}|dvc|{DEVICE_TYPE}|main&k50id=01000000_&gtm_source={SRC}&gtm_source_type={STYPE}&gtm_position={PTYPE}+{POS}#f[]=10?utm_source=yandex&utm_medium=cpc&utm_term={PHRASE}&utm_content=k50id|01000000_|cid|18040458|gid|{GBID}|aid|2025219129|adp|{BM}|pos|{PTYPE}{POS}|src|{STYPE}_{SRC}|dvc|{DEVICE_TYPE}|%dop%&utm_campaign=18040458&gtm_source={SRC}&gtm_source_type={STYPE}&gtm_position={PTYPE}+{POS}"),
                "http://www.roskosmetika.ru/category/kosmetika-dlja-lica/himicheskie-pilingi?k50id=01000000_#f[]=10?utm_source=yandex&utm_medium=cpc&utm_term=&utm_content=k50id|01000000_|cid|18040458|gid||aid|2025219129|adp||pos||src|_|dvc||%dop%&utm_campaign=18040458&gtm_source=&gtm_source_type=&gtm_position=+"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://clickserve.dartsearch.net/link/click?WT.mc_id=C_RU_paidsearch_Yandex_Non+Brandname_null_null_null&WT.srch=1&chn=SEM&cmp=Non+Brandname&ctycmp=RU&dev=device&ds_s_kwgid=58700001228312771&ds_url_v=2&lid=43700010776141601&src=Yandex&tcs=324&to=MIA&url=http%3A//www.airfrance.ru/RU/ru/local/process/standardbooking/SearchPrefillAction.do%3F"),
                "http://www.airfrance.ru/RU/ru/local/process/standardbooking/SearchPrefillAction.do"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("https://ad.doubleclick.net/ddm/searchclk?ds_aid=700000001620186&ds_e_ca_id=12273096&ds_e_ca_name=RU+-+Generic+-+Hoodies+and+Sweatshirts+-+Restructured&ds_e_ag_id={GBID}&ds_e_ag_name=%D0%93%D1%80%D1%83%D0%BF%D0%BF%D0%B0+%E2%84%96661524029&ds_e_cr_id={PARAM127}&ds_e_cr={PHRASE}&ds_e_ad_id=850960845&ds_e_ad_name=%23%D0%98%D0%BD%D1%82%D0%B5%D1%80%D0%BD%D0%B5%D1%82-%D0%BC%D0%B0%D0%B3%D0%B0%D0%B7%D0%B8%D0%BD+%D1%81%D0%B2%D0%B8%D1%82%D1%88%D0%BE%D1%82%D0%BE%D0%B2%23+ASOS&ds_dest_url=http%3A%2F%2Fwww.asos.com%2Fru%2Fsearch%2F%D1%81%D0%B2%D0%B8%D1%82%D1%88%D0%BE%D1%82%3Fq%3D%D1%81%D0%B2%D0%B8%D1%82%D1%88%D0%BE%D1%82%26channelref%3Dpaid%2Bsearch%26affid%3D9012%26utm_source%3Dyandex%26utm_medium%3Dcpc%26utm_campaign%3DRU%2B-%2BGeneric%2B-%2BHoodies%2Band%2BSweatshirts%2B-%2BRestructured%26utm_term%3D%7Bphrase_id%7D%26ppcadref%3D%7Bcampaignid%7D%7C%7Badgroupid%7D%7C%7Btargetid%7D"),
                "http://www.asos.com/ru/search/свитшот?affid=9012&channelref=paid+search&q=%D1%81%D0%B2%D0%B8%D1%82%D1%88%D0%BE%D1%82"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("http://rybinsk.kolesatyt.ru/catalog/diski/yokatta/model-58/yokatta-model-58-8x19-5x112-et47-d66-6-b-r/?utm-campaign=rybinsk.kolesatyt.ru-diski"),
                "http://rybinsk.kolesatyt.ru/catalog/diski/yokatta/model-58/yokatta-model-58-8x19-5x112-et47-d66-6-b-r/"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("https://ozon.onelink.me/SNMZ?pid=yandex_direct&is_retargeting=true&af_click_lookback=7d&af_dp=ozon%3A%2F%2Fproducts%2F30834481%2F&af_web_dp=https%3A%2F%2Fwww.ozon.ru%2Fcontext%2Fdetail%2Fid%2F30834481%2F&af_ios_url=https%3A%2F%2Fwww.ozon.ru%2Fcontext%2Fdetail%2Fid%2F30834481%2F&af_android_url=https%3A%2F%2Fwww.ozon.ru%2Fcontext%2Fdetail%2Fid%2F30834481%2F&utm_source=yandex_direct&utm_medium=cpc&utm_campaign=product_msk_dsa_all_appliance_vetchinnitsy_kci&c=dsa_msk_dsa_all_appliance&utm_content=id_30834481|catid_10571"),
                "https://www.ozon.ru/context/detail/id/30834481/"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("https://farfetch.onelink.me/4218782940?pid=yandexdirect_int&amp;is_retargeting=true&amp;af_dp=farfetch://deeplink/productdetail/13945532&amp;af_ios_url=https://www.farfetch.com/ru/shopping/women/nicholas-kirkwood-maeva-item-13945532.aspx?size=27&amp;amp;storeid=10200&amp;af_android_url=https://www.farfetch.com/ru/shopping/women/nicholas-kirkwood-maeva-item-13945532.aspx?size=27&amp;amp;storeid=10200&amp;af_web_dp=https://www.farfetch.com/ru/shopping/women/nicholas-kirkwood-maeva-item-13945532.aspx?size=27&amp;amp;storeid=10200"),
                "https://www.farfetch.com/ru/shopping/women/nicholas-kirkwood-maeva-item-13945532.aspx?size=27"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("https://darkstore.onelink.me/aH8K?pid=yandexdirect_int&amp;is_retargeting=true&amp;af_click_lookback=7d&amp;af_dp=ru.perekrestok.app.darkstore%3A%2F%2Fproduct%3Fid%3D959527&amp;af_reengagement_window=30d&amp;af_web_dp=https%3A%2F%2Fwww.vprok.ru%2Fproduct%2Faxe-axe-dez-skeytsv-rozy-aer-150ml--959527&amp;af_android_url=https%3A%2F%2Fwww.vprok.ru%2Fproduct%2Faxe-axe-dez-skeytsv-rozy-aer-150ml--959527&amp;clickid={logid}&amp;google_aid={googleaid}&amp;android_id={androidid}"),
                "https://www.vprok.ru/product/axe-axe-dez-skeytsv-rozy-aer-150ml--959527"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("https://sb76.adj.st/cars/used/sale/audi/a6/1065442514-a0ca1356/?adjust_t=s6bfls_qxyi1x&amp;adjust_campaign={campaign_name}&amp;adjust_adgroup={gbid}_{ad_id}&amp;adjust_creative={source}&amp;adjust_deeplink=autoru://app/cars/used/sale/audi/a6/1065442514-a0ca1356/&amp;adjust_fallback=https://auto.ru/cars/used/sale/audi/a6/1065442514-a0ca1356/"),
                "https://auto.ru/cars/used/sale/audi/a6/1065442514-a0ca1356/"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("https://1389598.redirect.appmetrica.yandex.com/product/pierre-cardin-karandash-dlia-gub-lipliner-waterproof-red-passion/100848391143?appmetrica_tracking_id=675398289859896266&referrer=reattribution%3D1&click_id={LOGID}&ios_ifa_sha1={IDFA_LC_SH1}&search_term={keyword}&device_type={device_type}&region_name={region_name}&source_type={source_type}&phrase_id={phrase_id}&ios_ifa={IOSIFA}&source={source}&position_type={position_type}&campaign_id={campaign_id}&path=%2Fproduct%2Fpierre-cardin-karandash-dlia-gub-lipliner-waterproof-red-passion%2F100848391143&utm_term=8510396|100848391143"),
                "https://pokupki.market.yandex.ru/product/pierre-cardin-karandash-dlia-gub-lipliner-waterproof-red-passion/100848391143"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("https://8jxm.adj.st/external?service=grocery&amp;adjust_t=as07wq9_65ujko4&amp;adj_deeplink=yandexlavka%3A%2F%2Fexternal%3Fservice%3Dgrocery%26href%3D%3Fitem%3D1aba85b40c324170949f47cc0d9b8c94000100010001&href=?item=1aba85b40c324170949f47cc0d9b8c94000100010001&adjust_campaign=%5BLVK%5DMA_Lavka-goal_RU-MOS-MOW_dynamic-network_RM&amp;adjust_fallback=https%3A%2F%2Feda.yandex%2Flavka%2F%26&utm_medium=cpc&utm_source=yasearch&utm_campaign=56083718.[EDA]DT_LUA-goal_RU-MOS-MOW_gCategory_restype-search_NU&utm_content=9779598080.{GBID}&utm_term={PHRASE}.{PHRASE_EXPORT_ID}&x=y"),
                "https://eda.yandex/lavka/?x=y"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("https://level.travel/hotels/9021054-Fortuna_Nha_Trang_Center_3%2A?from=Moscow-RU&amp;start_date=04.04.2019&amp;nights=7&amp;adults=2&amp;kids=0&show_search_params=true&utm_source=yandex&utm_medium=cpc&utm_campaign=arwm_yd_search_msk_fortuna_hotels_k50feed&utm_content=k50id|01000000{PHRASE_EXPORT_ID}_{PARAM126}|cid|37898909|gid|{GBID}|aid|6469018059|adp|{BM}|pos|{PTYPE}{POS}|src|{STYPE}_{SRC}|dvc|{DEVICE_TYPE}_6469018059_type1_{REGN_BS}_{COEF_GOAL_CONTEXT_ID}_{PARAM126}_{PARAM127}_{REG_BS}&utm_term={PHRASE}"),
                "https://level.travel/hotels/9021054-Fortuna_Nha_Trang_Center_3%2A?adults=2&kids=0&nights=7&show_search_params=true&start_date=04.04.2019"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("https://8jxm.adj.st/route?level=60&adjust_t=t6f6c0&adjust_campaign=%5BYT%5DMA_UA-goal_RU-NVS-ALL_Generic_iPhone.35440785&tracker_limit=1000000&adjust_adgroup={GBID}&ya_click_id={TRACKID}&adjust_creative=6009255592_{PHRASE}&adjust_idfa={IDFA_UC}&adj_deeplink=yandextaxi%3A%2F%2Froute%3Flevel%3D60"),
                "yandextaxi://route?level=60"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("https://beru.ru/special/blackberu?clid=602&utm_source=yandex&utm_medium=net&utm_campaign=yb_generic_blackberu_remyb_net_rus&utm_content=cid:47544434|gid:{GBID}|aid:8271992507|ph:{PHRASE_EXPORT_ID}|pt:{PTYPE}|pn:{POS}|src:{SRC}|st:{STYPE}|cgcid:{COEF_GOAL_CONTEXT_ID}|{SRC}&adjust_t=fs3pybh"),
                "https://beru.ru/special/blackberu?clid=602"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeDirectUrlDraft("https://8jxm.adj.st?adj_t=4fuqa1p_2cd4pff&adj_deep_link=yandextaxi%3A%2F%2F&adj_campaign=[YT]DT_UA-goal_RU-MOW-MSK_Video_Center&adj_adgroup=%7Badgroupid%7D_%7Bcreative%7D&adj_creative=%7Bplacement%7D8jxm.adj.st?adj_t=4fuqa1p_2cd4pff&adj_deep_link=yandextaxi%3A%2F%2F&adj_campaign=[YT]DT_UA-goal_RU-MOW-MSK_Video_Center&adj_adgroup=%7Badgroupid%7D_%7Bcreative%7D&adj_creative=%7Bplacement%7D&adj_fallback=https%3A%2F%2Ftaxi.yandex.ru%2Faction%2Fapp%2F%3Futm_medium%3Dcpc%26utm_source%3Dyadirect%26utm_campaign%3D%5BYT%5DDT_UA-goal_RU-MOW-MSK_Video_Center%26utm_term%3D%7Bkeyword%7D%26utm_content%3D%7Badgroupid%7D_%7Bcфreative%7D"),
                "https://taxi.yandex.ru/action/app/"
            );
        }
    }

    Y_UNIT_TEST_SUITE(TNormalizeUrlCgi) {
        Y_UNIT_TEST(NormalizeUrl) {
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeUrlCgi("https://yandex.ru/yandsearch", NUrlNorm::IsUtmOrYrwinfo),
                "https://yandex.ru/yandsearch"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeUrlCgi("https://yandex.ru/yandsearch#fragment", NUrlNorm::IsUtmOrYrwinfo),
                "https://yandex.ru/yandsearch#fragment"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeUrlCgi("https://yandex.ru/yandsearch?param1=val1&param2=val2", NUrlNorm::IsUtmOrYrwinfo),
                "https://yandex.ru/yandsearch?param1=val1&param2=val2"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeUrlCgi("https://yandex.ru/yandsearch?param1=val1&param2=val2#fragment", NUrlNorm::IsUtmOrYrwinfo),
                "https://yandex.ru/yandsearch?param1=val1&param2=val2#fragment"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeUrlCgi("https://yandex.ru/yandsearch?param1=val1&utm_suff1=val&param2=val2&yrwinfo=someval&param3=val3&utm_suffix2=utm_val", NUrlNorm::IsUtmOrYrwinfo),
                "https://yandex.ru/yandsearch?param1=val1&param2=val2&param3=val3"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeUrlCgi("https://yandex.ru/yandsearch?param1=val1&utm_suff1=val&param2=val2&yrwinfo=someval&param3=val3&utm_suffix2=utm_val#fragment", NUrlNorm::IsUtmOrYrwinfo),
                "https://yandex.ru/yandsearch?param1=val1&param2=val2&param3=val3#fragment"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeUrlCgi("https://yandex.ru/yandsearch?utm_suff1=val&yrwinfo=someval&utm_suffix2=utm_val", NUrlNorm::IsUtmOrYrwinfo),
                "https://yandex.ru/yandsearch"
            );
            UNIT_ASSERT_STRINGS_EQUAL(
                NUrlNorm::NormalizeUrlCgi("https://yandex.ru/yandsearch?utm_suff1=val&yrwinfo=someval&utm_suffix2=utm_val#fragment", NUrlNorm::IsUtmOrYrwinfo),
                "https://yandex.ru/yandsearch#fragment"
            );
        }
    }
} // anonymous namespace
