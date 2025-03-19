#include <kernel/snippets/smartcut/smartcut.h>

#include <kernel/snippets/strhl/goodwrds.h>
#include <kernel/snippets/strhl/hilite_mark.h>

#include <kernel/qtree/richrequest/richnode.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/charset/wide.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NSnippets {
    Y_UNIT_TEST_SUITE(TSmartCutTests) {
        const TString LOREM = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, "
            "sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut "
            "enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut "
            "aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit "
            "in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur "
            "sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt "
            "mollit anim id est laborum.";

        TString CallNoQuerySymbol(const TString& utf8Source, size_t maxSymbols) {
            TUtf16String source = UTF8ToWide(utf8Source);
            TTextCuttingOptions options;
            options.MaximizeLen = true;
            TUtf16String res = SmartCutSymbol(source, maxSymbols, options);
            return WideToUTF8(res);
        }

        Y_UNIT_TEST(TestNoQuerySymbol) {
            UNIT_ASSERT_STRINGS_EQUAL(CallNoQuerySymbol("", 100), "");
            UNIT_ASSERT_STRINGS_EQUAL(CallNoQuerySymbol("Abc", 100), "Abc");
            UNIT_ASSERT_STRINGS_EQUAL(CallNoQuerySymbol("Abc", 2), "");
            UNIT_ASSERT_STRINGS_EQUAL(CallNoQuerySymbol(LOREM, 30),
                "Lorem ipsum dolor sit amet...");
            UNIT_ASSERT_STRINGS_EQUAL(CallNoQuerySymbol(LOREM, 100),
                "Lorem ipsum dolor sit amet, consectetur adipiscing elit, "
                "sed do eiusmod tempor incididunt ut...");
            UNIT_ASSERT_STRINGS_EQUAL(CallNoQuerySymbol(LOREM, LOREM.size()), LOREM);
        }

        TString CallNoQueryPixel(const TString& utf8Source, float maxRows,
            int pixelsInLine = 200, float fontSize = 13.0f)
        {
            TUtf16String source = UTF8ToWide(utf8Source);
            TTextCuttingOptions options;
            options.MaximizeLen = true;
            TUtf16String res = SmartCutPixelNoQuery(source, maxRows, pixelsInLine, fontSize, options);
            return WideToUTF8(res);
        }

        Y_UNIT_TEST(TestNoQueryPixel) {
            UNIT_ASSERT_STRINGS_EQUAL(CallNoQueryPixel("", 1.0), "");
            UNIT_ASSERT_STRINGS_EQUAL(CallNoQueryPixel("Abc", 1.0), "Abc");
            UNIT_ASSERT_STRINGS_EQUAL(CallNoQueryPixel("Abc", 0.01), "");
            UNIT_ASSERT_STRINGS_EQUAL(CallNoQueryPixel(LOREM, 0.5),
                "Lorem ipsum...");
            UNIT_ASSERT_STRINGS_EQUAL(CallNoQueryPixel(LOREM, 1.0),
                "Lorem ipsum dolor sit amet...");
            UNIT_ASSERT_STRINGS_EQUAL(CallNoQueryPixel(LOREM, 1.5),
                "Lorem ipsum dolor sit amet, consectetur...");
            UNIT_ASSERT_STRINGS_EQUAL(CallNoQueryPixel(LOREM, 2.0),
                "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed...");
            UNIT_ASSERT_STRINGS_EQUAL(CallNoQueryPixel(LOREM, 2.0, 100, 13.0),
                "Lorem ipsum dolor sit amet...");
            UNIT_ASSERT_STRINGS_EQUAL(CallNoQueryPixel(LOREM, 2.0, 100, 7.0),
                "Lorem ipsum dolor sit amet, consectetur adipiscing elit...");
        }

        TString CallQueryPixel(const TString& utf8Source, const TString& utf8Query,
            float maxRows, int pixelsInLine = 200, float fontSize = 13.0f)
        {
            TUtf16String source = UTF8ToWide(utf8Source);
            TUtf16String query = UTF8ToWide(utf8Query);
            TRichTreePtr richTree = CreateRichTree(query, TCreateTreeOptions(LANG_LAT));
            TInlineHighlighter ih;
            ih.AddRequest(*richTree->Root, &DEFAULT_MARKS);
            TTextCuttingOptions options;
            options.MaximizeLen = true;
            TUtf16String res = SmartCutPixelWithQuery(source, ih, maxRows, pixelsInLine, fontSize, options);
            return WideToUTF8(res);
        }

        Y_UNIT_TEST(TestQueryPixel) {
            UNIT_ASSERT_STRINGS_EQUAL(CallQueryPixel("", "", 1.0), "");
            UNIT_ASSERT_STRINGS_EQUAL(CallQueryPixel("", "query", 1.0), "");
            UNIT_ASSERT_STRINGS_EQUAL(CallQueryPixel("Abc", "abc", 1.0), "Abc");
            UNIT_ASSERT_STRINGS_EQUAL(CallQueryPixel("Abc", "abc", 0.01), "");
            UNIT_ASSERT_STRINGS_EQUAL(CallQueryPixel(LOREM, "not found", 0.875),
                "Lorem ipsum dolor sit amet...");
            UNIT_ASSERT_STRINGS_EQUAL(CallQueryPixel(LOREM, "lorem ipsum dolor sit", 0.875),
                "Lorem ipsum dolor sit...");
            UNIT_ASSERT_STRINGS_EQUAL(CallQueryPixel(LOREM, "", 2.0),
                "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed...");
            UNIT_ASSERT_STRINGS_EQUAL(CallQueryPixel(LOREM, "consectetur adipiscing", 2.0),
                "Lorem ipsum dolor sit amet, consectetur adipiscing elit...");
        }

        TString CallWithOptions(const TString& utf8Source, size_t maxSymbols,
            const TTextCuttingOptions& options)
        {
            TUtf16String source = UTF8ToWide(utf8Source);
            TUtf16String res = SmartCutSymbol(source, maxSymbols, options);
            return WideToUTF8(res);
        }

        Y_UNIT_TEST(TestOptions) {
            {
                TTextCuttingOptions options;
                options.MaximizeLen = true;
                UNIT_ASSERT_STRINGS_EQUAL(CallWithOptions(LOREM, 70, options),
                    "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do...");
                const TString url = "http://" "www.cumhuriyet.com.tr/video/video/127908/"
                    "Sultangazi_de_HDP_li_gence_linc_girisimi_.html";
                UNIT_ASSERT_STRINGS_EQUAL(CallWithOptions(url, url.size() - 10, options), "http...");
                UNIT_ASSERT_STRINGS_EQUAL(CallWithOptions(url, url.size(), options), url);
                const TString url2 = "F A C E B O O K http://www.facebook.com/";
                UNIT_ASSERT_STRINGS_EQUAL(CallWithOptions(url2, 32, options), "F A C E B O O K http...");
            }
            {
                TTextCuttingOptions options;
                options.MaximizeLen = false;
                UNIT_ASSERT_STRINGS_EQUAL(CallWithOptions(LOREM, 70, options),
                    "Lorem ipsum dolor sit amet, consectetur adipiscing elit...");
                options.Threshold = 0.9;
                UNIT_ASSERT_STRINGS_EQUAL(CallWithOptions(LOREM, 70, options),
                    "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do...");
            }
            {
                TTextCuttingOptions options;
                options.CutLastWord = false;
                options.MaximizeLen = true;
                UNIT_ASSERT_STRINGS_EQUAL(CallWithOptions("Abc def", 100, options), "Abc def");
                UNIT_ASSERT_STRINGS_EQUAL(CallWithOptions("Abc", 100, options), "Abc");
                UNIT_ASSERT_STRINGS_EQUAL(CallWithOptions(LOREM, LOREM.size(), options), LOREM);
            }
            {
                TTextCuttingOptions options;
                options.CutLastWord = true;
                options.MaximizeLen = true;
                UNIT_ASSERT_STRINGS_EQUAL(CallWithOptions("Abc def", 100, options), "Abc...");
                UNIT_ASSERT_STRINGS_EQUAL(CallWithOptions("Abc", 100, options), "");
                UNIT_ASSERT_STRINGS_EQUAL(CallWithOptions(LOREM, LOREM.size(), options),
                    LOREM.substr(0, LOREM.size() - 9) + "...");
            }
            {
                TTextCuttingOptions options;
                options.AddEllipsisToShortText = false;
                UNIT_ASSERT_STRINGS_EQUAL(CallWithOptions("", 100, options), "");
                UNIT_ASSERT_STRINGS_EQUAL(CallWithOptions("Abc", 100, options), "Abc");
                UNIT_ASSERT_STRINGS_EQUAL(CallWithOptions("Abc!", 100, options), "Abc!");
                UNIT_ASSERT_STRINGS_EQUAL(CallWithOptions("Abc def", 100, options), "Abc def");
                UNIT_ASSERT_STRINGS_EQUAL(CallWithOptions("Abc def.", 100, options), "Abc def.");
            }
            {
                TTextCuttingOptions options;
                options.AddEllipsisToShortText = true;
                UNIT_ASSERT_STRINGS_EQUAL(CallWithOptions("", 100, options), "");
                UNIT_ASSERT_STRINGS_EQUAL(CallWithOptions("Abc", 100, options), "Abc...");
                UNIT_ASSERT_STRINGS_EQUAL(CallWithOptions("Abc!", 100, options), "Abc!");
                UNIT_ASSERT_STRINGS_EQUAL(CallWithOptions("Abc def", 100, options), "Abc def...");
                UNIT_ASSERT_STRINGS_EQUAL(CallWithOptions("Abc def.", 100, options), "Abc def.");
            }
            {
                TTextCuttingOptions options;
                options.AddBoundaryEllipsis = false;
                UNIT_ASSERT_STRINGS_EQUAL(CallWithOptions(LOREM, 70, options),
                    "Lorem ipsum dolor sit amet, consectetur adipiscing elit");
            }
        }

        TString CallForFeatures(const TString& utf8Source, size_t maxSymbols) {
            TUtf16String source = UTF8ToWide(utf8Source);
            TUtf16String res = SmartCutSymbol(source, maxSymbols);
            return WideToUTF8(res);
        }

        Y_UNIT_TEST(TestFeatures) {
            UNIT_ASSERT_STRINGS_EQUAL(CallForFeatures(
                "Lorem ipsum dolor sit amet, consectetur adipiscing elit", 43),
                "Lorem ipsum dolor sit amet...");
            UNIT_ASSERT_STRINGS_EQUAL(CallForFeatures(
                "Пример пример «пример пример» пример пример", 34),
                "Пример пример «пример пример»...");
            UNIT_ASSERT_STRINGS_EQUAL(CallForFeatures(
                "Пример пример (пример пример) пример пример", 41),
                "Пример пример (пример пример)...");
            UNIT_ASSERT_STRINGS_EQUAL(CallForFeatures(
                "Пример пример, пример пример. Пример пример", 41),
                "Пример пример, пример пример.");
            UNIT_ASSERT_STRINGS_EQUAL(CallForFeatures(
                "Пример пример, пример пример пример пример", 41),
                "Пример пример, пример пример пример...");
        }
    }
}
