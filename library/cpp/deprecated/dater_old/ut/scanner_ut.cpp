#include <library/cpp/deprecated/dater_old/scanner/dater.h>
#include <library/cpp/deprecated/dater_old/dater_stats.h>

#include <library/cpp/testing/unittest/registar.h>

#include <library/cpp/charset/recyr.hh>
#include <library/cpp/charset/wide.h>
#include <util/generic/algorithm.h>
#include <util/generic/vector.h>
#include <util/string/util.h>

#include <cstring>

/*
 * !!!!!IMPORTANT!!!!!
 * This code uses an ugly hack making it encoding-dependent.
 * Use only cp1251 for it.
 */
class TDateScannerTest: public TTestBase {
    UNIT_TEST_SUITE(TDateScannerTest);
    UNIT_TEST(DaterDateTest);
    UNIT_TEST(UrlDatesTest);
    UNIT_TEST(TextDatesTest);
    UNIT_TEST(DateStatsTest);
    UNIT_TEST_SUITE_END();

private:
    struct TErf {
        ui32 DaterFrom1 : 1;
        ui32 DaterFrom : 3;
        ui32 DaterYear : 5;
        ui32 DaterMonth : 4;
        ui32 DaterDay : 5;

        TErf() {
            Zero(*this);
        }

        TErf(ui32 dy, ui32 dm, ui32 dd, ui32 df, ui32 df1)
            : TErf()
        {
            DaterYear = dy;
            DaterMonth = dm;
            DaterDay = dd;
            DaterFrom = df;
            DaterFrom1 = df1;
        }
    };

    typedef std::pair<size_t, size_t> TOffsets;
    void DaterDateTest() {
        using namespace NDater;
        TDaterDate date;
        UNIT_ASSERT_C(!date, date.ToString());
        date = TDaterDate::MakeDateFull(11, 6, 31);
        UNIT_ASSERT_C(date && 1 == date.Day && 7 == date.Month && 2011 == date.Year, date.ToString());
        date = TDaterDate::MakeDateFull(11, 2, 30);
        UNIT_ASSERT_C(!date && !date.All, date.ToString());
        date = TDaterDate::MakeDateFull(11, 2, 29);
        UNIT_ASSERT_C(date && 1 == date.Day && 3 == date.Month && 2011 == date.Year, date.ToString());
        date = TDaterDate::MakeDateFull(11, 10, 9);
        UNIT_ASSERT_C(date && 9 == date.Day && 10 == date.Month && 2011 == date.Year, date.ToString());
        date = TDaterDate::MakeDateFull(11, 10, 32);
        UNIT_ASSERT_C(!date, date.ToString());
        date = TDaterDate::MakeDateFull(11, 13, 9);
        UNIT_ASSERT_C(!date, date.ToString());
        date = TDaterDate::MakeDateFull(11, 10, 9);
        UNIT_ASSERT_C(date && 9 == date.Day && 10 == date.Month && 2011 == date.Year, date.ToString());
        date = TDaterDate::MakeDateFull(95, 10, 9);
        UNIT_ASSERT_C(date && 9 == date.Day && 10 == date.Month && 1995 == date.Year && date.SaneInternetDate(), date.ToString());
        date = TDaterDate::MakeDateFull(50, 10, 9);
        UNIT_ASSERT_C(date && 9 == date.Day && 10 == date.Month && 1950 == date.Year && !date.SaneInternetDate(), date.ToString());
        date = TDaterDate::MakeDateFull(2050, 10, 9);
        UNIT_ASSERT_C(date && 9 == date.Day && 10 == date.Month && 2050 == date.Year && !date.SaneInternetDate(), date.ToString());
        date = TDaterDate::MakeDateMonth(11, 10);
        UNIT_ASSERT_C(date && 0 == date.Day && 10 == date.Month && 2011 == date.Year, date.ToString());
        date = TDaterDate::MakeDateYear(11);
        UNIT_ASSERT_C(date && 0 == date.Day && 0 == date.Month && 2011 == date.Year, date.ToString());
        date = TDaterDate::MakeDateFullNoYear(10, 9);
        UNIT_ASSERT_C(date && 9 == date.Day && 10 == date.Month && 0 == date.Year && date.NoYear, date.ToString());
        {
            TString s;

            s = "05/10/1999@U.D";
            date = TDaterDate::FromString(s);
            UNIT_ASSERT_STRINGS_EQUAL(date.ToString().c_str(), s.c_str());
            UNIT_ASSERT_VALUES_EQUAL_C(date.From, (ui32)TDaterDate::FromUrl, date.ToString());
            UNIT_ASSERT_C(!date.WordPattern, date.ToString());
            UNIT_ASSERT_C(date.LongYear, date.ToString());
            UNIT_ASSERT_EQUAL_C(date, TDaterDate::MakeDateFull(99, 10, 5), date.ToString());
            UNIT_ASSERT_C(date <= date, date.ToString());
            UNIT_ASSERT_C(date >= date, date.ToString());
            UNIT_ASSERT_C(!(date > date), date.ToString());
            UNIT_ASSERT_C(!(date < date), date.ToString());

            s = "00/10/1999@B.W";
            date = TDaterDate::FromString(s);
            UNIT_ASSERT_STRINGS_EQUAL(date.ToString().c_str(), s.c_str());
            UNIT_ASSERT_VALUES_EQUAL_C(date.From, (ui32)TDaterDate::FromContent, date.ToString());
            UNIT_ASSERT_C(date.WordPattern, date.ToString());
            UNIT_ASSERT_C(date.LongYear, date.ToString());
            UNIT_ASSERT_EQUAL_C(date, TDaterDate::MakeDateMonth(99, 10), date.ToString());

            s = "00/10/1999@Y.d";
            date = TDaterDate::FromString(s);
            UNIT_ASSERT_STRINGS_EQUAL(date.ToString().c_str(), s.c_str());
            UNIT_ASSERT_VALUES_EQUAL_C(date.From, (ui32)TDaterDate::FromText, date.ToString());
            UNIT_ASSERT_C(!date.WordPattern, date.ToString());
            UNIT_ASSERT_C(!date.LongYear, date.ToString());
            UNIT_ASSERT_EQUAL_C(date, TDaterDate::MakeDateMonth(99, 10), date.ToString());

            s = "00/00/1999@T.d";
            date = TDaterDate::FromString(s);
            UNIT_ASSERT_STRINGS_EQUAL(date.ToString().c_str(), s.c_str());
            UNIT_ASSERT_VALUES_EQUAL_C(date.From, (ui32)TDaterDate::FromTitle, date.ToString());
            UNIT_ASSERT_C(!date.WordPattern, date.ToString());
            UNIT_ASSERT_C(!date.LongYear, date.ToString());
            UNIT_ASSERT_EQUAL_C(date, TDaterDate::MakeDateYear(99), date.ToString());

            s = "00/00/0000@?.?";
            date = TDaterDate::FromString(s);
            UNIT_ASSERT_STRINGS_EQUAL(date.ToString().c_str(), s.c_str());
            UNIT_ASSERT_VALUES_EQUAL_C(date.From, (ui32)TDaterDate::FromUnknown, date.ToString());
            UNIT_ASSERT_EQUAL_C(date, TDaterDate(), date.ToString());

            s = "01/07/09@?.D";
            date = TDaterDate::FromString(s);
            UNIT_ASSERT_C(date.Day == 1 && date.Month == 7 && date.Year == 2009 && date.From == TDaterDate::FromUnknown && !date.WordPattern && date.LongYear, date.ToString());

            s = "01/07/09";
            date = TDaterDate::FromString(s);
            UNIT_ASSERT_C(date.Day == 1 && date.Month == 7 && date.Year == 2009 && date.From == TDaterDate::FromUnknown && !date.WordPattern && !date.LongYear, date.ToString());

            s = "00/00/00@F.w";
            date = TDaterDate::FromString(s);
            UNIT_ASSERT_C(date.Day == 0 && date.Month == 0 && date.Year == 2000 && date.WordPattern && !date.LongYear && date.From == TDaterDate::FromFooter,
                          date.ToString());

            s = "09/10/0000@F.w";
            date = TDaterDate::FromString(s);
            UNIT_ASSERT_C(date.Day == 9 && date.Month == 10 && date.Year == 0 && date.NoYear && date.WordPattern && !date.LongYear && date.From == TDaterDate::FromFooter,
                          date.ToString());

            date = TDaterDate::FromString("");
            UNIT_ASSERT_C(!date.All, date.ToString());
        }
        {
            TErf erf;

            WriteBestDateToErf(TDaterDate(), erf);
            date = ReadBestDateFromErf(erf);
            UNIT_ASSERT_C(!date.All, date.ToString());

            WriteBestDateToErf(TDaterDate(1992, 2, 1, TDaterDate::FromExternal), erf);
            date = ReadBestDateFromErf(erf);
            UNIT_ASSERT_C(TDaterDate::FromExternal == date.From && 1992 == date.Year && 2 == date.Month && 1 == date.Day, date.ToString());

            WriteBestDateToErf(TDaterDate(0, 2, 1, TDaterDate::FromExternal), erf);
            date = ReadBestDateFromErf(erf);
            UNIT_ASSERT_C(date && date.NoYear, date.ToString());

            WriteBestDateToErf(TDaterDate(1992, 0, 0, TDaterDate::FromExternal), erf);
            date = ReadBestDateFromErf(erf);
            UNIT_ASSERT_C(TDaterDate::TrustLevelExternal == date.TrustLevel() && date.IsVeryTrusted() && 1992 == date.Year && 0 == date.Month && 0 == date.Day, date.ToString());

            WriteBestDateToErf(TDaterDate(1992, 2, 0, TDaterDate::FromExternal), erf);
            date = ReadBestDateFromErf(erf);
            UNIT_ASSERT_C(1992 == date.Year && TDaterDate::TrustLevelExternal == date.TrustLevel() && date.IsVeryTrusted() && 2 == date.Month && 0 == date.Day, date.ToString());

            WriteBestDateToErf(TDaterDate(1992, 2, 0, TDaterDate::FromUrlId), erf);
            date = ReadBestDateFromErf(erf);
            UNIT_ASSERT_C(TDaterDate::FromUrlId == date.From && TDaterDate::TrustLevelUrl == date.TrustLevel() && date.IsVeryTrusted() && 1992 == date.Year && 2 == date.Month && 0 == date.Day, date.ToString());

            WriteBestDateToErf(TDaterDate(1994, 5, 12, TDaterDate::FromText), erf);
            date = ReadBestDateFromErf(erf);
            UNIT_ASSERT_C(TDaterDate::FromText == date.From && TDaterDate::TrustLevelText == date.TrustLevel()
                              //                            && !date.IsTrusted()
                              && date.IsLessTrusted() && 1994 == date.Year && 5 == date.Month && 12 == date.Day,
                          date.ToString());
        }
        {
            UNIT_ASSERT_VALUES_EQUAL(1330473600u, (ui32)ReadBestDateFromErf(TErf(23, 2, 29, 5, 0)).ToTimeT());
            UNIT_ASSERT_VALUES_EQUAL(1388188800u, (ui32)ReadBestDateFromErf(TErf(24, 12, 28, 5, 0)).ToTimeT());
            UNIT_ASSERT_VALUES_EQUAL(1350950400u, (ui32)ReadBestDateFromErf(TErf(23, 10, 23, 5, 0)).ToTimeT());
            UNIT_ASSERT_VALUES_EQUAL(1363392000u, (ui32)ReadBestDateFromErf(TErf(24, 3, 16, 5, 0)).ToTimeT());
            UNIT_ASSERT_VALUES_EQUAL(1361059200u, (ui32)ReadBestDateFromErf(TErf(24, 2, 17, 5, 0)).ToTimeT());
            UNIT_ASSERT_VALUES_EQUAL(1356220800u, (ui32)ReadBestDateFromErf(TErf(23, 12, 23, 5, 0)).ToTimeT());
            UNIT_ASSERT_VALUES_EQUAL(1337212800u, (ui32)ReadBestDateFromErf(TErf(23, 5, 17, 5, 0)).ToTimeT());
            UNIT_ASSERT_VALUES_EQUAL(1339718400u, (ui32)ReadBestDateFromErf(TErf(23, 6, 15, 5, 0)).ToTimeT());
            UNIT_ASSERT_VALUES_EQUAL(1237248000u, (ui32)ReadBestDateFromErf(TErf(20, 3, 17, 5, 0)).ToTimeT());
            UNIT_ASSERT_VALUES_EQUAL(1376265600u, (ui32)ReadBestDateFromErf(TErf(24, 8, 12, 5, 0)).ToTimeT());
        }
        // todo: test date formatting and GetMonths() and GetYears()
    }

    void CheckUrlDates(const char* text) {
        NDater::TDateCoords dates;
        CheckUrlDates(text, dates);
    }

    void CheckUrlDates(const char* text, size_t common, NDater::TDaterDate rd0, TOffsets c0) {
        NDater::TDateCoords dates;
        rd0.From = rd0.From ? rd0.From : (ui32)NDater::TDaterDate::FromUrl;
        NDater::TDateCoord date0(rd0, NDater::TCoord(common + c0.first, common + c0.second));
        dates.push_back(date0);

        CheckUrlDates(text, dates);
    }

    void CheckUrlDates(const char* text, size_t common, NDater::TDaterDate rd0, TOffsets c0, NDater::TDaterDate rd1,
                       TOffsets c1) {
        NDater::TDateCoords dates;
        rd0.From = rd0.From ? rd0.From : (ui32)NDater::TDaterDate::FromUrl;
        rd1.From = rd1.From ? rd1.From : (ui32)NDater::TDaterDate::FromUrl;
        NDater::TDateCoord date0(rd0, NDater::TCoord(common + c0.first, common + c0.second));
        NDater::TDateCoord date1(rd1, NDater::TCoord(common + c1.first, common + c1.second));
        dates.push_back(date0);
        dates.push_back(date1);

        CheckUrlDates(text, dates);
    }

    void CheckUrlDates(const char* text, size_t common, NDater::TDaterDate rd0, TOffsets c0, NDater::TDaterDate rd1,
                       TOffsets c1, NDater::TDaterDate rd2, TOffsets c2) {
        NDater::TDateCoords dates;
        rd0.From = rd0.From ? rd0.From : (ui32)NDater::TDaterDate::FromUrl;
        rd1.From = rd1.From ? rd1.From : (ui32)NDater::TDaterDate::FromUrl;
        rd2.From = rd2.From ? rd2.From : (ui32)NDater::TDaterDate::FromUrl;
        NDater::TDateCoord date0(rd0, NDater::TCoord(common + c0.first, common + c0.second));
        NDater::TDateCoord date1(rd1, NDater::TCoord(common + c1.first, common + c1.second));
        NDater::TDateCoord date2(rd2, NDater::TCoord(common + c2.first, common + c2.second));
        dates.push_back(date0);
        dates.push_back(date1);
        dates.push_back(date2);

        CheckUrlDates(text, dates);
    }

    void CheckUrlDates(const char* text, NDater::TDateCoords refDates) {
        using namespace NDater;
        TDateCoords dates = NDater::ScanUrl(text, text + strlen(text));
        Sort(refDates.begin(), refDates.end());
        Sort(dates.begin(), dates.end());

        for (TDateCoords::const_iterator it = dates.begin(), rit = refDates.begin(); it != dates.end() && rit != refDates.end(); ++it, ++rit) {
            UNIT_ASSERT_STRINGS_EQUAL_C(rit->ToString().c_str(), it->ToString().c_str(), text);
            UNIT_ASSERT_VALUES_EQUAL_C(rit->Begin, it->Begin, text);
            UNIT_ASSERT_VALUES_EQUAL_C(rit->End, it->End, text);
        }

        UNIT_ASSERT_VALUES_EQUAL_C(refDates.size(), dates.size(), text);
    }

    void UrlDatesTest() {
        using namespace NDater;

        CheckUrlDates("http://lenta.ru/09/10/11", strlen("http://lenta.ru/"), //
                      TDaterDate(2009, 10, 11, TDaterDate::FromUrl), TOffsets(0, 8));
        CheckUrlDates("http://lenta.ru/09/10/11/", strlen("http://lenta.ru/"), //
                      TDaterDate(2009, 10, 11, TDaterDate::FromUrl), TOffsets(0, 8));
        CheckUrlDates("http://lenta.ru/09/10/11?", strlen("http://lenta.ru/"), //
                      TDaterDate(2009, 10, 11, TDaterDate::FromUrl), TOffsets(0, 8));
        CheckUrlDates("http://lenta.ru/09/10/11#", strlen("http://lenta.ru/"), //
                      TDaterDate(2009, 10, 11, TDaterDate::FromUrl), TOffsets(0, 8));
        CheckUrlDates("http://2009.lenta.ru/2009/10/11?", strlen("http://"), //
                      TDaterDate(2009, 0, 0, TDaterDate::FromHost, true), TOffsets(0, 4),
                      TDaterDate(2009, 10, 11, TDaterDate::FromUrl, true),
                      TOffsets(strlen("2009.lenta.ru/"), strlen("2009.lenta.ru/") + 10));
        CheckUrlDates("http://lenta.ru/79/10/11?");
        CheckUrlDates("http://09-10-11.ru");
        CheckUrlDates("http://lenta.ru/2009/10/11?08.07.06", strlen("http://lenta.ru/"),    //
                      TDaterDate(2009, 10, 11, TDaterDate::FromUrl, true), TOffsets(0, 10), //
                      TDaterDate(2006, 7, 8, TDaterDate::FromUrl), TOffsets(11, 19));
        CheckUrlDates("http://lenta.ru/2009/10/11?08.7.6", strlen("http://lenta.ru/"), //
                      TDaterDate(2009, 10, 11, TDaterDate::FromUrl, true), TOffsets(0, 10));
        CheckUrlDates("http://lenta.ru/79/10/11?08.7.6");
        CheckUrlDates("http://lenta.ru/79/10/11?08-7.6");
        CheckUrlDates("http://lenta.ru/79/10/11?08-7.6#08.07.06");
        CheckUrlDates("http://lenta.ru/20091011", strlen("http://lenta.ru/"), //
                      TDaterDate(2009, 10, 11, TDaterDate::FromUrlId, true), TOffsets(0, 8));
        CheckUrlDates("http://lenta.ru/091011");
        CheckUrlDates("http://lenta.ru/0091011");
        CheckUrlDates("http://lenta.ru/09may99", strlen("http://lenta.ru/"), //
                      TDaterDate(1999, 5, 9, TDaterDate::FromUrl, false, true), TOffsets(0, 7));
        CheckUrlDates("http://lenta.ru/may99/test", strlen("http://lenta.ru/"), //
                      TDaterDate(1999, 5, 0, TDaterDate::FromUrl, false, true), TOffsets(0, 5));
        CheckUrlDates("http://lenta.ru/may/99/test", strlen("http://lenta.ru/"), //
                      TDaterDate(1999, 5, 0, TDaterDate::FromUrl, false, true), TOffsets(0, 6));
        CheckUrlDates("http://lenta.ru/09/may/test", strlen("http://lenta.ru/"), //
                      TDaterDate(2009, 5, 0, TDaterDate::FromUrl, false, true), TOffsets(0, 6));
        CheckUrlDates("http://www.SeoPro.ru/news/2009/2/114.html", //
                      strlen("http://www.SeoPro.ru/news/"),        //
                      TDaterDate(2009, 2, 0, TDaterDate::FromUrl, true), TOffsets(0, 6));
        CheckUrlDates("http://new.gpsclub.tomsk.ru/gps/dat120520092254/index.html", //
                      strlen("http://new.gpsclub.tomsk.ru/gps/dat"),                //
                      TDaterDate(2009, 5, 12, TDaterDate::FromUrlId, true), TOffsets(0, 12));
        CheckUrlDates("http://subj.us/20070830/luzhkov_ostavil_cherkizovski_diaspore_gorskih_evreev.html", //
                      strlen("http://subj.us/"),                                                           //
                      TDaterDate(2007, 8, 30, TDaterDate::FromUrlId, true), TOffsets(0, 8));
        CheckUrlDates("http://www.bcc.ru/press/publishing/pub02/pub20021213.html", //
                      strlen("http://www.bcc.ru/press/publishing/pub02/pub"),      //
                      TDaterDate(2002, 12, 13, TDaterDate::FromUrlId, true), TOffsets(0, 8));
        CheckUrlDates("http://www.wc3life.com/load/15-1-0-386");
        CheckUrlDates("http://www.groklaw.net/articlebasic.php?story=20080829221757478", //
                      strlen("http://www.groklaw.net/articlebasic.php?story="),          //
                      TDaterDate(2008, 8, 29, TDaterDate::FromUrlId, true), TOffsets(0, 17));
        CheckUrlDates("http://www.vedomosti.ru/newspaper/article.shtml?2008/09/16/161264", //
                      strlen("http://www.vedomosti.ru/newspaper/article.shtml?"),          //
                      TDaterDate(2008, 9, 16, TDaterDate::FromUrl, true), TOffsets(0, 10));
        CheckUrlDates("http://www.cnews.ru/news/line/index.shtml?2007/11/06/273448", //
                      strlen("http://www.cnews.ru/news/line/index.shtml?"),          //
                      TDaterDate(2007, 11, 6, TDaterDate::FromUrl, true), TOffsets(0, 10));
        CheckUrlDates("http://digitalshop.ru/shop/?action=print_info_tovar&id_podrazdel=98&id_tovar=2000902");
        CheckUrlDates("http://kkbweb.borda.ru/?1-5-0-00000003-000-0-0");
        CheckUrlDates("http://megalife.com.ua/2007/09/12/fraps_10.9.21__novaja_versija.html", //
                      strlen("http://megalife.com.ua/"),                                      //
                      TDaterDate(2007, 9, 12, TDaterDate::FromUrl, true), TOffsets(0, 10));
        CheckUrlDates("vp-site.net/2009/05/12/publichnye-izvrashhency-porno-onlajn.html", //
                      strlen("vp-site.net/"),                                             //
                      TDaterDate(2009, 5, 12, TDaterDate::FromUrl, true), TOffsets(0, 10));
        CheckUrlDates("vp-site.net?2008-10-03/2009/05/12/publichnye-izvrashhency-porno-onlajn.html", //
                      strlen("vp-site.net?"),                                                        //
                      TDaterDate(2008, 10, 3, TDaterDate::FromUrl, true), TOffsets(0, 10),           //
                      TDaterDate(2009, 5, 12, TDaterDate::FromUrl, true), TOffsets(11, 21));
        CheckUrlDates("http://pro-witch.ucoz.ru/forum/2-1-1");
        CheckUrlDates("http://xx-xx.ucoz.ru/publ/2-1-0-12");
        CheckUrlDates("http://investigl.3dn.ru/publ/10-10-9-10");
        CheckUrlDates("www.airwar.ru/enc/fighter/j10.html");
        CheckUrlDates("http://www.ixbt.com/news/all/index.shtml?11/17/06");
        CheckUrlDates("www.mignews.com/news/society/world/041207_112526_96429.html");
        CheckUrlDates("http://www.b2b-sng.ru/firms/index.html?show=buyers&f_country=643_4&cat_id=10100000");
        CheckUrlDates("www.garweb.ru/conf/ks/20030129/smi/msg.asp@id_msg130044.htm", strlen("www.garweb.ru/conf/ks/"),
                      TDaterDate(2003, 1, 29, TDaterDate::FromUrlId, true), TOffsets(0, 8));
        CheckUrlDates("an-doki.blogspot.com/2009_04_01_archive.html", strlen("an-doki.blogspot.com/"),
                      TDaterDate(2009, 04, 01, TDaterDate::FromUrl, true), TOffsets(0, 10));
    }

    void CheckTextDates(const char* text) {
        ui32 rd[] = {0};
        CheckTextDates(text, 0, rd);
    }

    void CheckTextDates(const char* text, NDater::TDaterDate rd0) {
        ui32 rd[] = {rd0.All};
        CheckTextDates(text, 1, rd);
    }

    void CheckTextDates(const char* text, NDater::TDaterDate rd0, NDater::TDaterDate rd1) {
        ui32 rd[] = {rd0.All, rd1.All};
        CheckTextDates(text, 2, rd);
    }

    void CheckTextDates(const char* text, NDater::TDaterDate rd0, NDater::TDaterDate rd1, NDater::TDaterDate rd2) {
        ui32 rd[] = {rd0.All, rd1.All, rd2.All};
        CheckTextDates(text, 3, rd);
    }

    void CheckTextDates(const char* text, ui32 sz, ui32* rd) {
        TUtf16String wtext = UTF8ToWide(text);
        NDater::TDateCoords refDates;

        if (sz && rd) {
            for (ui32 i = 0; i < sz; ++i) {
                NDater::TDaterDate date;
                date.All = rd[i];
                refDates.push_back(NDater::TDateCoord(date));
            }
        }

        Sort(refDates.begin(), refDates.end());

        NDater::TDateCoords dates = NDater::ScanText(wtext.begin(), wtext.end());

        NDater::FilterOverlappingDates(dates);

        Sort(dates.begin(), dates.end());

        for (NDater::TDateCoords::iterator it = dates.begin(), rit = refDates.begin(); it != dates.end() && rit != refDates.end(); ++it, ++rit) {
            UNIT_ASSERT_STRINGS_EQUAL_C(rit->ToString().c_str(), it->ToString().c_str(), WideToUTF8(wtext));
        }

        UNIT_ASSERT_VALUES_EQUAL_C(refDates.size(), dates.size(), WideToUTF8(wtext));
    }

    void TextDatesTest() {
        using namespace NDater;

        CheckTextDates("10.11.12", TDaterDate(2012, 11, 10, TDaterDate::FromText));
        CheckTextDates("10/11/12", TDaterDate(2012, 11, 10, TDaterDate::FromText));
        CheckTextDates("2012.11.10", TDaterDate(2012, 11, 10, TDaterDate::FromText, true));
        CheckTextDates("2012/11/10", TDaterDate(2012, 11, 10, TDaterDate::FromText, true));
        CheckTextDates("2012/31/10", TDaterDate(2012, 0, 0, TDaterDate::FromText, true));
        CheckTextDates("10 . 11 . 12", TDaterDate(2012, 11, 10, TDaterDate::FromText));
        CheckTextDates("1 . 1 . 12");
        CheckTextDates("10 / 11 / 12", TDaterDate(2012, 11, 10, TDaterDate::FromText));
        CheckTextDates("10 /11 /12 ", TDaterDate(2012, 11, 10, TDaterDate::FromText));
        CheckTextDates("1 / 2 / 2003 ", TDaterDate(2003, 2, 1, TDaterDate::FromText, true));
        CheckTextDates("2003 / 2 / 1 ", TDaterDate(2003, 2, 1, TDaterDate::FromText, true));
        CheckTextDates("10-11-12");
        CheckTextDates("10-11-2012", TDaterDate(2012, 11, 10, TDaterDate::FromText, true));
        CheckTextDates("10 . 11 .  12");
        CheckTextDates("10. 11. 2012", TDaterDate(2012, 11, 10, TDaterDate::FromText, true));
        CheckTextDates("10-ого Ноября 2012", TDaterDate(2012, 11, 10, TDaterDate::FromText, true, true));
        CheckTextDates("10ое, ноябрь 2012", TDaterDate(2012, 11, 10, TDaterDate::FromText, true, true));
        CheckTextDates("10 НОЯБ. 2012", TDaterDate(2012, 11, 10, TDaterDate::FromText, true, true));
        CheckTextDates("10 НОЯБ 2012", TDaterDate(2012, 11, 10, TDaterDate::FromText, true, true));
        CheckTextDates("10 ноя 99", TDaterDate(1999, 11, 10, TDaterDate::FromText, false, true));
        CheckTextDates("10 НОЯБ 12");
        CheckTextDates("ноябрь, 1е, 2012г", TDaterDate(2012, 11, 1, TDaterDate::FromText, true, true));
        CheckTextDates("1'st of November, 2012", TDaterDate(2012, 11, 1, TDaterDate::FromText, true, true));
        CheckTextDates("1st Nov. 2012", TDaterDate(2012, 11, 1, TDaterDate::FromText, true, true));
        CheckTextDates("2nd nov 2012", TDaterDate(2012, 11, 2, TDaterDate::FromText, true, true));
        CheckTextDates("10 NOV 2012", TDaterDate(2012, 11, 10, TDaterDate::FromText, true, true));
        CheckTextDates("1999г.", TDaterDate(1999, 0, 0, TDaterDate::FromText, true));
        CheckTextDates("10.11.12 1999", TDaterDate(2012, 11, 10, TDaterDate::FromText), //
                       TDaterDate(1999, 0, 0, TDaterDate::FromText, true));
        CheckTextDates("13.14.15 1999", TDaterDate(1999, 0, 0, TDaterDate::FromText, true));
        CheckTextDates("10.11.12 10.11.12", TDaterDate(2012, 11, 10, TDaterDate::FromText),
                       TDaterDate(2012, 11, 10, TDaterDate::FromText));
        CheckTextDates("Пятница, 8 мая 2009 г.", //
                       TDaterDate(2009, 5, 8, TDaterDate::FromText, true, true));
        CheckTextDates("май 99г.", TDaterDate(1999, 5, 0, TDaterDate::FromText, false, true));
        CheckTextDates("05nov2005", TDaterDate(2005, 11, 5, TDaterDate::FromText, true, true));
        CheckTextDates("Последнее обновление: 07.11.04 14:46", //
                       TDaterDate(2004, 11, 7, TDaterDate::FromText));
        CheckTextDates("07.11.04 14:46", TDaterDate(2004, 11, 7, TDaterDate::FromText));
        CheckTextDates("Все права защищены 2009", TDaterDate(2009, 0, 0, TDaterDate::FromText, true));
        CheckTextDates("2001-2004Ѣ", TDaterDate(2001, 0, 0, TDaterDate::FromText, true),
                       TDaterDate(2004, 0, 0, TDaterDate::FromText, true));
        CheckTextDates("Ѣ 2007-2009", TDaterDate(2007, 0, 0, TDaterDate::FromText, true),
                       TDaterDate(2009, 0, 0, TDaterDate::FromText, true));
        CheckTextDates("June 13, 2008, 12:22:24 PM", //
                       TDaterDate(2008, 6, 13, TDaterDate::FromText, true, true));
        CheckTextDates("Дата регистрации: 2008-05-16 | Подробная информация", //
                       TDaterDate(2008, 5, 16, TDaterDate::FromText, true));
        CheckTextDates("Наш телефон 8 (0112) 10-02-98");
        CheckTextDates("Товар был добавлен в наш каталог 21 Июня 2006г.", //
                       TDaterDate(2006, 6, 21, TDaterDate::FromText, true, true));
        CheckTextDates("Товар был добавлен в наш каталог 21 June 2006г.", //
                       TDaterDate(2006, 6, 21, TDaterDate::FromText, true, true));
        CheckTextDates("4 мая 2009 ", TDaterDate(2009, 5, 4, TDaterDate::FromText, true, true));
        CheckTextDates("September-2008", TDaterDate(2008, 9, 0, TDaterDate::FromText, true, true));
        CheckTextDates("2008-September", TDaterDate(2008, 9, 0, TDaterDate::FromText, true, true));
        CheckTextDates("2008-Сентябрь", TDaterDate(2008, 9, 0, TDaterDate::FromText, true, true));
        CheckTextDates("2008 Сентябрь - С мыслью по жизни", //
                       TDaterDate(2008, 9, 0, TDaterDate::FromText, true, true));
        CheckTextDates("2008 , Сентябрь", TDaterDate(2008, 9, 0, TDaterDate::FromText, true, true));
        CheckTextDates("Сентябрь-2008г", TDaterDate(2008, 9, 0, TDaterDate::FromText, true, true));

        // yes, this is definitely upyachka, but they asked for it
        CheckTextDates("обороты 1500 - 2000 об/мин", //
                       TDaterDate(2000, 0, 0, TDaterDate::FromText, true));

        CheckTextDates("12 Апр, 6 ч. и 9 мин.");

        // yes, this is definitely upyachka, but they asked for it
        CheckTextDates("ИП-2009, ИП-2018, ИП-2014Б",                       //
                       TDaterDate(2009, 0, 0, TDaterDate::FromText, true), //
                       TDaterDate(2018, 0, 0, TDaterDate::FromText, true), //
                       TDaterDate(2014, 0, 0, TDaterDate::FromText, true));
        CheckTextDates("26650 frm, 25,0000 frm/s");
        CheckTextDates("http://www.free-smile.info/smiles/5/2/5_2_4.gif");
        CheckTextDates("5/2/06", TDaterDate(2006, 2, 5, TDaterDate::FromText));
        CheckTextDates("2005/2/6", TDaterDate(2005, 2, 6, TDaterDate::FromText, true));
        CheckTextDates("фотку своей ауди в обмен предложишь))) Mug-Hunter  [24.11.2005 12:18:28]", //
                       TDaterDate(2005, 11, 24, TDaterDate::FromText, true));
        CheckTextDates("10.11.12.13");
        CheckTextDates("родился 22 ноября 1962 года в г. Москве.", TDaterDate(1962, 11, 22, TDaterDate::FromText, true,
                                                                              true));
        CheckTextDates("10.10.09г.", TDaterDate(2009, 10, 10, TDaterDate::FromText));
        CheckTextDates("01.05.2009-01.07.2009", TDaterDate(2009, 5, 1, TDaterDate::FromText, true),
                       TDaterDate(2009, 7, 1, TDaterDate::FromText, true));
        CheckTextDates("15Сентября2007", TDaterDate(2007, 9, 15, TDaterDate::FromText, true, true));
        CheckTextDates("СССР (12.07.1938 г. ѴСтроитель Востокаѵ", //
                       TDaterDate(1938, 7, 12, TDaterDate::FromText, true));
        CheckTextDates("Добавлено: Вс Мар 12, 2006 21:51", //
                       TDaterDate(2006, 3, 12, TDaterDate::FromText, true, true));
        CheckTextDates("7 Дек 06");
        CheckTextDates("17 августа, 15:06");
        CheckTextDates("This Week in Space 11 ę March 12, 2010", //
                       TDaterDate(2010, 3, 12, TDaterDate::FromText, true, true));
        CheckTextDates("25 августа ę 11 сентября 1960", //
                       TDaterDate(1960, 9, 11, TDaterDate::FromText, true, true));
        CheckTextDates("01.08.2009Спорт", TDaterDate(2009, 8, 1, TDaterDate::FromText, true, false));
        CheckTextDates(" <30/06/2009> ", TDaterDate(2009, 06, 30, TDaterDate::FromText, true, false));
    }

    void DateStatsTest() {
        using namespace NDater;
        {
            TDaterStats stats;
            UNIT_ASSERT(stats.Empty());
            UNIT_ASSERT_VALUES_EQUAL((ui32)TDaterDate::ErfZeroYear, stats.MaxYear(TDaterDate::FromUrlId));
            UNIT_ASSERT_VALUES_EQUAL((ui32)TDaterDate::ErfZeroYear, stats.MinYear(TDaterDate::FromUrlId));
            UNIT_ASSERT_VALUES_EQUAL(0, stats.YearNormLikelihood());
            UNIT_ASSERT_VALUES_EQUAL(0, stats.AverageSourceSegment());

            stats.Add(TDaterDate(2007, 1, 2, TDaterDate::FromUrlId));
            UNIT_ASSERT(!stats.Empty());
            UNIT_ASSERT_VALUES_EQUAL_C(2007u, stats.MaxYear(TDaterDate::FromUrlId), stats.ToStringYears());
            UNIT_ASSERT_VALUES_EQUAL_C(2007u, stats.MaxYear(TDaterDate::FromUrlId, TDaterDate::ModeFull), stats.ToStringYears());
            UNIT_ASSERT_VALUES_EQUAL(0, stats.YearNormLikelihood());
            UNIT_ASSERT_VALUES_EQUAL(156, stats.AverageSourceSegment());

            stats.Add(TDaterDate(2009, 0, 0, TDaterDate::FromUrlId));
            UNIT_ASSERT_VALUES_EQUAL_C(2009u, stats.MaxYear(TDaterDate::FromUrlId), stats.ToStringYears());
            UNIT_ASSERT_VALUES_EQUAL_C(2007u, stats.MaxYear(TDaterDate::FromUrlId, TDaterDate::ModeFull), stats.ToStringYears());
            UNIT_ASSERT_VALUES_EQUAL(149, stats.YearNormLikelihood());
            UNIT_ASSERT_VALUES_EQUAL(156, stats.AverageSourceSegment());

            stats.Add(TDaterDate(2009, 1, 2, TDaterDate::FromFooter));
            UNIT_ASSERT_VALUES_EQUAL_C(2009u, stats.MaxYear(TDaterDate::FromUrlId), stats.ToStringYears());
            UNIT_ASSERT_VALUES_EQUAL_C(2007u, stats.MaxYear(TDaterDate::FromUrlId, TDaterDate::ModeFull), stats.ToStringYears());
            UNIT_ASSERT_VALUES_EQUAL(144, stats.YearNormLikelihood());
            UNIT_ASSERT_VALUES_EQUAL(143, stats.AverageSourceSegment());

            stats.Add(TDaterDate(2009, 1, 2, TDaterDate::FromUrlId));
            stats.Add(TDaterDate(2009, 1, 2, TDaterDate::FromUrlId));
            UNIT_ASSERT_VALUES_EQUAL_C(2009u, stats.MaxYear(TDaterDate::FromUrlId), stats.ToStringYears());
            UNIT_ASSERT_VALUES_EQUAL_C(2009u, stats.MaxYear(TDaterDate::FromUrlId, TDaterDate::ModeFull), stats.ToStringYears());
            UNIT_ASSERT_VALUES_EQUAL(135, stats.YearNormLikelihood());
            UNIT_ASSERT_VALUES_EQUAL(149, stats.AverageSourceSegment());

            stats.Add(TDaterDate(2009, 1, 0, TDaterDate::FromUrlId));

            stats.Add(TDaterDate(2008, 1, 2, TDaterDate::FromUrlId));
            stats.Add(TDaterDate(0, 1, 2, TDaterDate::FromUrlId, false, false, true));

            stats.Add(TDaterDate(2007, 0, 0, TDaterDate::FromUrlId));
            stats.Add(TDaterDate(2000, 1, 0, TDaterDate::FromContent));

            stats.Add(TDaterDate(1917));
            stats.Add(TDaterDate(2008));

            TString y = stats.ToStringYears();
            TString my = stats.ToStringMonthsYears();
            TString dm = stats.ToStringDaysMonths();
            UNIT_ASSERT_VALUES_EQUAL_C(1u, stats.CountYears(TDaterDate::FromFooter, 2009, TDaterDate::ModeFull), stats.ToStringYears());
            UNIT_ASSERT_VALUES_EQUAL_C(2u, stats.CountYears(TDaterDate::FromUrlId, 2009, TDaterDate::ModeFull), stats.ToStringYears());
            UNIT_ASSERT_VALUES_EQUAL_C(1u, stats.CountYears(TDaterDate::FromUrlId, 2009, TDaterDate::ModeNoMonth), stats.ToStringYears());
            UNIT_ASSERT_VALUES_EQUAL_C(1u, stats.CountYears(TDaterDate::FromUrlId, 2009, TDaterDate::ModeNoDay), stats.ToStringYears());
            UNIT_ASSERT_VALUES_EQUAL_C(1u, stats.CountYears(TDaterDate::FromUrlId, 2008, TDaterDate::ModeFull), stats.ToStringYears());
            UNIT_ASSERT_VALUES_EQUAL_C(1u, stats.CountYears(TDaterDate::FromUrlId, 2007, TDaterDate::ModeFull), stats.ToStringYears());
            UNIT_ASSERT_VALUES_EQUAL_C(1u, stats.CountYears(TDaterDate::FromUrlId, 2007, TDaterDate::ModeNoMonth), stats.ToStringYears());
            UNIT_ASSERT_VALUES_EQUAL_C(1u, stats.CountYears(TDaterDate::FromUnknown, 2008, TDaterDate::ModeNoMonth), stats.ToStringYears());
            UNIT_ASSERT_VALUES_EQUAL_C(2009u, stats.MaxYear(TDaterDate::FromUrlId), stats.ToStringYears());
            UNIT_ASSERT_VALUES_EQUAL_C(2007u, stats.MinYear(TDaterDate::FromUrlId), stats.ToStringYears());
            UNIT_ASSERT_VALUES_EQUAL_C(2000u, stats.MaxYear(TDaterDate::FromContent), stats.ToStringYears());
            UNIT_ASSERT_VALUES_EQUAL_C((ui32)TDaterDate::ErfZeroYear, stats.MaxYear(TDaterDate::FromUrl), stats.ToStringYears());
            UNIT_ASSERT_VALUES_EQUAL_C(3u, stats.CountUniqYears(TDaterDate::FromUrlId), stats.ToStringYears());
            UNIT_ASSERT_VALUES_EQUAL_C(1u, stats.CountUniqYears(TDaterDate::FromFooter), stats.ToStringYears());
            UNIT_ASSERT_VALUES_EQUAL_C(1u, stats.CountUniqYears(TDaterDate::FromContent), stats.ToStringYears());
            UNIT_ASSERT_VALUES_EQUAL_C(0u, stats.CountDaysMonths(TDaterDate::FromUrlId, 1, 2), stats.ToStringDaysMonths());
            UNIT_ASSERT_VALUES_EQUAL_C(5u, stats.CountDaysMonths(TDaterDate::FromUrlId, 2, 1), stats.ToStringDaysMonths());
            UNIT_ASSERT_VALUES_EQUAL_C(0u, stats.CountMonthsYears(TDaterDate::FromUrlId, 2, 2009), stats.ToStringMonthsYears());
            UNIT_ASSERT_VALUES_EQUAL_C(3u, stats.CountMonthsYears(TDaterDate::FromUrlId, 1, 2009), stats.ToStringMonthsYears());

            {
                TDaterStats stats1;
                stats1.FromString(y);
                stats1.FromString(my);
                stats1.FromString(dm);
                UNIT_ASSERT_VALUES_EQUAL(y, "?.__Yd: 2008=1; B._MYd: 2000=1; F.DMYd: 2009=1; I.DMYd: 2007=1, 2008=1, 2009=2; I._MYd: 2009=1; I.__Yd: 2007=1, 2009=1; ");
                UNIT_ASSERT_VALUES_EQUAL(my, "?.__Yd: 00.2008=1; B._MYd: 01.2000=1; F.DMYd: 01.2009=1; I.DMYd: 01.2007=1, 01.2008=1, 01.2009=2; I._MYd: 01.2009=1; I.__Yd: 00.2007=1, 00.2009=1; ");
                UNIT_ASSERT_VALUES_EQUAL(dm, "B._MYd: 00.01=1; F.DMYd: 02.01=1; I.DMYd: 02.01=4; I._MYd: 00.01=1; I.DM_d: 02.01=1; ");
                UNIT_ASSERT_EQUAL_C(y, stats1.ToStringYears(), y + " vs " + stats1.ToStringYears());
                UNIT_ASSERT_EQUAL_C(my, stats1.ToStringMonthsYears(), my + " vs " + stats1.ToStringMonthsYears());
                UNIT_ASSERT_EQUAL_C(dm, stats1.ToStringDaysMonths(), dm + " vs " + stats1.ToStringDaysMonths());
            }
            {
                TDaterStats stats1;
                stats1.FromString(y + my + dm);
                UNIT_ASSERT_EQUAL_C(y, stats1.ToStringYears(), y + " vs " + stats1.ToStringYears());
                UNIT_ASSERT_EQUAL_C(my, stats1.ToStringMonthsYears(), my + " vs " + stats1.ToStringMonthsYears());
                UNIT_ASSERT_EQUAL_C(dm, stats1.ToStringDaysMonths(), dm + " vs " + stats1.ToStringDaysMonths());
            }
        }

        {
            const char* st = "Y.DMYD: 2010=25, 2011=2; Y.DMYd: 2010=1; Y.__YD: 2007=1, 2010=4, 2011=3;";
            TDaterStats stats;
            stats.FromString(st);
            UNIT_ASSERT_VALUES_EQUAL_C(0u, stats.CountYears(TDaterDate::FromMainContent, 2010, TDaterDate::ModeFull), stats.ToStringYears());
            UNIT_ASSERT_VALUES_EQUAL_C(0u, stats.CountYears(TDaterDate::FromContent, 2010, TDaterDate::ModeFull), stats.ToStringYears());
            UNIT_ASSERT_VALUES_EQUAL_C(26u, stats.CountYears(TDaterDate::FromText, 2010, TDaterDate::ModeFull), stats.ToStringYears());
            UNIT_ASSERT_VALUES_EQUAL_C(4u, stats.CountYears(TDaterDate::FromText, 2010, TDaterDate::ModeNoMonth), stats.ToStringYears());
        }

        {
            const char* st = "T.DMYd: 2011=1; Y.DMYd: 2010=2; Y.YD: 2005=1, 2011=2; M._MYW: 2010=1; M.YD: 2009=1, 2010=1;";
            TDaterStats stats;
            stats.FromString(st);
            UNIT_ASSERT_VALUES_EQUAL_C(0u, stats.CountYears(TDaterDate::FromMainContent, 2010, TDaterDate::ModeFull), stats.ToStringYears());
            UNIT_ASSERT_VALUES_EQUAL_C(0u, stats.CountYears(TDaterDate::FromContent, 2010, TDaterDate::ModeFull), stats.ToStringYears());
            UNIT_ASSERT_VALUES_EQUAL_C(2u, stats.CountYears(TDaterDate::FromText, 2010, TDaterDate::ModeFull), stats.ToStringYears());
            UNIT_ASSERT_VALUES_EQUAL_C(1u, stats.CountYears(TDaterDate::FromMainContent, 2010, TDaterDate::ModeNoDay), stats.ToStringYears());
            UNIT_ASSERT_VALUES_EQUAL_C(2u, stats.CountYears(TDaterDate::FromMainContent, 2010), stats.ToStringYears());
            UNIT_ASSERT_VALUES_EQUAL_C(0u, stats.CountYears(TDaterDate::FromContent, 2010), stats.ToStringYears());
            UNIT_ASSERT_VALUES_EQUAL_C(2u, stats.CountYears(TDaterDate::FromText, 2010), stats.ToStringYears());
            UNIT_ASSERT_VALUES_EQUAL_C(0u, stats.CountYears(TDaterDate::FromTitle, 2010), stats.ToStringYears());
            UNIT_ASSERT_VALUES_EQUAL_C(2011u, stats.MaxYear(TDaterDate::FromTitle), stats.ToStringYears());
            UNIT_ASSERT_VALUES_EQUAL_C(2011u, stats.MinYear(TDaterDate::FromTitle), stats.ToStringYears());
        }
        {
            const char* st = "T.__YD: 2010=1; Y._MYW: 2007=5, 2008=11, 2009=10, 2010=6, 2011=1; Y.__YD: 2008=1, 2010=2, 2011=1; M.DMYD: 2010=1; M.__YD: 2010=1; ";
            TDaterStats stats;
            stats.FromString(st);
            UNIT_ASSERT_VALUES_EQUAL_C(2010u, stats.MaxYear(TDaterDate::FromTitle), stats.ToStringYears());
            UNIT_ASSERT_VALUES_EQUAL_C(2010u, stats.MinYear(TDaterDate::FromTitle), stats.ToStringYears());
            UNIT_ASSERT_VALUES_EQUAL_C((ui32)TDaterDate::ErfZeroYear, stats.MaxYear(TDaterDate::FromUrl), stats.ToStringYears());
            UNIT_ASSERT_VALUES_EQUAL_C((ui32)TDaterDate::ErfZeroYear, stats.MinYear(TDaterDate::FromUrl), stats.ToStringYears());
        }
    }
};
UNIT_TEST_SUITE_REGISTRATION(TDateScannerTest)
