#include "movie.h"
#include "schemaorg_parse.h"

#include <kernel/snippets/i18n/i18n.h>

#include <util/charset/wide.h>
#include <util/datetime/base.h>
#include <util/string/cast.h>
#include <util/string/printf.h>

namespace NSchemaOrg {
    static const TUtf16String SPACE_BRACKET = u" (";
    static const TUtf16String COMMA_SPACE = u", ";
    static const TUtf16String COLON_SPACE = u": ";
    static const TUtf16String SPACE = u" ";
    static const TUtf16String DOT = u".";
    static const TUtf16String SLASH = u"/";
    static const TUtf16String IMDB_BAD_DESC = u"Add a Plot.";
    static const TUtf16String IMDB_ORIGINAL_TITLE = u"(original title)";
    static const wchar16 IMDB_QUOTE = '\"';
    static const size_t MAX_LIST_AMOUNT = 3;

    static void SkipChar(TWtringBuf& str, wchar16 c) {
        if (str && str[0] == c) {
            str.Skip(1);
        }
    }

    static void ChopChar(TWtringBuf& str, wchar16 c) {
        if (str && str.back() == c) {
            str.Chop(1);
        }
    }

    static bool IsBadText(const TUtf16String& text) {
        // check if has substring of "<.+>" format
        const size_t openTagPos = text.find('<');
        if (openTagPos != TUtf16String::npos && text.find('>', openTagPos + 2) != TUtf16String::npos) {
            return true;
        }

        // check if hash substring of "&[#[::alnum::]]+;" format
        int sequenceBegin = -1;
        for (size_t i = 0; i < text.length(); ++i) {
            if (text[i] == '&') {
                sequenceBegin = i;
            } else if (text[i] == ';' && sequenceBegin >= 0 && i - sequenceBegin > 1) {
                return true;
            } else if (!isalnum(text[i]) && text[i] != '#') {
                sequenceBegin = -1;
            }
        }
        return false;
    }

    static bool IsBadList(const TList<TUtf16String>& list) {
        for (const TUtf16String& text : list) {
            if (IsBadText(text)) {
                return true;
            }
        }
        return false;
    }

    bool TMovie::ContainsBadText() const {
        return IsBadList(Name) || IsBadText(RatingValue) || IsBadText(BestRating) ||
               IsBadList(Genre) || IsBadList(Director) || IsBadList(Actor) ||
               IsBadText(Description) || IsBadText(Duration) || IsBadText(InLanguage) ||
               IsBadList(MusicBy) || IsBadList(Producer);
    }

    TUtf16String TMovie::GetRating() const {
        TWtringBuf ratingValue = ParseRating(RatingValue);
        TWtringBuf bestRating = ParseRating(BestRating);
        if (!ratingValue || !bestRating)
            return TUtf16String();
        return ToWtring(ratingValue) + SLASH + ToWtring(bestRating);
    }

    TVector<TWtringBuf> TMovie::GetGenres(size_t maxAmount) const {
        return ParseGenreList(Genre, maxAmount);
    }

    TVector<TWtringBuf> TMovie::GetDirectors(size_t maxAmount) const {
        return ParseList(Director, maxAmount);
    }

    TVector<TWtringBuf> TMovie::GetActors(size_t maxAmount) const {
        return ParseList(Actor, maxAmount);
    }

    TUtf16String TMovie::GetDescription() const {
        TWtringBuf description = CutLeftTrash(Description);
        if (description == IMDB_BAD_DESC) {
            description.Clear();
        }
        return ToWtring(description);
    }

    TUtf16String TMovie::GetDuration() const {
        TDuration duration = ParseDuration(WideToUTF8(Duration));
        if (duration == TDuration::Zero()) {
            return TUtf16String();
        }
        int hrs = static_cast<int>(duration.Hours());
        int mins = static_cast<int>(duration.Minutes() - 60 * duration.Hours());
        int secs = static_cast<int>(duration.Seconds() - 60 * duration.Minutes());
        return TUtf16String::FromAscii(Sprintf("%d:%02d:%02d", hrs, mins, secs));
    }

    TUtf16String TMovie::GetInLanguage() const {
        TUtf16String result = InLanguage;
        result.to_upper();
        return result;
    }

    TVector<TWtringBuf> TMovie::GetMusicBys(size_t maxAmount) const {
        return ParseList(MusicBy, maxAmount);
    }

    TVector<TWtringBuf> TMovie::GetProducers(size_t maxAmount) const {
        return ParseList(Producer, maxAmount);
    }

    // Title replacement for imdb.com, see examples in unit tests
    TUtf16String TMovie::GetIMDBTitle(TWtringBuf naturalTitle, ELanguage uil) const {
        TWtringBuf originalName;
        TWtringBuf localName;
        for (const TUtf16String& name : Name) {
            if (name.EndsWith(IMDB_ORIGINAL_TITLE)) {
                originalName = TWtringBuf(name).Chop(IMDB_ORIGINAL_TITLE.size());
                ChopChar(originalName, ' ');
                ChopChar(originalName, IMDB_QUOTE);
                SkipChar(originalName, IMDB_QUOTE);
            } else {
                localName = name;
            }
        }
        TUtf16String newTitle;
        if (localName && originalName && naturalTitle.StartsWith(localName)) {
            naturalTitle.Skip(localName.size());
            if (uil == LANG_TUR || uil == LANG_ENG) {
                newTitle = ToWtring(originalName) + ToWtring(naturalTitle);
            } else {
                SkipChar(naturalTitle, ' ');
                SkipChar(naturalTitle, '(');
                newTitle = ToWtring(localName) + SPACE_BRACKET +
                           ToWtring(originalName) + COMMA_SPACE + ToWtring(naturalTitle);
            }
        }
        return newTitle;
    }

    class TMovieSents::TImpl {
    public:
        TVector<TUtf16String> Sents;
        TUtf16String Result;
        const ELanguage LocaleLang;
        TMovieSents::TCaseFunc ToTitleFunc;
        TMovieSents::TCaseFunc ToLowerFunc;

    public:
        TImpl(ELanguage localeLang, TMovieSents::TCaseFunc toTitleFunc, TMovieSents::TCaseFunc toLowerFunc)
            : LocaleLang(localeLang)
            , ToTitleFunc(toTitleFunc)
            , ToLowerFunc(toLowerFunc)
        {
        }

        void FormatMovie(const NSchemaOrg::TMovie& movie, bool addRating) {
            if (addRating) {
                AddField("Rating", movie.GetRating());
            }
            AddGenreList(movie.GetGenres(MAX_LIST_AMOUNT));
            AddList("Director", movie.GetDirectors(MAX_LIST_AMOUNT + 1));
            AddList("Stars", movie.GetActors(MAX_LIST_AMOUNT + 1));
            AddDescription(movie.GetDescription());
            AddField("Duration", movie.GetDuration());
            AddField("Language", movie.GetInLanguage());
            AddList("Music", movie.GetMusicBys(MAX_LIST_AMOUNT + 1));
            AddList("Producer", movie.GetProducers(MAX_LIST_AMOUNT + 1));
            for (const TUtf16String& sent : Sents) {
                if (Result) {
                    Result.append(SPACE);
                }
                Result.append(sent);
                if (Result && !IsTerminal(Result.back())) {
                    Result.append(DOT);
                }
            }
        }

    private:
        void AddField(const TString& name, const TUtf16String& value) {
            if (value) {
                TUtf16String localName = NSnippets::Localize(name, LocaleLang);
                if (value.StartsWith(localName))
                    Sents.push_back(value);
                else
                    Sents.push_back(localName + COLON_SPACE + value);
            }
        }

        void AddDescription(const TUtf16String& description) {
            if (description) {
                Sents.push_back(description);
            }
        }

        void AddList(const TString& name, const TVector<TWtringBuf>& values) {
            if (!values.empty()) {
                TUtf16String result;
                int count = 0;
                for (const TWtringBuf& value : values) {
                    if (count == MAX_LIST_AMOUNT) {
                        result.append(SPACE).append(NSnippets::Localize("etc", LocaleLang));
                        break;
                    }
                    if (count > 0) {
                        result.append(COMMA_SPACE);
                    }
                    result.append(value);
                    ++count;
                }
                AddField(name, result);
            }
        }

        void AddGenreList(const TVector<TWtringBuf>& values) {
            if (!values.empty()) {
                TUtf16String result;
                int count = 0;
                for (const TWtringBuf& value : values) {
                    TUtf16String str = ToWtring(value);
                    if (count == 0) {
                        ToTitleFunc(str);
                    } else {
                        ToLowerFunc(str);
                        result.append(COMMA_SPACE);
                    }
                    result.append(str);
                    ++count;
                }
                Sents.push_back(result);
            }
        }
    };

    TMovieSents::TMovieSents(ELanguage localeLang, TCaseFunc toTitleFunc, TCaseFunc toLowerFunc)
        : Impl(new TImpl(localeLang, toTitleFunc, toLowerFunc))
    {
    }

    TMovieSents::~TMovieSents() {
    }

    void TMovieSents::FormatMovie(const TMovie& movie, bool addRating) {
        Impl->FormatMovie(movie, addRating);
    }

    const TUtf16String& TMovieSents::GetResult() const {
        return Impl->Result;
    }

} // namespace NSchemaOrg
