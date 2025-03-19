#include "findurl.h"

#include <util/generic/singleton.h>
#include <contrib/libs/re2/re2/re2.h>

namespace NMango {

    // Сгенерировано с помощью lib/link_extraction/gen_twitter_link_regexp.rb
    static const char *URL_PATTERN = "(?i)(?:[^-\\/\"':!=a-z0-9_@\\p{Cyrillic}]|^|\\:)(https?:\\/\\/(?:[^[:punct:]\\s]+(?:[\\.-][^[:punct:]\\s]+)*\\.[a-z\\p{Cyrillic}]{2,}(?::[0-9]+)?)(?:/(?:(?:(?:\\((?:[\\p{Cyrillic}a-z0-9!\\*';:=\\+\\,\\$\\/%#\\[\\]\\-_~\\(\\)])+\\))|@(?:[\\p{Cyrillic}a-z0-9!\\*';:=\\+\\,\\$\\/%#\\[\\]\\-_~\\(\\)])+\\/|[\\.,](?:[\\p{Cyrillic}a-z0-9!\\*';:=\\+\\,\\$\\/%#\\[\\]\\-_~\\(\\)])+|(?:[\\p{Cyrillic}a-z0-9!\\*';:=\\+\\,\\$\\/%#\\[\\]\\-_~\\(\\)])+)+(?:[\\p{Cyrillic}a-z0-9=_#\\/\\+\\-]|(?:\\((?:[\\p{Cyrillic}a-z0-9!\\*';:=\\+\\,\\$\\/%#\\[\\]\\-_~\\(\\)])+\\)))|(?:(?:\\((?:[\\p{Cyrillic}a-z0-9!\\*';:=\\+\\,\\$\\/%#\\[\\]\\-_~\\(\\)])+\\))|@(?:[\\p{Cyrillic}a-z0-9!\\*';:=\\+\\,\\$\\/%#\\[\\]\\-_~\\(\\)])+\\/|[\\.,](?:[\\p{Cyrillic}a-z0-9!\\*';:=\\+\\,\\$\\/%#\\[\\]\\-_~\\(\\)])+|(?:[\\p{Cyrillic}a-z0-9!\\*';:=\\+\\,\\$\\/%#\\[\\]\\-_~\\(\\)])+)+(?:[\\p{Cyrillic}a-z0-9=_#\\/\\+\\-]|(?:\\((?:[\\p{Cyrillic}a-z0-9!\\*';:=\\+\\,\\$\\/%#\\[\\]\\-_~\\(\\)])+\\)))?|(?:[\\p{Cyrillic}a-z0-9=_#\\/\\+\\-]|(?:\\((?:[\\p{Cyrillic}a-z0-9!\\*';:=\\+\\,\\$\\/%#\\[\\]\\-_~\\(\\)])+\\))))?)?(?:\\?(?:[\\p{Cyrillic}a-z0-9!\\*'\\(\\);:&=\\+\\$\\/%#\\[\\]\\-_\\.,~])*(?:[\\p{Cyrillic}a-z0-9_&=#\\/]))?)";

    class TUrlsFinder {
    public:
        typedef TVector< std::pair<int, int> > TSegments;
    private:
        RE2 Re;
    public:
        static const TUrlsFinder *InstanceConst() { const static TUrlsFinder uf; return &uf; }
        TUrlsFinder();

        TSegments Find(const TString &text) const;
        TVector<TString> CutUrls(const TString &text, TVector<TString> *urls = nullptr) const;
    };

    TUrlsFinder::TUrlsFinder()
        : Re(URL_PATTERN)
    {
        Y_ASSERT(Re.ok());
    }

    TUrlsFinder::TSegments TUrlsFinder::Find(const TString &text) const {
        TUrlsFinder::TSegments res;
        re2::StringPiece contents(text.data(), text.size());

        re2::StringPiece sp;
        while (RE2::FindAndConsume(&contents, Re, &sp)) {
            res.push_back(std::make_pair(sp.data() - text.data(), sp.size()));
        }
        return res;
    }

    TVector<TString> TUrlsFinder::CutUrls(const TString &text, TVector<TString> *urls) const {
        TVector<TString> res;
        TUrlsFinder::TSegments segs = Find(text);

        int prev = 0;
        for (TUrlsFinder::TSegments::const_iterator it = segs.begin(); it != segs.end(); ++it) {
            if (urls != nullptr) {
                urls->push_back(text.substr(it->first, it->second));
            }
            if (prev < it->first)
                res.push_back(text.substr(prev, it->first - prev));
            prev = it->first + it->second;
        }
        if (prev < static_cast<int>(text.size()))
            res.push_back(text.substr(prev));

        return res;
    }


    TVector<TString> CutUrls(const TString &text, TVector<TString> *urls) {
        return TUrlsFinder::InstanceConst()->CutUrls(text, urls);
    }

    TVector<TString> FindUrls(const TString &text) {
        TVector<TString> res;
        TUrlsFinder::InstanceConst()->CutUrls(text, &res);
        return res;
    }

    TString RemoveUrls(const TString &text) {
        return JoinStrings(TUrlsFinder::InstanceConst()->CutUrls(text, nullptr), "");
    }
}
