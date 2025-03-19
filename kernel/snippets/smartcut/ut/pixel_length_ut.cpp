#include <kernel/snippets/smartcut/pixel_length.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/charset/wide.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NSnippets {

Y_UNIT_TEST_SUITE(TPixelLengthTests) {

static void ParseHilitedText(const TWtringBuf& hilitedText, TUtf16String& plainText,
    TVector<TBoldSpan>& boldSpans)
{
    plainText.clear();
    boldSpans.clear();
    size_t openPos = 0;
    for (size_t i = 0; i < hilitedText.size(); ++i) {
        if (hilitedText[i] == '<') {
            openPos = plainText.size();
        } else if (hilitedText[i] == '>') {
            boldSpans.push_back(TBoldSpan(openPos, plainText.size()));
        } else {
            plainText.append(hilitedText[i]);
        }
    }
}

Y_UNIT_TEST(TestSimple) {
    const float fontSize = 13.0f;
    TUtf16String plainText;
    TVector<TBoldSpan> boldSpans;
    ParseHilitedText(UTF8ToWide("<YouTube> (МФА: [juːt(j)uːb]; от англ. you — "
        "«ты», «вы» и жарг. англ. tube — «труба», «телевизор»; произносится как "
        "«<Ютуб>» или «Ютьюб») — видеохостинг, предоставляющий пользователям "
        "услуги хранения, доставки и показа видео."), plainText, boldSpans);
    size_t length = plainText.size();
    UNIT_ASSERT_EQUAL(length, 221);
    TPixelLengthCalculator calculator(plainText, boldSpans);
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInPixels(0, length, fontSize), 1420.36, 1e-2);
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInPixels(100, 200, fontSize), 676.93, 1e-2);
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInRows(0, length, fontSize, 550), 1571.85 / 550, 1e-4);
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInRows(0, length, fontSize, 550, 0.36), 1689.82 / 550, 1e-4);
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInRows(0, length, fontSize, 550, 0.36, 10, 20), 1709.82 / 550, 1e-4);
}

Y_UNIT_TEST(TestLongWord) {
    const float fontSize = 16.0f;
    const size_t length = 200;
    TUtf16String plainText(length, 'A');
    TPixelLengthCalculator calculator(plainText, TVector<TBoldSpan>());
    const float A = calculator.CalcLengthInPixels(0, 1, fontSize);
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInPixels(0, length, fontSize), A * length, 1e-2);
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInPixels(50, 150, fontSize), A * 100, 1e-2);
    const int pixelsInLine = static_cast<int>(A * 100) - 1; // 99 A's fit in line
    const float As = A / pixelsInLine;
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInRows(0, length, fontSize, pixelsInLine), 2 + 2 * As, 1e-4);
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInRows(0, length, fontSize, pixelsInLine, 0.1), 3 + 2 * As, 1e-4);
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInRows(0, length, fontSize, pixelsInLine, 10.1), 13 + 2 * As, 1e-4);
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInRows(0, length, fontSize, pixelsInLine, 0, 2 * A, 3 * A), 2 + 7 * As, 1e-4);
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInRows(50, 150, fontSize, pixelsInLine), 1 + As, 1e-4);
    const int pixelsInLineWide = static_cast<int>(A * length) + 1; // whole word fits in line
    const float Aw = A / pixelsInLineWide;
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInRows(0, length, fontSize, pixelsInLineWide), length * Aw, 1e-4);
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInRows(0, length, fontSize, pixelsInLineWide, 0.1), 1 + length * Aw, 1e-4);
    const int pixelsInLineNarrow = static_cast<int>(A) + 1; // one A fits in line
    const float An = A / pixelsInLineNarrow;
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInRows(0, length, fontSize, pixelsInLineNarrow), (length - 1) + An, 1e-4);
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInRows(0, length, fontSize, 1), length, 1e-4);
}

Y_UNIT_TEST(TestLongWordInTheText) {
    const float fontSize = 16.0f;
    const size_t wordLength = 200;
    const size_t length = wordLength + 4;
    TUtf16String plainText = u"B " + TUtf16String(wordLength, 'A') + u" B";
    TPixelLengthCalculator calculator(plainText, TVector<TBoldSpan>());
    const float B = calculator.CalcLengthInPixels(0, 1, fontSize);
    const float S = calculator.CalcLengthInPixels(1, 2, fontSize);
    const float A = calculator.CalcLengthInPixels(2, 3, fontSize);
    const int pixelsInLine = static_cast<int>(A * 100) + 1; // 100 A's fit in line
    const float As = A / pixelsInLine;
    const float Bs = B / pixelsInLine;
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInRows(0, length, fontSize, pixelsInLine), 3 + Bs, 1e-4);
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInRows(2, length - 2, fontSize, pixelsInLine), 1 + 100 * As, 1e-4);
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInRows(0, length, fontSize, pixelsInLine, 0.5), 3 + Bs, 1e-4);
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInRows(0, length, fontSize, pixelsInLine, 0.3, pixelsInLine / 3.0, B), 3 + 2 * Bs, 1e-4);
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInRows(0, length, fontSize, pixelsInLine, 0.5, pixelsInLine / 2.0 - 1, B), 4 + 2 * Bs, 1e-4);
    const int pixelsInLineWide = static_cast<int>(A * wordLength) + 1; // whole word fits in line
    const float Aw = A / pixelsInLineWide;
    const float Bw = B / pixelsInLineWide;
    const float Sw = S / pixelsInLineWide;
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInRows(0, length, fontSize, pixelsInLineWide), 2 + Bw, 1e-4);
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInRows(0, length - 2, fontSize, pixelsInLineWide), 1 + wordLength * Aw, 1e-4);
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInRows(2, length - 2, fontSize, pixelsInLineWide), wordLength * Aw, 1e-4);
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInRows(2, length, fontSize, pixelsInLineWide), 1 + Bw, 1e-4);
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInRows(3, length, fontSize, pixelsInLineWide), 1 + Bw, 1e-4);
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInRows(4, length, fontSize, pixelsInLineWide), (wordLength - 2) * Aw + Sw + Bw, 1e-4);
}

Y_UNIT_TEST(TestBolder) {
    const float fontSize = 13.0f;
    TUtf16String plainText;
    TVector<TBoldSpan> boldSpans;
    ParseHilitedText(UTF8ToWide("<Курганский> <пограничный> <институт> <ФСБ> "
        "<России> — военный <институт> в городе <Кургане> <Курганской> "
        "области. На основании Постановления ЦК КПСС приказом Министра "
        "обороны СССР от 13 марта 1967 года создается <Курганское> высшее..."),
        plainText, boldSpans);
    size_t length = plainText.size();
    UNIT_ASSERT_EQUAL(length, 217);
    TPixelLengthCalculator calculator(plainText, boldSpans);
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInPixels(0, length, fontSize), 1492.10, 1e-2);
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInPixels(100, 200, fontSize), 682.15, 1e-2);
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInRows(0, length, fontSize, 300), 5.4487, 1e-4);
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInRows(0, length, fontSize, 15), 129.72, 1e-2);
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInRows(0, length, fontSize, 1), 217.00, 1e-2);
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInRows(0, length, fontSize, 1, 0, 1.1, 1.1), 217.00, 1e-2);
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInRows(0, length, fontSize, 0, 0, 1.1, 1.1), 217.00, 1e-2);
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInRows(0, length, fontSize, -10, 0, 1.1, 1.1), 217.00, 1e-2);
}

Y_UNIT_TEST(TestHyphenation) {
    const float fontSize = 13.0f;
    TUtf16String plainText = u"Abc def Abc def-Abc def";
    size_t length = plainText.size();
    TPixelLengthCalculator calculator(plainText, TVector<TBoldSpan>());
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInPixels(0, 7, fontSize), 44.08, 1e-2);
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInPixels(8, 16, fontSize), 48.41, 1e-2);
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInRows(0, 7, fontSize, 45), 44.08 / 45, 1e-3);
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInRows(0, 8, fontSize, 45), 1, 1e-3);
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInRows(0, 9, fontSize, 45), 1 + 8.67 / 45, 1e-3);
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInRows(8, 15, fontSize, 45), 44.08 / 45, 1e-3);
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInRows(8, 16, fontSize, 45), 1 + 22.40 / 45, 1e-3);
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInRows(8, 16, fontSize, 50), 48.41 / 50, 1e-3);
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInRows(8, 17, fontSize, 50), 1 + 8.67 / 50, 1e-3);
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInRows(0, length, fontSize, 45), 3 + 18.07 / 45, 1e-3);
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInRows(0, length, fontSize, 50), 2 + 44.08 / 50, 1e-3);
}

Y_UNIT_TEST(TestHyphenationBoldBoundary) {
    const float fontSize = 13.0f;
    TUtf16String plainText;
    TVector<TBoldSpan> boldSpans;
    ParseHilitedText(u"zzz zzz-zzz <zzz>-zzz zzz zzz-<zzz> zzz <zzz>-<zzz>",
        plainText, boldSpans);
    size_t length = plainText.size();
    TPixelLengthCalculator calculator(plainText, boldSpans);
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInPixels(length - 8, length, fontSize), 46.941, 1e-3); // " <zzz>-<zzz>"
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInPixels(0, length, fontSize), 253.487, 1e-3);
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInRows(0, 11, fontSize, 47), 1.4149, 1e-4); // "zzz zzz-|zzz"
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInRows(8, 19, fontSize, 47), 1.9219, 1e-4); // "zzz |<zzz>-zzz"
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInRows(20, 31, fontSize, 47), 1.9219, 1e-4); // "zzz |zzz-<zzz>"
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInRows(32, 43, fontSize, 47), 1.9219, 1e-4); // "zzz |<zzz>-<zzz>"
    // "zzz zzz-|zzz |<zzz>-zzz |zzz |zzz-<zzz> |zzz |<zzz>-<zzz>"
    UNIT_ASSERT_DOUBLES_EQUAL(calculator.CalcLengthInRows(0, length, fontSize, 47), 6.9219, 1e-4);
}

Y_UNIT_TEST(TestMultiToken) {
    const float fontSize = 16.0f;
    TUtf16String plainText;
    TVector<TBoldSpan> boldSpans;
    ParseHilitedText(u"test <test> <t><e><s><t>", plainText, boldSpans);
    TPixelLengthCalculator calculator(plainText, boldSpans);
    float regular = calculator.CalcLengthInPixels(0, 4, fontSize);
    float bold = calculator.CalcLengthInPixels(5, 9, fontSize);
    float multiBold = calculator.CalcLengthInPixels(10, 14, fontSize);
    UNIT_ASSERT(regular + 1e-2 < bold);
    UNIT_ASSERT_DOUBLES_EQUAL(bold, multiBold, 1e-2);
}

Y_UNIT_TEST(TestGetStringPixelLength) {
    float length = GetStringPixelLength(u"test!@#$%", 18.0f);
    UNIT_ASSERT_DOUBLES_EQUAL(length, 88.31, 1e-2);
}

}

}
