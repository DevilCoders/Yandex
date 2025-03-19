#include "yaca_list.h"

#include <library/cpp/regex/pire/regexp.h>

#include <library/cpp/string_utils/url/url.h>
#include <util/stream/file.h>
#include <util/stream/mem.h>
#include <util/generic/singleton.h>

using namespace NRegExp;

namespace NSnippets
{

    // something like cat kernel/snippets/custom/data/forced_yaca_urls.txt | contrib/tools/file2c/file2c
    const unsigned char ForcedYacaUrlsDefault[] = {
    104,116,116,112,115,63,58,47,47,40,40,92,119,124,92,100,124,45,124,95,41,
    123,48,44,50,48,125,92,46,41,63,40,121,97,124,121,97,110,100,101,120,124,
    209,143,208,189,208,180,208,181,208,186,209,129,41,92,46,92,119,123,50,
    44,51,125,47,63,10,104,116,116,112,58,47,47,97,112,105,92,46,121,97,110,
    100,101,120,92,46,114,117,47,109,97,112,115,47,10,104,116,116,112,58,47,
    47,102,120,92,46,121,97,110,100,101,120,92,46,114,117,47,112,111,114,116,
    97,98,108,101,92,46,120,109,108,10,104,116,116,112,58,47,47,109,97,112,
    115,92,46,121,97,110,100,101,120,92,46,114,117,47,109,111,115,99,111,119,
    47,10,104,116,116,112,58,47,47,109,97,112,115,92,46,121,97,110,100,101,
    120,92,46,114,117,47,109,111,115,99,111,119,95,116,114,97,102,102,105,99,
    47,10,104,116,116,112,58,47,47,109,97,112,115,92,46,121,97,110,100,101,
    120,92,46,114,117,47,112,101,116,101,114,115,98,117,114,103,47,10,104,116,
    116,112,58,47,47,109,111,105,107,114,117,103,92,46,114,117,47,10,104,116,
    116,112,58,47,47,110,92,46,109,97,112,115,92,46,121,97,110,100,101,120,
    92,46,114,117,47,10,104,116,116,112,58,47,47,119,119,119,92,46,121,97,110,
    100,101,120,92,46,114,117,47,99,97,116,97,108,111,103,10,104,116,116,112,
    58,47,47,119,119,119,92,46,121,97,112,114,111,98,107,105,92,46,114,117,
    47,10
    };

    void TForcedYacaUrls::LoadFromStream(IInputStream& in)
    {
        TFsm urlChecker(TFsm::False());
        TFsm::TOptions opt;
        opt.SetCaseInsensitive(true);

        TString tmpl;
        while(in.ReadLine(tmpl)) {
            try {
                urlChecker = urlChecker | TFsm(tmpl, opt);
            } catch (Pire::Error e) {
                Cerr << e.what() << " : \"" << tmpl << "\"" << Endl;
            }
        }
        UrlChecker.Reset(new TFsm(urlChecker));
    }

    void TForcedYacaUrls::LoadDefault()
    {
        TMemoryInput in(ForcedYacaUrlsDefault, Y_ARRAY_SIZE(ForcedYacaUrlsDefault));
        LoadFromStream(in);
    }

    void TForcedYacaUrls::LoadFromFile(const TString& fileName)
    {
        TUnbufferedFileInput in(fileName);
        LoadFromStream(in);
    }

    bool TForcedYacaUrls::Contains(const TString& url) const
    {
        return NRegExp::TMatcher(*UrlChecker.Get()).Match(AddSchemePrefix(url)).Final();
    }

    struct TDefaultGetter {
        TForcedYacaUrls Value;
        TDefaultGetter()
        {
            Value.LoadDefault();
        }
    };

    const TForcedYacaUrls& TForcedYacaUrls::GetDefault()
    {
        return Default<TDefaultGetter>().Value;
    }
}

