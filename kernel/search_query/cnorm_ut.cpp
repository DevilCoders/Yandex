#include <kernel/search_query/cnorm.h>
#include <util/charset/utf8.h>
#include <util/charset/wide.h>
#include <util/stream/file.h>
#include <util/stream/zlib.h>
#include <util/datetime/cputimer.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/resource/resource.h>
#include <ysite/yandex/reqanalysis/fast_normalize_request.h>
#include <kernel/qtree/request/reqlenlimits.h>
#include <kernel/qtree/request/request.h>

namespace {
    const TString InputQueries = NResource::Find("/texts");

    struct TFastNormalizerHolder {
        const NQueryNorm::TFastNormalizer Normalizer{false};
    };

    TString QNorm(TStringBuf in) {
        TUtf16String wideQuery = UTF8ToWide(in);
        try {
            TUtf16String wideQnormed = Default<TFastNormalizerHolder>().Normalizer(wideQuery);
            return WideToUTF8(wideQnormed);
        } catch (yexception&) {
            return {};
        }
    }

    void Test(std::function<void(const TString&)> handler) {
        TStringInput input(InputQueries);
        TString line;
        while (input.ReadLine(line)) {
            handler(line);
        }
    }
}

Y_UNIT_TEST_SUITE(TConfigurableNormalizerTest) {
    Y_UNIT_TEST(ConvertTest) {
        UNIT_ASSERT_EQUAL(WideToUTF8(TUtf32String::FromUtf8("abc-аБд")), "abc-аБд");
        TUtf16String utf16 = UTF8ToWide("制服美人");
        TUtf32String utf32 = TUtf32String::FromUtf16(utf16);
        UNIT_ASSERT_EQUAL(utf16, UTF32ToWide(utf32.data(), utf32.length()));
    }

    Y_UNIT_TEST(SimpleTest) {
        auto check = [](const TStringBuf in, TStringBuf expected1, TStringBuf expected2) {
            {
                TString returned = NCnorm::Cnorm(in);
                UNIT_ASSERT_NO_DIFF(returned, expected1);
            }
            {
                TString returned = NCnorm::BNorm(in);
                UNIT_ASSERT_NO_DIFF(returned, expected2);
            }
            {
                TString returned = NCnorm::BNormOld(in);
                UNIT_ASSERT_NO_DIFF(returned, expected2);
            }
        };

        check("abc-ABC", "abc abc", "abc - abc");
        check("abc-A9C", "abc a 9 c", "abc - a9c");
        check("abc-ABC abc-ABC", "abc abc abc abc", "abc - abc abc - abc");
        check("aбвгд~x~yz", "aбвгд x yz", "aбвгд ~ x ~ yz");
        check("aбв9ГД~x~!:yz", "aбв 9 гд x yz", "aбв9гд ~ x ~ ! : yz");
        check("abc-ABC url:yandex.ru xyz", "abc abc xyz", "abc - abc xyz");
        check("abc-ABC url:yandex.ru xyz\255", "abc abc xyz", "abc - abc xyz");//non utf8 example
        check(TString(1000, 'A'), TString(32, 'a'), TString(512, 'a'));
        check("fio_len:5 fio_len:5.qq asd:asd", "asd asd", "asd : asd");
        check("fio_len:5 url", "url", "url");
        check("филипп киркоров марина текст песни полностью 𝅘𝅥𝅯 𝅘𝅥𝅯 𝅘𝅥𝅯", "филипп киркоров марина текст песни полностью 𝅘 𝅘 𝅘", "филипп киркоров марина текст песни полностью 𝅘 𝅘 𝅘");
    }

    Y_UNIT_TEST(CanonizeCNorm) {
        Test([](const TString& line) {
            TString normalized = NCnorm::Cnorm(line);
            Cout << "\"" << line << "\" => \"" << normalized << "\"" << Endl;
        });
    }

    Y_UNIT_TEST(CanonizeBNorm) {
        Test([](const TString& line) {
            TString normalized = NCnorm::BNorm(line);
            Cout << "\"" << line << "\" => \"" << normalized << "\"" << Endl;
        });
    }

    Y_UNIT_TEST(CanonizeBNormOld) {
        Test([](const TString& line) {
            TString normalized = NCnorm::BNormOld(line);
            Cout << "\"" << line << "\" => \"" << normalized << "\"" << Endl;
        });
    }

    Y_UNIT_TEST(CanonizeANorm) {
        Test([](const TString& line) {
            TString normalized = NCnorm::ANorm(line);
            Cout << "\"" << line << "\" => \"" << normalized << "\"" << Endl;
        });
    }

    Y_UNIT_TEST(TestSpaces) {
        Test([](const TString& line) {
            const auto& normalizedUtf32 = TUtf32String::FromUtf8(NCnorm::Cnorm(line));
            UNIT_ASSERT(!FindIfPtr(normalizedUtf32, [](const wchar32 c) {
                return IsWhitespace(c) && c != ' ';
            }));

            for (size_t ind = 0; ind + 1 < normalizedUtf32.length(); ++ind) {
                UNIT_ASSERT(!(normalizedUtf32[ind] == ' ' && normalizedUtf32[ind + 1] == ' '));
            }
        });
    }

    Y_UNIT_TEST(TestForLackOfSpecialSymbols) {
        Test([](const TString& line) {
            const auto& normalizedUtf32 = TUtf32String::FromUtf8(NCnorm::Cnorm(line));
            UNIT_ASSERT(!FindIfPtr(normalizedUtf32, [](const wchar32 c) {
                return (IsPunct(c) || IsCombining(c)) && c != U'#';
            }));
        });
    }

    Y_UNIT_TEST(CmpQnormToCNorm) {
        size_t all = 0;
        size_t numberOfErrors = 0;
        Test([&](const TString& line) {
            const auto& queryNormalized = QNorm(line);
            const auto& consistentNormalized = NCnorm::Cnorm(line);
            if (queryNormalized != consistentNormalized) {
                ++numberOfErrors;
            }
            ++all;
        });
        UNIT_ASSERT_LE_C(100 * numberOfErrors, all, "Number of errors greater than 1%");
    }

    Y_UNIT_TEST(CNormIdempotenceTest) {
        Test([](const TString& line) {
            TString normalized = NCnorm::Cnorm(line);
            TString doubleNormalized = NCnorm::Cnorm(normalized);
            UNIT_ASSERT_STRINGS_EQUAL(normalized, doubleNormalized);
        });
    }

    Y_UNIT_TEST(BNormIdempotenceTest) {
        Test([](const TString& line) {
            TString normalized = NCnorm::BNorm(line);
            TString doubleNormalized = NCnorm::BNorm(normalized);
            UNIT_ASSERT_STRINGS_EQUAL(normalized, doubleNormalized);
        });
    }

    Y_UNIT_TEST(ONormIdempotenceTest) {
        Test([](const TString& line) {
            TString normalized = NCnorm::BNormOld(line);
            TString doubleNormalized = NCnorm::BNormOld(normalized);
            UNIT_ASSERT_STRINGS_EQUAL(normalized, doubleNormalized);
        });
    }

    Y_UNIT_TEST(ANormIdempotenceTest) {
        Test([](const TString& line) {
            TString normalized = NCnorm::ANorm(line);
            TString doubleNormalized = NCnorm::ANorm(normalized);
            UNIT_ASSERT_STRINGS_EQUAL(normalized, doubleNormalized);
        });
    }
}
