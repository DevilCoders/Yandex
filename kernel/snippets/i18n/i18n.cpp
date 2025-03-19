#include "i18n.h"

#include <contrib/libs/i18n/mofile.h>
#include <library/cpp/archive/yarchive.h>
#include <library/cpp/langs/langs.h>
#include <util/charset/wide.h>
#include <util/generic/hash.h>
#include <util/generic/ptr.h>
#include <util/generic/singleton.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/memory/blob.h>
#include <util/stream/input.h>

namespace NSnippets {

namespace {

const unsigned char SNIPPETS_LOCALES[] = {
#include "snippets_locales.inc"
};

class TMODictionary {
    using TMOFilePtr = TAutoPtr<TMOFile>;
    using TMOFiles = THashMap<ELanguage, TMOFilePtr>;
private:
    TMOFiles MOFiles;
public:
    TMODictionary() {
        TArchiveReader archive(TBlob::NoCopy(SNIPPETS_LOCALES, sizeof(SNIPPETS_LOCALES)));
        for (size_t i = 0; i < archive.Count(); ++i) {
            TString key = archive.KeyByIndex(i);
            TString lang = key;
            if (lang.StartsWith('/'))
                lang.erase(0, 1);
            if (lang.EndsWith(".mo"))
                lang.resize(lang.size() - 3);
            TMOFilePtr& moFile = MOFiles[LanguageByName(lang)];
            moFile.Reset(new TMOFile());
            moFile->LoadRaw(archive.ObjectByKey(key)->ReadAll());
        }
    }

    TUtf16String Localize(const TStringBuf& key, const ELanguage lang) const {
        TMOFiles::const_iterator it = MOFiles.find(lang);
        if (it == MOFiles.cend()) {
            it = MOFiles.find(LANG_ENG);//English is a default language
            Y_ASSERT(it != MOFiles.cend());
        }
        return UTF8ToWide(it->second->GetText(key));
    }
};

} //anonymous namespace

TUtf16String Localize(const TStringBuf& key, const ELanguage lang) {
    return Singleton<TMODictionary>()->Localize(key, lang);
}

} // namespace NSnippets
