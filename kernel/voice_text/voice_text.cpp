#include "voice_text.h"

#include <kernel/text_marks/hilite.h>

#include <library/cpp/deprecated/iter/vector.h>
#include <kernel/lemmer/core/language.h>
#include <library/cpp/resource/resource.h>
#include <library/cpp/scheme/scheme.h>
#include <library/cpp/telfinder/phone_collect.h>
#include <library/cpp/telfinder/simple_tokenizer.h>
#include <library/cpp/telfinder/tel_schemes.h>
#include <library/cpp/token/charfilter.h>
#include <library/cpp/tokenizer/split.h>

#include <util/charset/unidata.h>
#include <util/charset/wide.h>
#include <util/generic/hash_set.h>
#include <util/string/builder.h>
#include <util/string/subst.h>

static const wchar16 DEGREE_SIGN = L'°';

static const float MAX_SHRINK_RATIO = .2f;

static const THashSet<wchar16> SPECIAL_CHARACTERS = {
    ' ',
    '(',
    ')',
    '{',
    '}',
    ',',
    '"',
    '\'',
    '-',
    0x00AD, // soft hyphen
    ';',
    ':',
    '+',
    DEGREE_SIGN,
    L'°',
    L'±',
    L'…',
    L'№',
    L'‘',
    '\'',
    '`',
    L'●',
    L'✔',
    0x00A0, // non-breakable space
    L'à',
    L'á',
    L'è',
    L'é',
    L'ò',
    L'ó',
    L'À',
    L'Á',
    L'È',
    L'É',
    L'Ò',
    L'Ó',
    L'’',
    0x0301, // Combining Acute Accent
    0x0306, // Combining Breve
    0x200B, // Zero Width Space
    0x030A // Combining Ring Above
};

static const THashSet<wchar16> UPPER_RUSSIAN_VOWELS = {
    0x0401, // 'Ё'
    0x0410, // 'А'
    0x0415, // 'Е'
    0x0418, // 'И'
    0x041E, // 'О'
    0x0423, // 'У'
    0x042B, // 'Ы'
    0x042D, // 'Э'
    0x042E, // 'Ю'
    0x042F, // 'Я'
};

static const THashSet<wchar16> RUSSIAN_I_CHARACTERS = {
    0x0438, // 'и'
    0x0418, // 'И'
};

static const TUtf16String TTS_ACCENT_MARK = u"+";

namespace NVoiceText {
    const THashSet<char> SENTENCE_ENDINGS = {'.', '!', '?'};

    TString MakeFullDelimiter(const TString& currentText, const TString& chunkDelimiter, const TString& sentenceDelimiter) {
        if (currentText.empty()) {
            return "";
        }
        return SENTENCE_ENDINGS.contains(currentText[currentText.size() - 1])
            ? chunkDelimiter
            : sentenceDelimiter + chunkDelimiter;
    }

    bool TryVoiceTextArray(const NSc::TArray& utf8TextArray, const TTelFinder& telFinder, TString& voicedText,
        const TString& chunkDelimiter, const TString& sentenceDelimiter, const TString& language, bool replaceXMLEscapedChars) {
        TStringBuilder answer;
        size_t answerLen = 0;
        TVector<TUtf16String> sentences;

        // add whole list items while they fit
        for (const NSc::TValue& textItem: utf8TextArray) {
            TUtf16String wideFragment;
            if (!TryVoiceText(UTF8ToWide(textItem.GetString()), telFinder, wideFragment, language, replaceXMLEscapedChars)) {
                continue;
            }
            const TString fragment = WideToUTF8(wideFragment);
            size_t fragmentLen = wideFragment.size();
            const TString delimiter = MakeFullDelimiter(answer, chunkDelimiter, sentenceDelimiter);
            const size_t delimiterLen = GetNumberOfUTF8Chars(delimiter);

            if (answerLen + delimiterLen + fragmentLen > MAX_ANSWER_LEN) {
                // do not add the next list item
                if (answerLen < GOOD_ANSWER_LEN) {
                    // but add part of it (full sentences, see the cycle below)
                    sentences = SplitIntoSentences(UTF8ToWide(textItem.GetString()));
                }
                break;
            }

            answer << delimiter;
            answerLen += delimiterLen;
            answer << fragment;
            answerLen += fragmentLen;
        }

        // add some sentences of the last list item that did not wholly fit
        bool isFirstSent = true;
        for (const TUtf16String& sent: sentences) {
            if (answerLen >= MIN_ANSWER_LEN && answerLen + sent.size() > MAX_ANSWER_LEN) { // sent.Size() is inaccurate
                break;
            }
            if (!answer.empty()) {
                if (isFirstSent) {
                    const TString delimiter = MakeFullDelimiter(answer, chunkDelimiter, sentenceDelimiter);
                    const size_t delimiterLen = GetNumberOfUTF8Chars(delimiter);
                    answer << delimiter;
                    answerLen += delimiterLen;
                    isFirstSent = false;
                } else {
                    answer << " ";
                    answerLen += 1;
                }
            }
            answer << WideToUTF8(StripRight(sent));
            answerLen += sent.size();
        }
        if (answer.empty()) {
            return false;
        }
        voicedText = std::move(answer);
        return true;
    }

    // https://doc.yandex-team.ru/alice/alice-dev-guide/concepts/scenario-nlg.html#scenario-nlg__section_ucb_m45_vcb
    TString GetPauseTag(size_t ms) {
        return TStringBuilder() << " sil <[ " << ms << " ]> ";
    }

    TPhoneSchemes ReadSchemesLocal(const TString& phoneSchemesFileContent) {
        TPhoneSchemes result;
        TStringInput sin(phoneSchemesFileContent);
        result.ReadSchemes(sin);
        return result;
    }

    TUtf16String ReplaceXMLEscapedChars(const TUtf16String& text) {
        static const wchar16 XMLEscapedFormStart = '&';
        static const std::pair<const wchar16*, wchar16> XMLEscapedForms[] = {
            {u"&nbsp;", 0x00A0},
            {u"&amp;", '&'},
            {u"&lt;", '<'},
            {u"&gt;", '>'},
            {u"&quot;", '"'},
            {u"&apos;", '\''}};
        TUtf16String result;
        for (size_t i = 0; i < text.size(); ++i) {
            if (text[i] == XMLEscapedFormStart) {
                bool matched = false;
                for (const std::pair<const wchar16*, wchar16>& form : XMLEscapedForms) {
                    size_t length = CountWideChars(form.first);
                    if (i + length <= text.size() && text.substr(i, length) == form.first) {
                        matched = true;
                        i += length - 1;
                        result += form.second;
                        break;
                    }
                }
                if (matched)
                    continue;
            }
            result += text[i];
        }
        return result;
    }
}

TUtf16String ReplaceDashes(const TUtf16String& text) {
    TUtf16String result;
    for (const wchar16 c : text) {
        if (IsDash(c))
            result.push_back(wchar16('-'));
        else
            result.push_back(c);
    }
    return result;
}

TPhoneSchemes ReadSchemes(const TString& phoneSchemesFile) {
    TPhoneSchemes result;
    TFileInput fin(phoneSchemesFile);
    result.ReadSchemes(fin);
    return result;
}

bool CheckPhoneCharacters(const TUtf16String& text) {
    for (const wchar16 c : text) {
        if (!SPECIAL_CHARACTERS.contains(c) && !IsCommonDigit(c)) {
            return false;
        }
    }
    return true;
}

bool IsPhone(TUtf16String text, const TTelFinder& telFinder) {
    if (!CheckPhoneCharacters(text)) {
        return false;
    }
    text = ReplaceDashes(text);
    TVector<TToken> tokens = TSimpleTokenizer::BuildTokens(text);
    TFoundPhones foundPhones;
    TPhoneCollector coltr(foundPhones);
    telFinder.FindPhones(NIter::TVectorIterator<TToken>(tokens), coltr);
    if (foundPhones.size() != 1) {
        return false;
    }
    return true;
}

bool TryRetrieveTextWithoutBrackets(const TUtf16String& text, TUtf16String& voicedText) {
    voicedText = TUtf16String();
    int balance = 0;
    for (const wchar16 c : text) {
        if (c == '(' || c == '{') {
            balance += 1;
        } else if (c == ')' || c == '}') {
            balance -= 1;
        } else if (balance == 0) {
            voicedText += c;
        }

        if (balance < 0) {
            return false;
        }
    }
    if (balance > 0) {
        return false;
    }
    return true;
}

inline bool IsLatin(wchar16 c) {
    return c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z';
}

bool CheckCharacters(const TUtf16String& voicedText, const TString& language) {
    bool hasLetters = false;
    bool hasDigits = false;
    size_t letterFollowedByUpperDigitCount = 0;
    const TLangMask langMask = LanguageByName(language);
    static const TLangMask defaultLangMask = TLangMask();

    size_t textLength = voicedText.size();
    for (size_t i = 0; i < textLength; ++i) {
        wchar16 c = voicedText[i];
        if (!SPECIAL_CHARACTERS.contains(c) && !IsNumeric(c) && !IsRomanDigit(c)) {
            TLangMask charMask = NLemmer::GetCharInfoAlpha(c);
            const bool isRightLang = (charMask & langMask) != defaultLangMask;
            if (isRightLang || IsLatin(c)) {
                hasLetters = true;
            }
        }
        if (IsNumeric(c)) {
            hasDigits = true;
            if (i > 0 && IsUpper(voicedText[i - 1]) && IsLatin(voicedText[i - 1])) {
                ++letterFollowedByUpperDigitCount;
            }
        }
        if (IsRomanDigit(c)) {
            hasDigits = true;
        }
    }
    if (letterFollowedByUpperDigitCount > 1) { // probably a chemical formula
        return false;
    }
    return hasLetters || hasDigits;
}

bool IsLowerCyrillic(wchar16 c) {
    return (c >= 0x0430 && c <= 0x044F) || c == 0x451;
}

bool IsUpperCyrillic(wchar16 c) {
    return (c >= 0x0410 && c <= 0x042F) || c == 0x401;
}

void VoiceRussianAccents(TUtf16String& text) {
    for (size_t i = 1; i < text.size(); ++i) {
        if (UPPER_RUSSIAN_VOWELS.contains(text[i])) {
            bool afterLower = IsLowerCyrillic(text[i - 1]);
            bool isSecondInWord = i == 1 || (i > 1 && IsSpace(text[i - 2]));
            bool afterUpperBeforeLower = IsUpperCyrillic(text[i - 1]) && i < text.size() - 1 && IsLowerCyrillic(text[i + 1]);
            if (afterLower || isSecondInWord && afterUpperBeforeLower) {
                text[i] = ToLower(text[i]);
                text.insert(i, TTS_ACCENT_MARK);
                ++i;
            }
        }
    }
}

void ChangeRussianICharacter(TUtf16String& text) {
    if (text.size() == 0) {
        return;
    }
    for (size_t i = 0; i < text.size() - 1; ++i) {
        if (RUSSIAN_I_CHARACTERS.contains(text[i]) && (text[i + 1] == 0x0306)) {  // 'и' with emphasis
            text[i] = text[i] + 1;  // change to 'й'
        }
    }
}

void RemoveAccents(TUtf16String& text) {
    TWideToken token;
    token.Token = text.c_str();
    token.Leng = text.size();
    token.SubTokens.push_back(0, text.size());
    TCharFilter<TAccents> filter(token.Leng);
    const TWideToken& result = filter.Filter(token);
    text = result.Text();
}

void RemoveHilite(TUtf16String& text) {
    SubstGlobal(text, NTextMarks::HILITE_OPEN_WIDE, TUtf16String());
    SubstGlobal(text, NTextMarks::HILITE_CLOSE_WIDE, TUtf16String());
}

bool TryVoiceText(const TUtf16String& text, const TTelFinder& telFinder, TUtf16String& voicedText, const TString& language, bool replaceXMLEscapedChars) {
    voicedText.clear();

    TUtf16String correctedText = replaceXMLEscapedChars ? NVoiceText::ReplaceXMLEscapedChars(text) : text;
    RemoveHilite(correctedText);

    if (IsPhone(correctedText, telFinder)) {
        voicedText = text;
        return true;
    }

    TUtf16String buffer;
    TryRetrieveTextWithoutBrackets(correctedText, buffer);
    TVector<TUtf16String> sentences = SplitIntoSentences(buffer);
    for (const auto& sentence : sentences) {
        if (!CheckCharacters(sentence, language)) {
            break;
        }
        voicedText.append(sentence);
    }
    VoiceRussianAccents(voicedText);
    ChangeRussianICharacter(voicedText);
    RemoveAccents(voicedText);
    if (voicedText.size() < MAX_SHRINK_RATIO * text.size()) {
        voicedText.clear();
    }
    return !!voicedText;
}

bool TTextVoicer::TryVoiceText(const TUtf16String& text, TUtf16String& voicedText, const TString& language) const {
    if (!TelFinder) {
        InitTelFinder();
    }
    return ::TryVoiceText(text, *TelFinder, voicedText, language);
}

bool TTextVoicer::TryVoiceTextArray(const NSc::TArray& utf8TextArray, TString& voicedText, const TString& chunkDelimiter,
    const TString& sentenceDelimiter, const TString& language) const
{
    if (!TelFinder) {
        InitTelFinder();
    }
    return NVoiceText::TryVoiceTextArray(utf8TextArray, *TelFinder, voicedText, chunkDelimiter, sentenceDelimiter, language);
}

void TTextVoicer::InitTelFinder() const {
    TString phoneSchemesFileContent = NResource::Find("phone_schemes");
    TelFinder.Reset(new TTelFinder(NVoiceText::ReadSchemesLocal(phoneSchemesFileContent)));
}
