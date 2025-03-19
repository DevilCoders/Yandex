#pragma once

#include <library/cpp/langs/langs.h>
#include <util/generic/list.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <functional>

namespace NSchemaOrg {
    class TMovie {
    public:
        bool ContainsBadText() const;
        TUtf16String GetRating() const;
        TVector<TWtringBuf> GetGenres(size_t maxAmount) const;
        TVector<TWtringBuf> GetDirectors(size_t maxAmount) const;
        TVector<TWtringBuf> GetActors(size_t maxAmount) const;
        TUtf16String GetDescription() const;
        TUtf16String GetDuration() const;
        TUtf16String GetInLanguage() const;
        TVector<TWtringBuf> GetMusicBys(size_t maxAmount) const;
        TVector<TWtringBuf> GetProducers(size_t maxAmount) const;
        TUtf16String GetIMDBTitle(TWtringBuf naturalTitle, ELanguage uil) const;

    public:
        TList<TUtf16String> Name;
        TUtf16String RatingValue;
        TUtf16String BestRating;
        TList<TUtf16String> Genre;
        TList<TUtf16String> Director;
        TList<TUtf16String> Actor;
        TUtf16String Description;
        TUtf16String Duration;
        TUtf16String InLanguage;
        TList<TUtf16String> MusicBy;
        TList<TUtf16String> Producer;
    };

    class TMovieSents {
    public:
        using TCaseFunc = std::function<void(TUtf16String&)>;

    public:
        class TImpl;
        THolder<TImpl> Impl;

    public:
        TMovieSents(ELanguage localeLang, TCaseFunc toTitleFunc, TCaseFunc toLowerFunc);
        ~TMovieSents();
        void FormatMovie(const TMovie& movie, bool addRating);
        const TUtf16String& GetResult() const;
    };

} // namespace NSchemaOrg
