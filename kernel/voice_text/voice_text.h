#pragma once

#include <library/cpp/telfinder/telfinder.h>

#include <util/generic/fwd.h>

namespace NSc {
class TArray;
}

namespace NVoiceText {
    inline constexpr size_t MAX_ANSWER_LEN = 10000;
    inline constexpr size_t GOOD_ANSWER_LEN = 9000;
    inline constexpr size_t MIN_ANSWER_LEN = 100;

    bool TryVoiceTextArray(const NSc::TArray& utf8TextArray, const TTelFinder& telFinder, TString& voicedText,
        const TString& chunkDelimiter, const TString& sentenceDelimiter = ".", const TString& language = "ru", bool replaceXMLEscapedChars = true);
    TString GetPauseTag(size_t milliseconds);
    TPhoneSchemes ReadSchemesLocal(const TString& phoneSchemesFileContent);
    TUtf16String ReplaceXMLEscapedChars(const TUtf16String& text);
}

TPhoneSchemes ReadSchemes(const TString& phoneSchemesFile);


bool TryVoiceText(const TUtf16String& text, const TTelFinder& telFinder, TUtf16String& voicedText, const TString& language = "ru", bool replaceXMLEscapedChars = true);



class TTextVoicer {
public:
    bool TryVoiceText(const TUtf16String& text, TUtf16String& voicedText, const TString& language = "ru") const;
    bool TryVoiceTextArray(const NSc::TArray& utf8TextArray, TString& voicedText, const TString& chunkDelimiter,
        const TString& sentenceDelimiter = ".", const TString& language = "ru") const;

private:
    void InitTelFinder() const;

    mutable THolder<TTelFinder> TelFinder;
};
