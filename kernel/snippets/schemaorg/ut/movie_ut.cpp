#include <kernel/snippets/schemaorg/movie.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/charset/wide.h>
#include <util/string/vector.h>

using namespace NSchemaOrg;

Y_UNIT_TEST_SUITE(TMovieTests) {
    Y_UNIT_TEST(TestMovieGetRating) {
        TMovie movie;

        movie.RatingValue = u"9";
        movie.BestRating = u"10";
        UNIT_ASSERT_EQUAL(u"9/10", movie.GetRating());

        movie.RatingValue = u"9";
        movie.BestRating = u"";
        UNIT_ASSERT_EQUAL(u"", movie.GetRating());

        movie.RatingValue = u"";
        movie.BestRating = u"10";
        UNIT_ASSERT_EQUAL(u"", movie.GetRating());

        movie.RatingValue = u"0";
        movie.BestRating = u"0";
        UNIT_ASSERT_EQUAL(u"", movie.GetRating());
    }

    inline TUtf16String JoinStrings(const TVector<TWtringBuf>& v) {
        return JoinStrings(v.begin(), v.end(), u"^");
    }

    Y_UNIT_TEST(TestMovieGetDuration) {
        TMovie movie;

        movie.Duration = u"130 min";
        UNIT_ASSERT_EQUAL(u"2:10:00", movie.GetDuration());

        movie.Duration = u"PT12H34M56S";
        UNIT_ASSERT_EQUAL(u"12:34:56", movie.GetDuration());

        movie.Duration = u"ZZZ";
        UNIT_ASSERT_EQUAL(u"", movie.GetDuration());
    }

    Y_UNIT_TEST(TestMovieGetIMDBTitle) {
        TMovie movie;
        TUtf16String naturalTitle;

        movie.Name.clear();
        movie.Name.push_back(u"Железный человек 3");
        movie.Name.push_back(u"\"Iron Man Three\" (original title)");
        naturalTitle = u"Железный человек 3 (2013) - IMDb";
        UNIT_ASSERT_EQUAL(u"Iron Man Three (2013) - IMDb", movie.GetIMDBTitle(naturalTitle, LANG_TUR));
        UNIT_ASSERT_EQUAL(u"Железный человек 3 (Iron Man Three, 2013) - IMDb", movie.GetIMDBTitle(naturalTitle, LANG_RUS));
        UNIT_ASSERT_EQUAL(u"", movie.GetIMDBTitle(u"zzz", LANG_RUS));
        UNIT_ASSERT_EQUAL(u"", movie.GetIMDBTitle(u"zzz (2013) - IMDb", LANG_RUS));

        movie.Name.clear();
        movie.Name.push_back(u"Iron Man 3");
        movie.Name.push_back(u"\"Iron Man Three\" (original title)");
        naturalTitle = u"Iron Man 3 (2013) - IMDb";
        UNIT_ASSERT_EQUAL(u"Iron Man Three (2013) - IMDb", movie.GetIMDBTitle(naturalTitle, LANG_TUR));
        UNIT_ASSERT_EQUAL(u"Iron Man 3 (Iron Man Three, 2013) - IMDb", movie.GetIMDBTitle(naturalTitle, LANG_RUS));

        movie.Name.clear();
        movie.Name.push_back(u"Гарри Поттер и Дары смерти: Часть II");
        movie.Name.push_back(u"\"Harry Potter and the Deathly Hallows: Part 2\" (original title)");
        naturalTitle = u"Гарри Поттер и Дары смерти: Часть II (2011) - IMDb";
        UNIT_ASSERT_EQUAL(u"Harry Potter and the Deathly Hallows: Part 2 (2011) - IMDb", movie.GetIMDBTitle(naturalTitle, LANG_TUR));
        UNIT_ASSERT_EQUAL(u"Гарри Поттер и Дары смерти: Часть II (Harry Potter and the Deathly Hallows: Part 2, 2011) - IMDb", movie.GetIMDBTitle(naturalTitle, LANG_RUS));

        movie.Name.clear();
        movie.Name.push_back(u"Harry Potter and the Deathly Hallows: Part 2");
        naturalTitle = u"Harry Potter and the Deathly Hallows: Part 2 (2011) - IMDb";
        UNIT_ASSERT_EQUAL(u"", movie.GetIMDBTitle(naturalTitle, LANG_TUR));
        UNIT_ASSERT_EQUAL(u"", movie.GetIMDBTitle(naturalTitle, LANG_RUS));

        movie.Name.clear();
        movie.Name.push_back(u"Три икса");
        movie.Name.push_back(u"\"xXx\" (original title)");
        naturalTitle = u"Три икса (2002) - IMDb";
        UNIT_ASSERT_EQUAL(u"xXx (2002) - IMDb", movie.GetIMDBTitle(naturalTitle, LANG_TUR));
        UNIT_ASSERT_EQUAL(u"Три икса (xXx, 2002) - IMDb", movie.GetIMDBTitle(naturalTitle, LANG_RUS));

        movie.Name.clear();
        movie.Name.push_back(u"(500) дней летa");
        movie.Name.push_back(u"\"(500) Days of Summer\" (original title)");
        naturalTitle = u"(500) дней летa (2009) - IMDb";
        UNIT_ASSERT_EQUAL(u"(500) Days of Summer (2009) - IMDb", movie.GetIMDBTitle(naturalTitle, LANG_TUR));
        UNIT_ASSERT_EQUAL(u"(500) дней летa ((500) Days of Summer, 2009) - IMDb", movie.GetIMDBTitle(naturalTitle, LANG_RUS));
    }
}
