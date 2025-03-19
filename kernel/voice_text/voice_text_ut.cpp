#include "voice_text.h"

#include <kernel/text_marks/hilite.h>

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/telfinder/telfinder.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/string.h>
#include <util/memory/pool.h>

#include <initializer_list>
#include <iterator>

namespace {

    template<class Ctr>
    const NSc::TArray& MakeTextArray(const Ctr& strings) {
        static TMemoryPool pool(NVoiceText::MAX_ANSWER_LEN * 2);
        static NSc::TArray dst(&pool);
        dst.clear();
        dst.AppendAll(std::begin(strings), std::end(strings));
        return dst;
    }

    void SentencizeString(TString& trg, const TString::value_type& sentenceDelimiter, size_t sentenceLen) {
        for (size_t i = sentenceLen; i + 1 < trg.size(); i += sentenceLen) {
            trg[i] = sentenceDelimiter;
            trg[i + 1] = ' ';
        }
    }

}

Y_UNIT_TEST_SUITE(ReplaceXMLEscapedCharsTests) {
    Y_UNIT_TEST(TextWithoutEscapedChars) {
        TUtf16String src = u"Приветствую мир";
        UNIT_ASSERT(NVoiceText::ReplaceXMLEscapedChars(src) == src);
        src = u"А &nbsp Б";
        UNIT_ASSERT(NVoiceText::ReplaceXMLEscapedChars(src) == src);
        src = u"Слово1 nbsp; слово2";
        UNIT_ASSERT(NVoiceText::ReplaceXMLEscapedChars(src) == src);
        src = u"Начало&gt";
        UNIT_ASSERT(NVoiceText::ReplaceXMLEscapedChars(src) == src);
        src = u"apos;Конец";
        UNIT_ASSERT(NVoiceText::ReplaceXMLEscapedChars(src) == src);
    }

    Y_UNIT_TEST(TextWithEscapedChars) {
        UNIT_ASSERT(NVoiceText::ReplaceXMLEscapedChars(u"Приветствую&nbsp;мир") == u"Приветствую\u00A0мир");
        UNIT_ASSERT(NVoiceText::ReplaceXMLEscapedChars(u"1 &lt; 2") == u"1 < 2");
        UNIT_ASSERT(NVoiceText::ReplaceXMLEscapedChars(u"1 &gt; 2") == u"1 > 2");
        UNIT_ASSERT(NVoiceText::ReplaceXMLEscapedChars(u"Слово1 &amp; слово2") == u"Слово1 & слово2");
        UNIT_ASSERT(NVoiceText::ReplaceXMLEscapedChars(u"&lt;&quot;&quot;&apos;&amp;&nbsp;&gt;") == u"<\"\"'&\u00A0>");
    }
}


Y_UNIT_TEST_SUITE(TextArrayTests) {
    Y_UNIT_TEST(OneShortChunk) {
        const auto sentence1 = "AA BBB CCCC DDDDD";
        const auto chunkDelimiter = "\n";
        const auto sentences = {sentence1};
        auto& src = MakeTextArray(sentences);
        TString dst;
        UNIT_ASSERT(NVoiceText::TryVoiceTextArray(src, TTelFinder(), dst, chunkDelimiter, ".", "en"));
        UNIT_ASSERT_STRINGS_EQUAL(dst, sentence1);
    }

    Y_UNIT_TEST(TwoShortChunks) {
        const auto sentence1 = "AA BBB CCCC DDDDD";
        const auto sentence2 = "ZZ BBB CCCC DDDDD";
        const auto chunkDelimiter = "\n";
        const auto sentenceDelimiter = ".";
        const auto sentences = {sentence1, sentence2};
        auto& src = MakeTextArray(sentences);
        TString dst;
        UNIT_ASSERT(NVoiceText::TryVoiceTextArray(src, TTelFinder(), dst, chunkDelimiter, sentenceDelimiter, "en"));
        UNIT_ASSERT_STRINGS_EQUAL(dst, TString(sentence1) + sentenceDelimiter + chunkDelimiter + sentence2);
    }

    Y_UNIT_TEST(OneLongChunk) {
        auto chunk = TString(NVoiceText::MAX_ANSWER_LEN, 'Y');
        const size_t sentenceLen = NVoiceText::MIN_ANSWER_LEN + 1;
        constexpr TString::value_type sentenceDelimiter = '.';
        SentencizeString(chunk, sentenceDelimiter, sentenceLen);
        const auto chunkDelimiter = "\n";
        const auto sentences = {chunk};
        auto& src = MakeTextArray(sentences);
        TString dst;
        UNIT_ASSERT(NVoiceText::TryVoiceTextArray(src, TTelFinder(), dst, chunkDelimiter, ".", "en"));
        UNIT_ASSERT_STRINGS_EQUAL(dst, chunk);
    }

    Y_UNIT_TEST(OneTooLongChunk) {
        auto chunk1 = TString(NVoiceText::MAX_ANSWER_LEN + 1, 'Y');
        const size_t sentenceLen = NVoiceText::MIN_ANSWER_LEN + 1;
        constexpr TString::value_type sentenceDelimiter = '.';
        SentencizeString(chunk1, sentenceDelimiter, sentenceLen);
        const auto chunkDelimiter = "\n";
        const auto chunks = {chunk1};
        auto& src = MakeTextArray(chunks);
        const TString::value_type sentenceDelimiterStr[2] = {sentenceDelimiter, '\0'};
        TString dst;
        UNIT_ASSERT(NVoiceText::TryVoiceTextArray(src, TTelFinder(), dst, chunkDelimiter, sentenceDelimiterStr, "en"));
        UNIT_ASSERT(dst.size() == NVoiceText::MAX_ANSWER_LEN - sentenceLen);
    }

    Y_UNIT_TEST(ManyShortChunks) {
        constexpr size_t chunkLen = NVoiceText::MIN_ANSWER_LEN + 1;
        const auto chunkSample = TString(chunkLen, 'Y');
        constexpr size_t numChunks = NVoiceText::MAX_ANSWER_LEN / chunkLen + 1;
        TVector<TString> chunks(numChunks, chunkSample);
        auto& src = MakeTextArray(chunks);
        const TString chunkDelimiter = "\n";
        const TString sentenceDelimiter = ".";
        TString dst;
        UNIT_ASSERT(NVoiceText::TryVoiceTextArray(src, TTelFinder(), dst, chunkDelimiter, sentenceDelimiter, "en"));
        const size_t answerChunkLen = chunkLen + sentenceDelimiter.size() + chunkDelimiter.size();
        const size_t expectedSize = ((NVoiceText::MAX_ANSWER_LEN / answerChunkLen - 1) * answerChunkLen) + chunkLen;
        UNIT_ASSERT(dst.size() == expectedSize);
    }
}


Y_UNIT_TEST_SUITE(SingleTextTests) {
    Y_UNIT_TEST(BracketRemovalSimple) {
        const TString mainText = "Abcdef";
        const TString bracketedText = "fedcba";
        const TString src = mainText + "(" + bracketedText + ")";
        TUtf16String dst;
        UNIT_ASSERT(TryVoiceText(UTF8ToWide(src), TTelFinder(), dst));
        UNIT_ASSERT_STRINGS_EQUAL(WideToUTF8(dst), mainText);
    }

    Y_UNIT_TEST(BracketRemovalNested) {
        const TString mainText = "Abcdef";
        const TString bracketedText1 = "fedcba";
        const TString bracketedText2 = "zyxwvu";
        const TString src = mainText + "(" + bracketedText1 + "(" + bracketedText2 + ")" + " " + ")";
        TUtf16String dst;
        UNIT_ASSERT(TryVoiceText(UTF8ToWide(src), TTelFinder(), dst, "en"));
        UNIT_ASSERT_STRINGS_EQUAL(WideToUTF8(dst), mainText);
    }

    Y_UNIT_TEST(BracketRemovalDiffNested) {
        const TString mainText = "Abcdef";
        const TString bracketedText1 = "fedcba";
        const TString bracketedText2 = "zyxwvu";
        const TString src = mainText + "(" + bracketedText1 + "{" + bracketedText2 + "}" + " " + ")";
        TUtf16String dst;
        UNIT_ASSERT(TryVoiceText(UTF8ToWide(src), TTelFinder(), dst, "en"));
        UNIT_ASSERT_STRINGS_EQUAL(WideToUTF8(dst), mainText);
    }

    Y_UNIT_TEST(BracketRemovalDiffNestedBuggy) {
        const TString mainText = "Abcdef";
        const TString bracketedText1 = "fedcba";
        const TString bracketedText2 = "zyxwvu";
        const TString src = mainText + "(" + bracketedText1 + "{" + bracketedText2 + ")" + " " + "}";
        TUtf16String dst;
        UNIT_ASSERT(TryVoiceText(UTF8ToWide(src), TTelFinder(), dst, "en"));
        UNIT_ASSERT_STRINGS_EQUAL(WideToUTF8(dst), mainText);
    }

    Y_UNIT_TEST(BracketRemovalTruncated) {
        const TString mainText = "Abcdef";
        const TString truncatedText = "fedcba";
        const TString src = mainText + ")" + truncatedText;
        TUtf16String dst;
        UNIT_ASSERT(TryVoiceText(UTF8ToWide(src), TTelFinder(), dst, "en"));
        UNIT_ASSERT_STRINGS_EQUAL(WideToUTF8(dst), mainText);
    }

    Y_UNIT_TEST(BracketRemovalUnfinished) {
        const TString mainText = "Abcdef";
        const TString truncatedText = "fedcba";
        const TString src = mainText + "(" + truncatedText;
        TUtf16String dst;
        UNIT_ASSERT(TryVoiceText(UTF8ToWide(src), TTelFinder(), dst, "en"));
        UNIT_ASSERT_STRINGS_EQUAL(WideToUTF8(dst), mainText);
    }

    Y_UNIT_TEST(BracketRemovalSentences) {
        const TString sentenceDelimiter = ". ";
        const TString sentence1 = "Это первое предложение";
        const TString bracketedText1 = "фывапр";
        const TString sentence2 = "А это - второе";
        const TString bracketedText2 = "йцукен";
        const TString src = sentence1 + "(" + bracketedText1 + ")" + sentenceDelimiter + sentence2 + "{" + bracketedText2 + "}" + sentenceDelimiter;
        TUtf16String dst;
        UNIT_ASSERT(TryVoiceText(UTF8ToWide(src), TTelFinder(), dst, "ru"));
        UNIT_ASSERT_STRINGS_EQUAL(WideToUTF8(dst), sentence1 + sentenceDelimiter + sentence2 + sentenceDelimiter);
    }

    Y_UNIT_TEST(BracketRemovalSentenceEndInside) {
        const TString sentenceDelimiter = ". ";
        const TString sentence1 = "Это первое предложение";
        const TString sentence2 = "А это - внутри скобок";
        const TString sentence3 = "И это - внутри скобок";
        const TString src = sentence1 + "(" + sentence2 + sentenceDelimiter + sentence3 + ")";
        TUtf16String dst;
        UNIT_ASSERT(TryVoiceText(UTF8ToWide(src), TTelFinder(), dst, "ru"));
        UNIT_ASSERT_STRINGS_EQUAL(WideToUTF8(dst), sentence1);
    }

    Y_UNIT_TEST(HiliteRemoval) {
        const TString sentenceDelimiter = ". ";
        const TString src= TString("Это текст ") + NTextMarks::HILITE_OPEN + "с хайлайтами" + NTextMarks::HILITE_CLOSE;
        TUtf16String dst;
        UNIT_ASSERT(TryVoiceText(UTF8ToWide(src), TTelFinder(), dst, "ru"));
        UNIT_ASSERT_STRINGS_EQUAL(WideToUTF8(dst), "Это текст с хайлайтами");
    }

    Y_UNIT_TEST(VoiceRussianAccents) {
        TTelFinder telFinder;
        TUtf16String dst;
        UNIT_ASSERT(TryVoiceText(u"АлфавИт", telFinder, dst, "ru"));
        UNIT_ASSERT_STRINGS_EQUAL(WideToUTF8(dst), "Алфав+ит");
        UNIT_ASSERT(TryVoiceText(u"ДОгма", telFinder, dst, "ru"));
        UNIT_ASSERT_STRINGS_EQUAL(WideToUTF8(dst), "Д+огма");
        UNIT_ASSERT(TryVoiceText(u"понялА", telFinder, dst, "ru"));
        UNIT_ASSERT_STRINGS_EQUAL(WideToUTF8(dst), "понял+а");
        UNIT_ASSERT(TryVoiceText(u"АаАаА", telFinder, dst, "ru"));
        UNIT_ASSERT_STRINGS_EQUAL(WideToUTF8(dst), "Аа+аа+а");
        UNIT_ASSERT(TryVoiceText(u"Алиса", telFinder, dst, "ru"));
        UNIT_ASSERT_STRINGS_EQUAL(WideToUTF8(dst), "Алиса");
        UNIT_ASSERT(TryVoiceText(u"НИИЧаВо", telFinder, dst, "ru"));
        UNIT_ASSERT_STRINGS_EQUAL(WideToUTF8(dst), "НИИЧаВо");
        UNIT_ASSERT(TryVoiceText(u"НИИ ЧИаВо", telFinder, dst, "ru"));
        UNIT_ASSERT_STRINGS_EQUAL(WideToUTF8(dst), "НИИ Ч+иаВо");
        UNIT_ASSERT(TryVoiceText(u"НИИЧИаВо", telFinder, dst, "ru"));
        UNIT_ASSERT_STRINGS_EQUAL(WideToUTF8(dst), "НИИЧИаВо");
    }

    Y_UNIT_TEST(XMLEscapedChars) {
        TTelFinder telFinder;
        TUtf16String dst;
        UNIT_ASSERT(TryVoiceText(u"Привет&nbsp;мир", telFinder, dst, "ru"));
        UNIT_ASSERT_STRINGS_EQUAL(WideToUTF8(dst), "Привет\u00A0мир");
        UNIT_ASSERT(TryVoiceText(u"1 &lt; 3 &gt; 2", telFinder, dst, "ru"));
        UNIT_ASSERT_STRINGS_EQUAL(WideToUTF8(dst), "1 < 3 > 2");
    }

    Y_UNIT_TEST(XMLEscapedCharsTurnedOff) {
        TTelFinder telFinder;
        TUtf16String dst;
        UNIT_ASSERT(TryVoiceText(u"Привет&nbsp;мир", telFinder, dst, "ru", false));
        UNIT_ASSERT_STRINGS_EQUAL(WideToUTF8(dst), "Привет&nbsp;мир");
        UNIT_ASSERT(TryVoiceText(u"1 &lt; 3 &gt; 2", telFinder, dst, "ru", false));
        UNIT_ASSERT_STRINGS_EQUAL(WideToUTF8(dst), "1 &lt; 3 &gt; 2");
    }

    Y_UNIT_TEST(ChangeRussianICharacter) {
        TTelFinder telFinder;
        TUtf16String dst;
        UNIT_ASSERT(TryVoiceText(u"Йога Бэй", telFinder, dst, "ru"));
        UNIT_ASSERT_STRINGS_EQUAL(WideToUTF8(dst), "Йога Бэй");
    }
}
