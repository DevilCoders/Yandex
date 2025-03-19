#include <kernel/snippets/schemaorg/schemaorg_parse.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/charset/wide.h>
#include <util/string/vector.h>

using namespace NSchemaOrg;

Y_UNIT_TEST_SUITE(TSchemaOrgParseTests) {
    Y_UNIT_TEST(TestParseDuration) {
        TDuration res;

        res = ParseDuration("130 min");
        UNIT_ASSERT_EQUAL(TDuration::Minutes(130), res);

        res = ParseDuration("PT130M");
        UNIT_ASSERT_EQUAL(TDuration::Minutes(130), res);

        res = ParseDuration("PT2H10M00S");
        UNIT_ASSERT_EQUAL(TDuration::Minutes(130), res);

        res = ParseDuration("PT12H34M56S");
        UNIT_ASSERT_EQUAL(
            TDuration::Hours(12) + TDuration::Minutes(34) + TDuration::Seconds(56),
            res);

        res = ParseDuration("PT112H134M156S");
        UNIT_ASSERT_EQUAL(
            TDuration::Hours(112) + TDuration::Minutes(134) + TDuration::Seconds(156),
            res);

        res = ParseDuration("2H10M00S");
        UNIT_ASSERT_EQUAL(TDuration::Zero(), res);

        res = ParseDuration("ZZZ");
        UNIT_ASSERT_EQUAL(TDuration::Zero(), res);
    }

    Y_UNIT_TEST(TestCutSchemaPrefix) {
        UNIT_ASSERT_EQUAL(
            u"OutOfStock",
            CutSchemaPrefix(u"OutOfStock"));

        UNIT_ASSERT_EQUAL(
            u"OutOfStock",
            CutSchemaPrefix(u"http://schema.org/OutOfStock"));

        UNIT_ASSERT_EQUAL(
            u"schema.org/OutOfStock",
            CutSchemaPrefix(u"schema.org/OutOfStock"));

        UNIT_ASSERT_EQUAL(
            u"",
            CutSchemaPrefix(u"http://schema.org/"));
    }

    Y_UNIT_TEST(TestParseRating) {
        UNIT_ASSERT_EQUAL(u"10", ParseRating(u"10"));
        UNIT_ASSERT_EQUAL(u"10", ParseRating(u"10"));
        UNIT_ASSERT_EQUAL(u"9", ParseRating(u"9.000"));
        UNIT_ASSERT_EQUAL(u"9.1", ParseRating(u"9.100"));
        UNIT_ASSERT_EQUAL(u"9,1", ParseRating(u"9,100"));
        UNIT_ASSERT_EQUAL(u"0,1", ParseRating(u"0,1"));
        UNIT_ASSERT_EQUAL(u"9.1", ParseRating(u"- 9.100 /"));
        UNIT_ASSERT_EQUAL(u"9.1", ParseRating(u"9.100 pts"));
        UNIT_ASSERT_EQUAL(u"", ParseRating(u"-"));
        UNIT_ASSERT_EQUAL(u"", ParseRating(u"0"));
        UNIT_ASSERT_EQUAL(u"4,579/5", ParseRating(u"4,579/5"));
        UNIT_ASSERT_EQUAL(u"4 out of 5", ParseRating(u"4 out of 5"));
        UNIT_ASSERT_EQUAL(u"3.141592653589793238462643383279", ParseRating(u"3.141592653589793238462643383279"));
    }

    inline TUtf16String JoinStrings(const TVector<TWtringBuf>& v) {
        return JoinStrings(v.begin(), v.end(), u"^");
    }

    Y_UNIT_TEST(TestParseList) {
        TList<TUtf16String> genre;

        genre.clear();
        genre.push_back(u"Action");
        genre.push_back(u"Adventure");
        genre.push_back(u"Sci-Fi");
        UNIT_ASSERT_EQUAL(
            u"Action",
            JoinStrings(ParseList(genre, 1)));
        UNIT_ASSERT_EQUAL(
            u"Action^Adventure",
            JoinStrings(ParseList(genre, 2)));
        UNIT_ASSERT_EQUAL(
            u"Action^Adventure^Sci-Fi",
            JoinStrings(ParseList(genre, 3)));
        UNIT_ASSERT_EQUAL(
            u"Action^Adventure^Sci-Fi",
            JoinStrings(ParseList(genre, 4)));
        UNIT_ASSERT_EQUAL(
            u"Action^Adventure^Sci-Fi",
            JoinStrings(ParseGenreList(genre, 3)));

        genre.push_back(u"Genres: Action | Adventure | Sci-Fi");
        UNIT_ASSERT_EQUAL(
            u"Action^Adventure^Sci-Fi^Genres: Action | Adventure | Sci-Fi",
            JoinStrings(ParseList(genre, 4)));
        UNIT_ASSERT_EQUAL(
            u"Action^Adventure^Sci-Fi",
            JoinStrings(ParseGenreList(genre, 4)));

        genre.clear();
        genre.push_back(u"* фантастика, боевик, приключения");
        genre.push_back(u"приключения/мультфильм / мюзикл / фэнтези / комедия / мелодрама / семейный");
        TUtf16String expected = u"фантастика^боевик^приключения^мультфильм^мюзикл";
        UNIT_ASSERT_EQUAL(expected, JoinStrings(ParseList(genre, 5)));
        UNIT_ASSERT_EQUAL(expected, JoinStrings(ParseGenreList(genre, 5)));
    }

    Y_UNIT_TEST(TestCutLeftTrash) {
        UNIT_ASSERT_EQUAL(u"Aaa bbb. Ccc", CutLeftTrash(u"Aaa bbb. Ccc"));
        UNIT_ASSERT_EQUAL(u"", CutLeftTrash(u""));
        UNIT_ASSERT_EQUAL(u"Aaa bbb. Ccc", CutLeftTrash(u" - Aaa bbb. Ccc"));
        UNIT_ASSERT_EQUAL(u"Aaa bbb. Ccc", CutLeftTrash(u" *** Aaa bbb. Ccc"));
        UNIT_ASSERT_EQUAL(u"", CutLeftTrash(u" *** - *** "));
    }
}
