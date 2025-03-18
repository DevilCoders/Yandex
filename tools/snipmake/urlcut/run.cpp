#include "xml_stuff.h"

#include <kernel/qtree/richrequest/printrichnode.h>
#include <kernel/qtree/richrequest/richnode.h>
#include <kernel/snippets/urlcut/url_wanderer.h>
#include <kernel/snippets/urlcut/urlcut.h>

#include <library/cpp/charset/wide.h>
#include <library/cpp/getopt/opt.h>

TRichTreePtr GetRichTree(TString q, ELanguage lang)
{
    return (DeserializeRichTree(DecodeRichTreeBase64(GetQtree(q, lang))));
}

int main(int argc, char** argv)
{
    ELanguage lang = LANG_UNK;
    Opt opt(argc, argv, "l:");
    int optlet;
    while ((optlet = opt.Get()) != EOF) {
        switch (optlet) {
            case 'l':
                lang = LanguageByName(opt.Arg);
                break;
            default:
                break;

        }
    }

    if (lang == LANG_UNK) {
        lang = LANG_RUS;
        Cerr << "Default language is " << NameByLanguage(lang) << Endl;
    }

    TString url;
    TUtf16String cUrl;
    TUtf16String q;

    while (Cin.ReadLine(url) && Cin.ReadLine(q)) {
        try
        {
            TRichTreePtr rTreePtr = GetRichTree(WideToUTF8(q), lang);
            NUrlCutter::TRichTreeWanderer rTreeWanderer(rTreePtr);
            if (rTreePtr.Get() != nullptr) {
                Cout << "Query: " << q << Endl;
                Cout << "RTrie: " << PrintRichRequest(rTreePtr.Get()) << Endl;
                Cout << "Url:   " << url << Endl;

                NUrlCutter::THilitedString hilitedUrl =
                    NUrlCutter::HiliteAndCutUrl(url, 50, 40, rTreeWanderer, lang);
                cUrl = hilitedUrl.Merge(u"\e[00;32m", u"\e[0m");

                Cout << "CUrl:  " << cUrl << Endl;
            }
        }
        catch (yexception ex) {
            Cerr << ex.what() << Endl;
        }
        url.clear();
        q.clear();
    }

    return 0;
}

