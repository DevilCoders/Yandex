#include <kernel/lemmer/core/language.h>

#include <util/generic/strbuf.h>
#include <util/generic/ptr.h>


static const char TAB = '\t';


int main(int /* argc */, const char* /* argv */[]) {
    TString line;
    while (Cin.ReadLine(line)) {
        TStringBuf lineBuf(line);
        TStringBuf word = lineBuf.NextTok(TAB);
        TUtf16String wideWord = UTF8ToWide(word);

        TWLemmaArray lemmata;
        TUtf16String form;
        TString lemma;
        NLemmer::AnalyzeWord(
            wideWord.data(), wideWord.size(),
            lemmata,
            TLangMask(LANG_RUS, LANG_ENG) /* LI_BASIC_LANGUAGES */
        );
        for (const auto& lemmaInfo: lemmata) {
            lemma = WideToUTF8(lemmaInfo.GetText());
            THolder<NLemmer::TFormGenerator> generator = lemmaInfo.Generator();

            for (NLemmer::TFormGenerator& it = *generator; it.IsValid(); ++it) {
                it->ConstructText(form);
                Cout << lemma << '\t' << WideToUTF8(form) << Endl;
            }
        }
    }
}
