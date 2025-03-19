#include <kernel/dater/dater.h>
#include <kernel/dater/dater_simple.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/string_utils/relaxed_escaper/relaxed_escaper.h>
#include <util/string/vector.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <util/stream/file.h>
#include <util/string/cast.h>

namespace ND2 {

using namespace std::string_view_literals;

// This piece of code is for reproducing memory corruption
void RemoveDates(ND2::TDater& Dater, TUtf16String* text) {
    Dater.InputText = *text;
    Dater.InputTokenPositions.clear();
    Dater.OutputDates.clear();
    size_t startPos = 0;
    size_t wordIdx = 1;
    while (startPos < text->size()) {
        size_t endPos = text->find(L' ', startPos);
        if (endPos == TUtf16String::npos) {
            endPos = text->size();
        }
        if (startPos == endPos) {
            ++startPos;
            continue;
        }
        Dater.InputTokenPositions.push_back(NSegm::TPosCoord(NSegm::TAlignedPosting(1, wordIdx), startPos, endPos));
        startPos = endPos + 1;
        ++wordIdx;
    }
    Dater.Scan();
    const ND2::TDates& dates = Dater.OutputDates;
    for (ND2::TDates::const_reverse_iterator it = dates.rbegin(); it != dates.rend(); ++it) {
        if (it->Begin.Word() < 1 || it->Begin.Word() > Dater.InputTokenPositions.size() ||
            it->End.Word() < 2 || it->End.Word() > Dater.InputTokenPositions.size() + 1)
        {
            continue;
        }
        size_t begPos = Dater.InputTokenPositions[it->Begin.Word() - 1].Begin;
        size_t endPos = Dater.InputTokenPositions[it->End.Word() - 2].End;
        text->erase(begPos, endPos - begPos);
    }
}

void SetBaseline(ND2::TDater& Dater, const TString& UserRequest) {
    Dater.Clear();
    TUtf16String userRequest = UTF8ToWide(UserRequest);
    RemoveDates(Dater, &userRequest);
}


struct TTestContext {
    TDaterDocumentContext Ctx;
    NImpl::TNormTextConcatenator Concatenator;
    NImpl::TScanner Scanner;

    NImpl::TChunks HostChunks;
    NImpl::TChunks PathChunks;
    NImpl::TChunks QueryChunks;
    NImpl::TChunks TextChunks;

    TUtf16String Host;
    TUtf16String Path;
    TUtf16String Query;
    TUtf16String RawText;
    TUtf16String Text;

    void SetLang(ELanguage lang) {
        Ctx.MainLanguage = lang;
    }

    void SetUrl(TStringBuf url) {
        Concatenator.Normalizer.SetLanguages(Ctx.MainLanguage, Ctx.AuxLanguage);

        Ctx.Url.SetUrl(url);
        Concatenator.DoNormalize(Ctx.Url.Host, Host);
        Concatenator.DoNormalize(Ctx.Url.Path, Path);
        Concatenator.DoNormalize(Ctx.Url.Query, Query);

        Scanner.SetContext(&Ctx, &HostChunks, Host);
        Scanner.ScanHost();

        Scanner.SetContext(&Ctx, &PathChunks, Path);
        Scanner.PreScanUrl();
        Scanner.ScanPath();

        Scanner.SetContext(&Ctx, &QueryChunks, Query);
        Scanner.PreScanUrl();
        Scanner.ScanQuery();
    }

    void SetText(TStringBuf text) {
        Concatenator.Normalizer.SetLanguages(Ctx.MainLanguage, Ctx.AuxLanguage);

        UTF8ToWide(text, RawText);
        Concatenator.DoNormalize(RawText, Text);
        Text.prepend(' ');
        Scanner.SetContext(&Ctx, &TextChunks, Text);
        Scanner.ScanText(true);
    }

    void Clear() {
        Host.clear();
        Path.clear();
        Query.clear();
        RawText.clear();
        Text.clear();
        HostChunks.clear();
        PathChunks.clear();
        QueryChunks.clear();
        TextChunks.clear();
        Ctx.Clear();
    }
};

typedef TVector<TStringBuf> TStringBufs;
}

Y_UNIT_TEST_SUITE(TDater2ScannerTest) {

    Y_UNIT_TEST(TestAtoi) {
        UNIT_ASSERT_VALUES_EQUAL(143353580ul, ND2::NImpl::Atoi("143353580"));
        UNIT_ASSERT_VALUES_EQUAL(ui64(-1), ND2::NImpl::Atoi("14afa"));
    }

    void DloadTest(ND2::TTestContext& t, bool expecturldload) {
        using namespace ND2;

        UNIT_ASSERT_VALUES_EQUAL_C(t.Ctx.Flags.UrlHasDownloadPattern, expecturldload, t.Ctx.Url.RawUrl.data());

        t.Clear();
    }

    Y_UNIT_TEST(TestDload) {
        using namespace ND2;
        TTestContext t;

        t.SetUrl("http://yandex.ru/download");
        DloadTest(t, true);

        t.SetUrl("http://yandex.ru/dloadme");
        DloadTest(t, true);

        t.SetUrl("http://yandex.ru/sezon");
        DloadTest(t, false);

        t.SetLang(LANG_RUS);
        t.SetUrl("http://yandex.ru/sezon");
        DloadTest(t, true);

        t.SetUrl("http://yandex.ru/%D0%A1%D0%B5%D0%B7%D0%BE%D0%BD");
        DloadTest(t, false);

        t.SetLang(LANG_RUS);
        t.SetUrl("http://yandex.ru/%D0%A1%D0%B5%D0%B7%D0%BE%D0%BD");
        DloadTest(t, true);

        t.SetUrl("http://yandex.ru/texe");
        DloadTest(t, false);

        t.SetUrl("http://yandex.ru/exec");
        DloadTest(t, false);

        t.SetUrl("http://yandex.ru/exe");
        DloadTest(t, false);

        t.SetUrl("http://yandex.ru/t.exe");
        DloadTest(t, true);
    }

    void CompareChunks(const ND2::NImpl::TChunks& chs, const ND2::TStringBufs& vals, TStringBuf marker) {
        using namespace NEscJ;
        UNIT_ASSERT_VALUES_EQUAL_C(chs.size(), vals.size(),
                                   (EscapeJ<true>(marker) + " :\n" +
                                     EscapeJ<false>(JoinStrings(chs.begin(), chs.end(), "\n"), "\n") + "\n!=\n" +
                                     EscapeJ<false>(JoinStrings(vals.begin(), vals.end(), "\n"), "\n")).data());

        for (ui32 i = 0; i < vals.size(); ++i)
            UNIT_ASSERT_EQUAL_C(chs[i].ToString(), ToString(vals[i]),
                                       (EscapeJ<true>(marker) + " :\n" +
                                         EscapeJ<false>(chs[i].ToString()) + "\n!=\n" +
                                         EscapeJ<false>(vals[i])).data());
    }

    void UrlDateTest(ND2::TTestContext& t,
                     const ND2::TStringBufs& path = ND2::TStringBufs(),
                     const ND2::TStringBufs& query = ND2::TStringBufs(),
                     const ND2::TStringBufs& host = ND2::TStringBufs() ) {
        using namespace ND2;

        TStringBuf rawurl = t.Ctx.Url.RawUrl;

        CompareChunks(t.HostChunks, host, rawurl);
        CompareChunks(t.PathChunks, path, rawurl);
        CompareChunks(t.QueryChunks, query, rawurl);

        t.Clear();
    }

    void TextDateTest(ND2::TTestContext& t, TStringBuf text, ELanguage lang = LANG_RUS,
                      const ND2::TStringBufs& chunks = ND2::TStringBufs()) {
        using namespace ND2;
        t.SetLang(lang);
        t.SetText(text);

        CompareChunks(t.TextChunks, chunks, text);

        t.Clear();
    }

    void Ct(ND2::TStringBufs& bufs, TStringBuf b) {
        if (!!b)
            bufs.push_back(b);
    }

    void Ct(ND2::TStringBufs& bufs, TStringBuf b0, TStringBuf b1) {
        Ct(bufs, b0);
        Ct(bufs, b1);
    }

    void Ct(ND2::TStringBufs& bufs, TStringBuf b0, TStringBuf b1, TStringBuf b2, TStringBuf b3) {
        Ct(bufs, b0, b1);
        Ct(bufs, b2, b3);
    }

    ND2::TStringBufs Ct(TStringBuf ch0 = "", TStringBuf ch1 = "", TStringBuf ch2 = "", TStringBuf ch3 = "",
                              TStringBuf ch4 = "", TStringBuf ch5 = "", TStringBuf ch6 = "", TStringBuf ch7 = "",
                              TStringBuf ch8 = "", TStringBuf ch9 = "", TStringBuf chA = "", TStringBuf chB = "",
                              TStringBuf chC = "", TStringBuf chD = "", TStringBuf chE = "", TStringBuf chF = "") {
        using namespace ND2;
        TStringBufs bufs;
        Ct(bufs, ch0, ch1, ch2, ch3);
        Ct(bufs, ch4, ch5, ch6, ch7);
        Ct(bufs, ch8, ch9, chA, chB);
        Ct(bufs, chC, chD, chE, chF);

        return bufs;
    }

    Y_UNIT_TEST(TestUrlScan) {
        using namespace ND2;
        TTestContext t;

        t.SetUrl("http://www2009.yandex.ru");
        UrlDateTest(t, Ct(), Ct(),
                    Ct("[w2009.] {w} {2009}.#.Y {.}"sv));

        t.SetUrl("http://208.yandex.ru");
        UrlDateTest(t);

        t.SetUrl("http://w208.yandex.ru");
        UrlDateTest(t);

        t.SetUrl("http://w20008.yandex.ru");
        UrlDateTest(t);

        t.SetUrl("http://20008.yandex.ru");
        UrlDateTest(t);

        t.SetUrl("http://lenta.ru/09/10/11/");
        UrlDateTest(t,
                    Ct("[/09/10/11/] {/} {09}.#.Y {/} {10}.#.M {/} {11}.#.D {/}"sv));

        t.SetUrl("http://lenta.ru/09/10/11");
        UrlDateTest(t,
                    Ct("[/09/10/11\0] {/} {09}.#.Y {/} {10}.#.M {/} {11}.#.D {\0}"sv));

        t.SetUrl("http://lenta.ru/09/10/11?");
        UrlDateTest(t,
                    Ct("[/09/10/11\0] {/} {09}.#.Y {/} {10}.#.M {/} {11}.#.D {\0}"sv));

        t.SetUrl("http://lenta.ru/09/10/11#");
        UrlDateTest(t,
                    Ct("[/09/10/11\0] {/} {09}.#.Y {/} {10}.#.M {/} {11}.#.D {\0}"sv));

        t.SetUrl("http://2009.lenta.ru/2009/10/11?");
        UrlDateTest(t,
                    Ct("[/2009/10/11\0] {/} {2009}.#.Y {/} {10}.#.M {/} {11}.#.D {\0}"sv),
                    Ct(),
                    Ct("[2009.] {2009}.#.Y {.}"sv));

        t.SetUrl("http://lenta.ru/79/10/11?");
        UrlDateTest(t,
                    Ct("[/79/10/11\0] {/79/10/11\0}.J"sv));

        t.SetUrl("http://09-10-11.ru");
        UrlDateTest(t);

        t.SetUrl("http://lenta.ru/2009/10/11/08.07.06");
        UrlDateTest(t,
                    Ct("[/2009/10/11/] {/} {2009}.#.Y {/} {10}.#.M {/} {11}.#.D {/}"sv,
                       "[/08.07.06\0] {/} {08}.#.D {.} {07}.#.M {.} {06}.#.Y {\0}"sv));

        t.SetUrl("http://lenta.ru/2009/10/11?08.7.6");
        UrlDateTest(t,
                    Ct("[/2009/10/11\0] {/} {2009}.#.Y {/} {10}.#.M {/} {11}.#.D {\0}"sv));

        t.SetUrl("http://lenta.ru/79/10/11?08.7.6");
        UrlDateTest(t,
                    Ct("[/79/10/11\0] {/79/10/11\0}.J"sv));

        t.SetUrl("http://lenta.ru/79/10/11?08-7.6");
        UrlDateTest(t,
                    Ct("[/79/10/11\0] {/79/10/11\0}.J"sv));

        t.SetUrl("http://lenta.ru/79/10/11?08-7.6#08.07.06");
        UrlDateTest(t,
                    Ct("[/79/10/11\0] {/79/10/11\0}.J"sv));

        t.SetUrl("http://lenta.ru/20091011");
        UrlDateTest(t,
                    Ct("[/20091011\0] {/} {2009}.#.Y {10}.#.M {11}.#.D {\0}"sv));

        t.SetUrl("http://lenta.ru/091011");
        UrlDateTest(t);

        t.SetUrl("http://lenta.ru/0091011");
        UrlDateTest(t);

        t.SetUrl("http://lenta.ru/09may99");
        UrlDateTest(t,
                    Ct("[/09may99\0] {/} {09}.#.D {may}.@.M {99}.#.Y {\0}"sv));

        t.SetUrl("http://lenta.ru/may99/test");
        UrlDateTest(t,
                    Ct("[/may99/] {/} {may}.@.M {99}.#.Y {/}"sv));

        t.SetUrl("http://lenta.ru/may/99/test");
        UrlDateTest(t,
                    Ct("[/may/99/] {/} {may}.@.M {/} {99}.#.Y {/}"sv));

        t.SetUrl("http://lenta.ru/09/may/test");
        UrlDateTest(t,
                    Ct("[/09/may/] {/} {09}.#.Y {/} {may}.@.M {/}"sv));

        t.SetUrl("http://www.SeoPro.ru/news/2009/2/114.html");
        UrlDateTest(t,
                    Ct("[/2009/2/] {/} {2009}.#.Y {/} {2}.#.M {/}"sv));

        t.SetUrl("http://new.gpsclub.tomsk.ru/gps/dat120520092254/index.html");
        UrlDateTest(t,
                    Ct("[t120520092254/] {t} {12}.#.D {05}.#.M {2009}.#.Y {2254} {/}"sv));

        t.SetUrl("http://subj.us/20070830/luzhkov_ostavil_cherkizovski_diaspore_gorskih_evreev.html");
        UrlDateTest(t,
                    Ct("[/20070830/] {/} {2007}.#.Y {08}.#.M {30}.#.D {/}"sv));

        t.SetUrl("http://www.bcc.ru/press/publishing/pub02/pub20021213.html");
        UrlDateTest(t,
                    Ct("[b20021213.] {b} {2002}.#.Y {12}.#.M {13}.#.D {.}"sv));

        t.SetUrl("http://www.wc3life.com/load/15-1-0-386");
        UrlDateTest(t,
                    Ct("[/15-1-0-386\0] {/15-1-0-386\0}.J"sv));

        t.SetUrl("http://www.groklaw.net/articlebasic.php?story=20080829221757478");
        UrlDateTest(t,
                    Ct(),
                    Ct("[=20080829221757478\0] {=} {2008}.#.Y {08}.#.M {29}.#.D {221757478} {\0}"sv));

        t.SetUrl("http://www.vedomosti.ru/newspaper/article.shtml?2008/09/16/161264");
        UrlDateTest(t,
                    Ct(),
                    Ct("[?2008/09/16/] {?} {2008}.#.Y {/} {09}.#.M {/} {16}.#.D {/}"sv));

        t.SetUrl("http://www.cnews.ru/news/line/index.shtml?2007/11/06/273448");
        UrlDateTest(t,
                    Ct(),
                    Ct("[?2007/11/06/] {?} {2007}.#.Y {/} {11}.#.M {/} {06}.#.D {/}"sv));

        t.SetUrl("http://digitalshop.ru/shop/?action=print_info_tovar&id_podrazdel=98&id_tovar=2000902");
        UrlDateTest(t);

        t.SetUrl("http://kkbweb.borda.ru/?1-5-0-00000003-000-0-0");
        UrlDateTest(t,
                    Ct(),
                    Ct("[?1-5-0-00000003-000-0-0\0] {?1-5-0-00000003-000-0-0\0}.J"sv));

        t.SetUrl("http://megalife.com.ua/2007/09/12/fraps_10.9.21__novaja_versija.html");
        UrlDateTest(t,
                    Ct("[/2007/09/12/] {/} {2007}.#.Y {/} {09}.#.M {/} {12}.#.D {/}"sv,
                       "[_10.9.21_] {_} {10}.#.D {.} {9}.#.M {.} {21}.#.Y {_}"sv));

        t.SetUrl("vp-site.net/2009/05/12/publichnye-izvrashhency-porno-onlajn.html");
        UrlDateTest(t,
                    Ct("[/2009/05/12/] {/} {2009}.#.Y {/} {05}.#.M {/} {12}.#.D {/}"sv));

        t.SetUrl("vp-site.net?2008-10-03/2009/05/12/publichnye-izvrashhency-porno-onlajn.html");
        UrlDateTest(t,
                    Ct(),
                    Ct("[?2008-10-03/] {?} {2008}.#.Y {-} {10}.#.M {-} {03}.#.D {/}"sv,
                       "[/2009/05/12/] {/} {2009}.#.Y {/} {05}.#.M {/} {12}.#.D {/}"sv));

        t.SetUrl("http://pro-witch.ucoz.ru/forum/2-1-1");
        UrlDateTest(t,
                    Ct("[/2-1-1\0] {/2-1-1\0}.J"sv));

        t.SetUrl("http://xx-xx.ucoz.ru/publ/2-1-0-12");
        UrlDateTest(t,
                    Ct("[/2-1-0-12\0] {/2-1-0-12\0}.J"sv));

        t.SetUrl("http://investigl.3dn.ru/publ/10-10-9-10");
        UrlDateTest(t,
                    Ct("[/10-10-9-10\0] {/10-10-9-10\0}.J"sv));

        t.SetUrl("www.airwar.ru/enc/fighter/j10.html");
        UrlDateTest(t);

        t.SetUrl("http://www.ixbt.com/news/all/index.shtml?11/17/06");
        UrlDateTest(t,
                    Ct(),
                    Ct("[?11/17/06\0] {?} {11}.#.M {/} {17}.#.D {/} {06}.#.Y {\0}"sv));

        t.SetUrl("www.mignews.com/news/society/world/041207_112526_96429.html");
        UrlDateTest(t);

        t.SetUrl("http://www.b2b-sng.ru/firms/index.html?show=buyers&f_country=643_4&cat_id=10100000");
        UrlDateTest(t,
                    Ct(),
                    Ct("[=10100000\0] {=10100000\0}.J"sv));

        t.SetUrl("www.garweb.ru/conf/ks/20030129/smi/msg.asp@id_msg130044.htm");
        UrlDateTest(t,
                    Ct("[/20030129/] {/} {2003}.#.Y {01}.#.M {29}.#.D {/}"sv));

        t.SetUrl("an-doki.blogspot.com/2009_04_01_archive.html");
        UrlDateTest(t,
                    Ct("[/2009_04_01_] {/} {2009}.#.Y {_} {04}.#.M {_} {01}.#.D {_}"sv));

        t.SetUrl("http://www.rian.ru/jpquake_effect/20110312/346073494.html");
        UrlDateTest(t,
                    Ct("[/20110312/] {/} {2011}.#.Y {03}.#.M {12}.#.D {/}"sv,
                       "[/346073494.] {/346073494.}.J"sv));

        t.SetUrl("polprav.blogspot.com/2009/09/12.html");
        UrlDateTest(t,
                    Ct("[/2009/09/12.] {/} {2009}.#.Y {/} {09}.#.M {/} {12}.#.D {.}"sv));

        t.SetUrl("http://copypast.ru/images/5/1991/alcohol_022.html");
        UrlDateTest(t,
                    Ct("[/1991/] {/1991/}.J"sv));

        t.SetUrl("www.pixiome.fr/2009/11/20-examples-of-origami-inspired-logo-designs/");
        UrlDateTest(t,
                    Ct("[/2009/11/20-] {/} {2009}.#.Y {/} {11}.#.M {/} {20}.#.D {-}"sv));

        t.SetUrl("www.softitem.com/Windows/detail-download-w9x552d4-exe-1995-12-31-10187088.html");
        UrlDateTest(t,
                    Ct("[-1995-12-31-] {-1995-12-31-}.J"sv,
                       "[-10187088.] {-10187088.}.J"sv));

        t.SetUrl("www.fullreleases.ws/free-full-download-pgware-pcmedik-6.7.7.2008-crack-serial-keygen-torrent.html");
        UrlDateTest(t,
                    Ct("[-6.7.7.] {-6.7.7.}.J"sv,
                       "[.2008-] {.} {2008}.#.Y {-}"sv));

        t.SetUrl("greatfreesite.ru/19488-va-global-dance-hits-vol-2-2010.html");
        UrlDateTest(t,
                    Ct("[-2010.] {-} {2010}.#.Y {.}"sv));

        t.SetUrl("greatfreesite.ru/6286-vselennaya-the-universe-season-1-2007-hdtv-720p.html");
        UrlDateTest(t,
                    Ct("[/6286-] {/6286-}.J"sv,
                       "[-2007-] {-} {2007}.#.Y {-}"sv));

        t.SetUrl("worldforfree.net/music/1146232380-deep-balearic-best-of-vol-1-2010.html");
        UrlDateTest(t,
                    Ct("[/1146232380-] {/1146232380-}.J"sv,
                       "[-2010.] {-} {2010}.#.Y {.}"sv));

        t.SetUrl("https://www.hamilton.ca/NR/rdonlyres/239C5663-8820-4615-BEB0-A952A3001CF5/0/Jun14EDRMS_n89836_v1_8_5__PW05095c.pdf");
        UrlDateTest(t,
                    Ct("[-8820-] {-8820-}.J"sv,
                       "[-4615-] {-4615-}.J"sv));

        t.SetUrl("replicawatches1.blogge.rs/2010/10/07/carbon-prince-on-your-watch/");
        UrlDateTest(t,
                    Ct("[/2010/10/07/] {/} {2010}.#.Y {/} {10}.#.M {/} {07}.#.D {/}"sv));

        t.SetUrl("moyteremok.ru/archives/date/2010/03/14");
        UrlDateTest(t,
                    Ct("[/2010/03/14\0] {/} {2010}.#.Y {/} {03}.#.M {/} {14}.#.D {\0}"sv));

        t.SetUrl("rybakov.spb.ru/stat/sost_jul_06.htm");
        UrlDateTest(t,
                    Ct("[_jul_06.] {_} {jul}.@.M {_} {06}.#.Y {.}"sv));

        t.SetUrl("http://www.2x2.su/kpamur/text/10_02_06/");
        UrlDateTest(t,
                    Ct("[/10_02_06/] {/} {10}.#.D {_} {02}.#.M {_} {06}.#.Y {/}"sv));

        t.SetUrl("www.slavcenter.ge/trn/?p=20060810-061546");
        UrlDateTest(t,
                    Ct(),
                    Ct("[=20060810-] {=} {2006}.#.Y {08}.#.M {10}.#.D {-}"sv));

        t.SetUrl("http://www.pravda.ru/news/showbiz/24-01-2011/1064468-moiseev-0/");
        UrlDateTest(t,
                    Ct("[/24-01-2011/] {/} {24}.#.D {-} {01}.#.M {-} {2011}.#.Y {/}"sv));

        t.SetUrl("http://obozrevatel.com/news/2009/3/30/294659.htm");
        UrlDateTest(t,
                    Ct("[/2009/3/30/] {/} {2009}.#.Y {/} {3}.#.M {/} {30}.#.D {/}"sv));

        t.SetUrl("http://www.lepestok.kharkov.ua/bio/s20100302.htm");
        UrlDateTest(t,
                    Ct("[s20100302.] {s} {2010}.#.Y {03}.#.M {02}.#.D {.}"sv));

        t.SetUrl("http://www.belspravka.ru/screen/1/directory?fid=500000010227");
        UrlDateTest(t,
                    Ct(),
                    Ct("[=500000010227\0] {=500000010227\0}.J"sv));

        t.SetUrl("http://travel.mail.ru/?mod=companions&res=1&geo_id=1460&date1=04.09.2007&date2=29.08.2008");
        UrlDateTest(t,
                    Ct(),
                    Ct("[=1460&] {=1460&}.J"sv,
                       "[=04.09.2007&] {=} {04}.#.D {.} {09}.#.M {.} {2007}.#.Y {&}"sv,
                       "[=29.08.2008\0] {=} {29}.#.D {.} {08}.#.M {.} {2008}.#.Y {\0}"sv));

        t.SetUrl("http://www.prodindustry.ru/archive/2005/september/0002.php");
        UrlDateTest(t,
                    Ct("[/2005/september/] {/} {2005}.#.Y {/} {september}.@.M {/}"sv,
                       "[/0002.] {/0002.}.J"sv));

        t.SetUrl("http://www.cher.su/13.04.2011/prichinu_avarijnoj_posadki_tu__142_mr_vyjasnit_prokuratura/");
        UrlDateTest(t,
                    Ct("[/13.04.2011/] {/} {13}.#.D {.} {04}.#.M {.} {2011}.#.Y {/}"sv));

        t.SetUrl("http://videoefir.ru/serial/5003-druzya-sezon-1-5-1994-1999-dvdrip.html");
        UrlDateTest(t,
                    Ct("[/5003-] {/5003-}.J"sv,
                       "[-1-5-1994-] {-1-5-1994-}.J"sv,
                       "[-1999-] {-} {1999}.#.Y {-}"sv));

        t.SetUrl("http://www.freedomscientific.com/fs_news/July-August2004_newsletter.asp");
        UrlDateTest(t,
                    Ct("[/july-august2004_] {/} {july}.@.M {-} {august}.@.M {2004}.#.Y {_}"sv));

        t.SetUrl("http://www.stallman.org/archives/2009-mar-jun.html");
        UrlDateTest(t,
                    Ct("[/2009-mar-jun.] {/} {2009}.#.Y {-} {mar}.@.M {-} {jun}.@.M {.}"sv));

        t.SetUrl("http://www.spiritccca.com/sitebuildercontent/sitebuilderfiles/sparkplug_may-june2009_article.pdf");
        UrlDateTest(t,
                    Ct("[_may-june2009_] {_} {may}.@.M {-} {june}.@.M {2009}.#.Y {_}"sv));

        t.SetUrl("http://yandex.ru/1%D0%BC%D0%B0%D1%8F2011");
        UrlDateTest(t);

        t.SetLang(LANG_RUS);
        t.SetUrl("http://yandex.ru/1%D0%BC%D0%B0%D1%8F2011");
        UrlDateTest(t,
                    Ct("[/1мая2011\0] {/} {1}.#.D {мая}.@.M {2011}.#.Y {\0}"sv));

        t.SetUrl("http://qiq.ru/16/02/2009/books/96733/vjacheslav_mescherjakov___trening_mozga_deystvennyy_metod_transformatsii_soznanija_psixologija_ocr_bez_oshibok.html");
        UrlDateTest(t,
                    Ct("[/16/02/2009/] {/} {16}.#.D {/} {02}.#.M {/} {2009}.#.Y {/}"sv));

        t.SetUrl("http://www.sobor.by/idnews.php?id=2009-Apr-1");
        UrlDateTest(t,
                    Ct(),
                    Ct("[=2009-apr-1\0] {=} {2009}.#.Y {-} {apr}.@.M {-} {1}.#.D {\0}"sv));

        t.SetUrl("http://www.sobor.by/idnews.php?id=2009-Apr-1-22:53:20");
        UrlDateTest(t,
                    Ct(),
                    Ct("[=2009-apr-1-] {=} {2009}.#.Y {-} {apr}.@.M {-} {1}.#.D {-}"sv));

        t.SetUrl("http://www.bbc.co.uk/russian/rolling_news/2014/02/140203_rn_moscow_school_end.shtml");
        UrlDateTest(t,
                    Ct("[/2014/02/] {/} {2014}.#.Y {/} {02}.#.M {/}"sv));

        t.SetUrl("http://test.com/06042015");
        UrlDateTest(t, Ct("[/06042015\0] {/} {06}.#.D {04}.#.M {2015}.#.Y {\0}"sv));

        t.SetUrl("http://test.com/0604201523");
        UrlDateTest(t, Ct("[/0604201523\0] {/} {06}.#.D {04}.#.M {2015}.#.Y {23} {\0}"sv));

        t.SetUrl("http://test.com/4406042015");
        UrlDateTest(t, Ct("[/4406042015\0] {/} {44} {06}.#.D {04}.#.M {2015}.#.Y {\0}"sv));
    }

    Y_UNIT_TEST(TestMonthNames) {
        using namespace ND2;
        TTestContext t;

        TextDateTest(t, "08 студзень,      08 студз,    08 стд, 08 стдз, 08 Студзеня"
                        ", 08 люты,          08 лют,      08 лютага"
                        ", 08 сакавiк,       08 сак,      08 Сакавіка"
                        ", 08 красавiк,      08 крас,     08 крс, 08 красавіка", LANG_BEL,
                        Ct("[ 08 cтудзень, ] { } {08}.#.D { } {cтудзень}.@.M {, }"sv,
                           "[ 08 cтудз, ] { } {08}.#.D { } {cтудз}.@.M {, }"sv,
                           "[ 08 cтд, ] { } {08}.#.D { } {cтд}.@.M {, }"sv,
                           "[ 08 cтдз, ] { } {08}.#.D { } {cтдз}.@.M {, }"sv,
                           "[ 08 cтудзеня, ] { } {08}.#.D { } {cтудзеня}.@.M {, }"sv,
                           "[ 08 люты, ] { } {08}.#.D { } {люты}.@.M {, }"sv,
                           "[ 08 лют, ] { } {08}.#.D { } {лют}.@.M {, }"sv,
                           "[ 08 лютага, ] { } {08}.#.D { } {лютага}.@.M {, }"sv,
                           "[ 08 cакавiк, ] { } {08}.#.D { } {cакавiк}.@.M {, }"sv,
                           "[ 08 cак, ] { } {08}.#.D { } {cак}.@.M {, }"sv,
                           "[ 08 cакавiка, ] { } {08}.#.D { } {cакавiка}.@.M {, }"sv,
                           "[ 08 краcавiк, ] { } {08}.#.D { } {краcавiк}.@.M {, }"sv,
                           "[ 08 краc, ] { } {08}.#.D { } {краc}.@.M {, }"sv,
                           "[ 08 крc, ] { } {08}.#.D { } {крc}.@.M {, }"sv,
                           "[ 08 краcавiка\0] { } {08}.#.D { } {краcавiка}.@.M {\0}"sv
                           ));
        TextDateTest(t, "08 травень,       08 тра,      08 траўня, 08 травня"
                        ", 08 чэрвень,       08 чэр,      08 чэрвеня, 08 червень,       08 чер,      08 червеня"
                        ", 08 лiпень,        08 лiп,      08 лпн, 08 ліпеня", LANG_BEL,
                        Ct("[ 08 травень, ] { } {08}.#.D { } {травень}.@.M {, }"sv,
                           "[ 08 тра, ] { } {08}.#.D { } {тра}.@.M {, }"sv,
                           "[ 08 травня, ] { } {08}.#.D { } {травня}.@.M {, }"sv,
                           "[ 08 травня, ] { } {08}.#.D { } {травня}.@.M {, }"sv,
                           "[ 08 червень, ] { } {08}.#.D { } {червень}.@.M {, }"sv,
                           "[ 08 чер, ] { } {08}.#.D { } {чер}.@.M {, }"sv,
                           "[ 08 червеня, ] { } {08}.#.D { } {червеня}.@.M {, }"sv,
                           "[ 08 червень, ] { } {08}.#.D { } {червень}.@.M {, }"sv,
                           "[ 08 чер, ] { } {08}.#.D { } {чер}.@.M {, }"sv,
                           "[ 08 червеня, ] { } {08}.#.D { } {червеня}.@.M {, }"sv,
                           "[ 08 лiпень, ] { } {08}.#.D { } {лiпень}.@.M {, }"sv,
                           "[ 08 лiп, ] { } {08}.#.D { } {лiп}.@.M {, }"sv,
                           "[ 08 лпн, ] { } {08}.#.D { } {лпн}.@.M {, }"sv,
                           "[ 08 лiпеня\0] { } {08}.#.D { } {лiпеня}.@.M {\0}"sv
                           ));
        TextDateTest(t, "08 жнiвень,       08 жнiв,     08 жнв, 08 жніўня, 08 жнiвня"
                        ", 08 верасень,      08 вер,      08 врс, 08 верасня, 08 верасьня"
                        ", 08 кастрычнiк,    08 каст,     08 кст, 08 Кастрычніка", LANG_BEL,
                        Ct("[ 08 жнiвень, ] { } {08}.#.D { } {жнiвень}.@.M {, }"sv,
                           "[ 08 жнiв, ] { } {08}.#.D { } {жнiв}.@.M {, }"sv,
                           "[ 08 жнв, ] { } {08}.#.D { } {жнв}.@.M {, }"sv,
                           "[ 08 жнiвня, ] { } {08}.#.D { } {жнiвня}.@.M {, }"sv,
                           "[ 08 жнiвня, ] { } {08}.#.D { } {жнiвня}.@.M {, }"sv,
                           "[ 08 вераcень, ] { } {08}.#.D { } {вераcень}.@.M {, }"sv,
                           "[ 08 вер, ] { } {08}.#.D { } {вер}.@.M {, }"sv,
                           "[ 08 врc, ] { } {08}.#.D { } {врc}.@.M {, }"sv,
                           "[ 08 вераcня, ] { } {08}.#.D { } {вераcня}.@.M {, }"sv,
                           "[ 08 вераcня, ] { } {08}.#.D { } {вераcня}.@.M {, }"sv,
                           "[ 08 каcтрычнiк, ] { } {08}.#.D { } {каcтрычнiк}.@.M {, }"sv,
                           "[ 08 каcт, ] { } {08}.#.D { } {каcт}.@.M {, }"sv,
                           "[ 08 кcт, ] { } {08}.#.D { } {кcт}.@.M {, }"sv,
                           "[ 08 каcтрычнiка\0] { } {08}.#.D { } {каcтрычнiка}.@.M {\0}"sv
                           ));
        TextDateTest(t, "08 лiстапад,      08 лiс,      08 лістапада"
                        ", 08 снежань,       08 сьнежань, 08 снеж, 08 снж, 08 Снежня, 08 сьнежня", LANG_BEL,
                     Ct("[ 08 лicтапад, ] { } {08}.#.D { } {лicтапад}.@.M {, }"sv,
                        "[ 08 лic, ] { } {08}.#.D { } {лic}.@.M {, }"sv,
                        "[ 08 лicтапада, ] { } {08}.#.D { } {лicтапада}.@.M {, }"sv,
                        "[ 08 cнежань, ] { } {08}.#.D { } {cнежань}.@.M {, }"sv,
                        "[ 08 cнежань, ] { } {08}.#.D { } {cнежань}.@.M {, }"sv,
                        "[ 08 cнеж, ] { } {08}.#.D { } {cнеж}.@.M {, }"sv,
                        "[ 08 cнж, ] { } {08}.#.D { } {cнж}.@.M {, }"sv,
                        "[ 08 cнежня, ] { } {08}.#.D { } {cнежня}.@.M {, }"sv,
                        "[ 08 cнежня\0] { } {08}.#.D { } {cнежня}.@.M {\0}"sv
                        ));

        TextDateTest(t, "08 Gener,    08 gen"
                        ", 08 Febrer,   08 feb"
                        ", 08 Març,     08 mar"
                        ", 08 Abril,    08 abr"
                        ", 08 Maig,     08 mai"
                        ", 08 Juny,     08 jun", LANG_CAT,
                        Ct("[ 08 gener, ] { } {08}.#.D { } {gener}.@.M {, }"sv,
                           "[ 08 gen, ] { } {08}.#.D { } {gen}.@.M {, }"sv,
                           "[ 08 febrer, ] { } {08}.#.D { } {febrer}.@.M {, }"sv,
                           "[ 08 feb, ] { } {08}.#.D { } {feb}.@.M {, }"sv,
                           "[ 08 marc, ] { } {08}.#.D { } {marc}.@.M {, }"sv,
                           "[ 08 mar, ] { } {08}.#.D { } {mar}.@.M {, }"sv,
                           "[ 08 abril, ] { } {08}.#.D { } {abril}.@.M {, }"sv,
                           "[ 08 abr, ] { } {08}.#.D { } {abr}.@.M {, }"sv,
                           "[ 08 maig, ] { } {08}.#.D { } {maig}.@.M {, }"sv,
                           "[ 08 mai, ] { } {08}.#.D { } {mai}.@.M {, }"sv,
                           "[ 08 juny, ] { } {08}.#.D { } {juny}.@.M {, }"sv,
                           "[ 08 jun\0] { } {08}.#.D { } {jun}.@.M {\0}"sv
                           ));
        TextDateTest(t, "08 Juliol,   08 jul"
                        ", 08 Agost,    08 ago"
                        ", 08 Setembre, 08 set"
                        ", 08 Octubre,  08 oct"
                        ", 08 Novembre, 08 nov"
                        ", 08 Desembre, 08 des", LANG_CAT,
                        Ct("[ 08 juliol, ] { } {08}.#.D { } {juliol}.@.M {, }"sv,
                           "[ 08 jul, ] { } {08}.#.D { } {jul}.@.M {, }"sv,
                           "[ 08 agost, ] { } {08}.#.D { } {agost}.@.M {, }"sv,
                           "[ 08 ago, ] { } {08}.#.D { } {ago}.@.M {, }"sv,
                           "[ 08 setembre, ] { } {08}.#.D { } {setembre}.@.M {, }"sv,
                           "[ 08 set, ] { } {08}.#.D { } {set}.@.M {, }"sv,
                           "[ 08 octubre, ] { } {08}.#.D { } {octubre}.@.M {, }"sv,
                           "[ 08 oct, ] { } {08}.#.D { } {oct}.@.M {, }"sv,
                           "[ 08 novembre, ] { } {08}.#.D { } {novembre}.@.M {, }"sv,
                           "[ 08 nov, ] { } {08}.#.D { } {nov}.@.M {, }"sv,
                           "[ 08 desembre, ] { } {08}.#.D { } {desembre}.@.M {, }"sv,
                           "[ 08 des\0] { } {08}.#.D { } {des}.@.M {\0}"sv
                           ));

        TextDateTest(t, "08 leden,    08 led, 08 ledna"
                        ", 08 únor,     08 úno, 08 února"
                        ", 08 březen,   08 bře, 08 března"
                        ", 08 duben,    08 dub, 08 dubna"
                        ", 08 květen,   08 kvě, 08 května", LANG_CZE,
                        Ct("[ 08 leden, ] { } {08}.#.D { } {leden}.@.M {, }"sv,
                           "[ 08 led, ] { } {08}.#.D { } {led}.@.M {, }"sv,
                           "[ 08 ledna, ] { } {08}.#.D { } {ledna}.@.M {, }"sv,
                           "[ 08 unor, ] { } {08}.#.D { } {unor}.@.M {, }"sv,
                           "[ 08 uno, ] { } {08}.#.D { } {uno}.@.M {, }"sv,
                           "[ 08 unora, ] { } {08}.#.D { } {unora}.@.M {, }"sv,
                           "[ 08 brezen, ] { } {08}.#.D { } {brezen}.@.M {, }"sv,
                           "[ 08 bre, ] { } {08}.#.D { } {bre}.@.M {, }"sv,
                           "[ 08 brezna, ] { } {08}.#.D { } {brezna}.@.M {, }"sv,
                           "[ 08 duben, ] { } {08}.#.D { } {duben}.@.M {, }"sv,
                           "[ 08 dub, ] { } {08}.#.D { } {dub}.@.M {, }"sv,
                           "[ 08 dubna, ] { } {08}.#.D { } {dubna}.@.M {, }"sv,
                           "[ 08 kveten, ] { } {08}.#.D { } {kveten}.@.M {, }"sv,
                           "[ 08 kve, ] { } {08}.#.D { } {kve}.@.M {, }"sv,
                           "[ 08 kvetna\0] { } {08}.#.D { } {kvetna}.@.M {\0}"sv
                           ));

        TextDateTest(t, "08 červen,   08 Čvn, 08 června"
                        ", 08 červenec, 08 Čvc, 08 července"
                        ", 08 srpen,    08 srp, 08 srpna"
                        ", 08 září,     08 zář, 08 září"
                        ", 08 říjen,    08 říj, 08 října", LANG_CZE,
                        Ct("[ 08 cerven, ] { } {08}.#.D { } {cerven}.@.M {, }"sv,
                           "[ 08 cvn, ] { } {08}.#.D { } {cvn}.@.M {, }"sv,
                           "[ 08 cervna, ] { } {08}.#.D { } {cervna}.@.M {, }"sv,
                           "[ 08 cervenec, ] { } {08}.#.D { } {cervenec}.@.M {, }"sv,
                           "[ 08 cvc, ] { } {08}.#.D { } {cvc}.@.M {, }"sv,
                           "[ 08 cervence, ] { } {08}.#.D { } {cervence}.@.M {, }"sv,
                           "[ 08 srpen, ] { } {08}.#.D { } {srpen}.@.M {, }"sv,
                           "[ 08 srp, ] { } {08}.#.D { } {srp}.@.M {, }"sv,
                           "[ 08 srpna, ] { } {08}.#.D { } {srpna}.@.M {, }"sv,
                           "[ 08 zari, ] { } {08}.#.D { } {zari}.@.M {, }"sv,
                           "[ 08 zar, ] { } {08}.#.D { } {zar}.@.M {, }"sv,
                           "[ 08 zari, ] { } {08}.#.D { } {zari}.@.M {, }"sv,
                           "[ 08 rijen, ] { } {08}.#.D { } {rijen}.@.M {, }"sv,
                           "[ 08 rij, ] { } {08}.#.D { } {rij}.@.M {, }"sv,
                           "[ 08 rijna\0] { } {08}.#.D { } {rijna}.@.M {\0}"sv
                           ));
        TextDateTest(t, "08 listopad, 08 lis, 08 listopadu"
                        ", 08 prosinec, 08 pro, 08 prosince", LANG_CZE,
                        Ct("[ 08 listopad, ] { } {08}.#.D { } {listopad}.@.M {, }"sv,
                           "[ 08 lis, ] { } {08}.#.D { } {lis}.@.M {, }"sv,
                           "[ 08 listopadu, ] { } {08}.#.D { } {listopadu}.@.M {, }"sv,
                           "[ 08 prosinec, ] { } {08}.#.D { } {prosinec}.@.M {, }"sv,
                           "[ 08 pro, ] { } {08}.#.D { } {pro}.@.M {, }"sv,
                           "[ 08 prosince\0] { } {08}.#.D { } {prosince}.@.M {\0}"sv
                           ));
        TextDateTest(t, "08 Januar,    08 Jan"
                        ", 08 Februar,   08 Feb"
                        ", 08 März,      08 Mär, 08 Mar"
                        ", 08 April,     08 Apr"
                        ", 08 Mai"
                        ", 08 Juni,      08 Jun", LANG_GER,
                        Ct("[ 08 januar, ] { } {08}.#.D { } {januar}.@.M {, }"sv,
                           "[ 08 jan, ] { } {08}.#.D { } {jan}.@.M {, }"sv,
                           "[ 08 februar, ] { } {08}.#.D { } {februar}.@.M {, }"sv,
                           "[ 08 feb, ] { } {08}.#.D { } {feb}.@.M {, }"sv,
                           "[ 08 maerz, ] { } {08}.#.D { } {maerz}.@.M {, }"sv,
                           "[ 08 maer, ] { } {08}.#.D { } {maer}.@.M {, }"sv,
                           "[ 08 mar, ] { } {08}.#.D { } {mar}.@.M {, }"sv,
                           "[ 08 april, ] { } {08}.#.D { } {april}.@.M {, }"sv,
                           "[ 08 apr, ] { } {08}.#.D { } {apr}.@.M {, }"sv,
                           "[ 08 mai, ] { } {08}.#.D { } {mai}.@.M {, }"sv,
                           "[ 08 juni, ] { } {08}.#.D { } {juni}.@.M {, }"sv,
                           "[ 08 jun\0] { } {08}.#.D { } {jun}.@.M {\0}"sv
                           ));
        TextDateTest(t, "08 Juli,      08 Jul"
                        ", 08 August,    08 Aug"
                        ", 08 September, 08 Sep"
                        ", 08 Oktober,   08 Okt"
                        ", 08 November,  08 Nov"
                        ", 08 Dezember,  08 Dez", LANG_GER,
                        Ct("[ 08 juli, ] { } {08}.#.D { } {juli}.@.M {, }"sv,
                           "[ 08 jul, ] { } {08}.#.D { } {jul}.@.M {, }"sv,
                           "[ 08 august, ] { } {08}.#.D { } {august}.@.M {, }"sv,
                           "[ 08 aug, ] { } {08}.#.D { } {aug}.@.M {, }"sv,
                           "[ 08 september, ] { } {08}.#.D { } {september}.@.M {, }"sv,
                           "[ 08 sep, ] { } {08}.#.D { } {sep}.@.M {, }"sv,
                           "[ 08 oktober, ] { } {08}.#.D { } {oktober}.@.M {, }"sv,
                           "[ 08 okt, ] { } {08}.#.D { } {okt}.@.M {, }"sv,
                           "[ 08 november, ] { } {08}.#.D { } {november}.@.M {, }"sv,
                           "[ 08 nov, ] { } {08}.#.D { } {nov}.@.M {, }"sv,
                           "[ 08 dezember, ] { } {08}.#.D { } {dezember}.@.M {, }"sv,
                           "[ 08 dez\0] { } {08}.#.D { } {dez}.@.M {\0}"sv
                           ));

        TextDateTest(t, "08 january,   08 jan"
                        ", 08 february,  08 feb, 08 febr"
                        ", 08 march,     08 mar"
                        ", 08 april,     08 apr"
                        ", 08 may"
                        ", 08 june,      08 jun", LANG_ENG,
                        Ct("[ 08 january, ] { } {08}.#.D { } {january}.@.M {, }"sv,
                           "[ 08 jan, ] { } {08}.#.D { } {jan}.@.M {, }"sv,
                           "[ 08 february, ] { } {08}.#.D { } {february}.@.M {, }"sv,
                           "[ 08 feb, ] { } {08}.#.D { } {feb}.@.M {, }"sv,
                           "[ 08 febr, ] { } {08}.#.D { } {febr}.@.M {, }"sv,
                           "[ 08 march, ] { } {08}.#.D { } {march}.@.M {, }"sv,
                           "[ 08 mar, ] { } {08}.#.D { } {mar}.@.M {, }"sv,
                           "[ 08 april, ] { } {08}.#.D { } {april}.@.M {, }"sv,
                           "[ 08 apr, ] { } {08}.#.D { } {apr}.@.M {, }"sv,
                           "[ 08 may, ] { } {08}.#.D { } {may}.@.M {, }"sv,
                           "[ 08 june, ] { } {08}.#.D { } {june}.@.M {, }"sv,
                           "[ 08 jun\0] { } {08}.#.D { } {jun}.@.M {\0}"sv
                           ));
        TextDateTest(t, "08 july,      08 jul, 08 jly"
                        ", 08 august,    08 aug"
                        ", 08 september, 08 sep, 08 sept"
                        ", 08 october,   08 oct"
                        ", 08 november,  08 nov"
                        ", 08 december,  08 dec", LANG_ENG,
                        Ct("[ 08 july, ] { } {08}.#.D { } {july}.@.M {, }"sv,
                           "[ 08 jul, ] { } {08}.#.D { } {jul}.@.M {, }"sv,
                           "[ 08 jly, ] { } {08}.#.D { } {jly}.@.M {, }"sv,
                           "[ 08 august, ] { } {08}.#.D { } {august}.@.M {, }"sv,
                           "[ 08 aug, ] { } {08}.#.D { } {aug}.@.M {, }"sv,
                           "[ 08 september, ] { } {08}.#.D { } {september}.@.M {, }"sv,
                           "[ 08 sep, ] { } {08}.#.D { } {sep}.@.M {, }"sv,
                           "[ 08 sept, ] { } {08}.#.D { } {sept}.@.M {, }"sv,
                           "[ 08 october, ] { } {08}.#.D { } {october}.@.M {, }"sv,
                           "[ 08 oct, ] { } {08}.#.D { } {oct}.@.M {, }"sv,
                           "[ 08 november, ] { } {08}.#.D { } {november}.@.M {, }"sv,
                           "[ 08 nov, ] { } {08}.#.D { } {nov}.@.M {, }"sv,
                           "[ 08 december, ] { } {08}.#.D { } {december}.@.M {, }"sv,
                           "[ 08 dec\0] { } {08}.#.D { } {dec}.@.M {\0}"sv
                           ));

        TextDateTest(t, "08 enero,      08 ene"
                        ", 08 febrero,    08 feb"
                        ", 08 marzo,      08 mar"
                        ", 08 abril,      08 abr"
                        ", 08 mayo,       08 may"
                        ", 08 junio,      08 jun", LANG_SPA,
                        Ct("[ 08 enero, ] { } {08}.#.D { } {enero}.@.M {, }"sv,
                           "[ 08 ene, ] { } {08}.#.D { } {ene}.@.M {, }"sv,
                           "[ 08 febrero, ] { } {08}.#.D { } {febrero}.@.M {, }"sv,
                           "[ 08 feb, ] { } {08}.#.D { } {feb}.@.M {, }"sv,
                           "[ 08 marzo, ] { } {08}.#.D { } {marzo}.@.M {, }"sv,
                           "[ 08 mar, ] { } {08}.#.D { } {mar}.@.M {, }"sv,
                           "[ 08 abril, ] { } {08}.#.D { } {abril}.@.M {, }"sv,
                           "[ 08 abr, ] { } {08}.#.D { } {abr}.@.M {, }"sv,
                           "[ 08 mayo, ] { } {08}.#.D { } {mayo}.@.M {, }"sv,
                           "[ 08 may, ] { } {08}.#.D { } {may}.@.M {, }"sv,
                           "[ 08 junio, ] { } {08}.#.D { } {junio}.@.M {, }"sv,
                           "[ 08 jun\0] { } {08}.#.D { } {jun}.@.M {\0}"sv
                           ));
        TextDateTest(t, "08 julio,      08 jul"
                        ", 08 agosto,     08 ago"
                        ", 08 septiembre, 08 sep"
                        ", 08 octubre,    08 oct"
                        ", 08 noviembre,  08 nov"
                        ", 08 diciembre,  08 dic", LANG_SPA,
                        Ct("[ 08 julio, ] { } {08}.#.D { } {julio}.@.M {, }"sv,
                           "[ 08 jul, ] { } {08}.#.D { } {jul}.@.M {, }"sv,
                           "[ 08 agosto, ] { } {08}.#.D { } {agosto}.@.M {, }"sv,
                           "[ 08 ago, ] { } {08}.#.D { } {ago}.@.M {, }"sv,
                           "[ 08 septiembre, ] { } {08}.#.D { } {septiembre}.@.M {, }"sv,
                           "[ 08 sep, ] { } {08}.#.D { } {sep}.@.M {, }"sv,
                           "[ 08 octubre, ] { } {08}.#.D { } {octubre}.@.M {, }"sv,
                           "[ 08 oct, ] { } {08}.#.D { } {oct}.@.M {, }"sv,
                           "[ 08 noviembre, ] { } {08}.#.D { } {noviembre}.@.M {, }"sv,
                           "[ 08 nov, ] { } {08}.#.D { } {nov}.@.M {, }"sv,
                           "[ 08 diciembre, ] { } {08}.#.D { } {diciembre}.@.M {, }"sv,
                           "[ 08 dic\0] { } {08}.#.D { } {dic}.@.M {\0}"sv
                           ));

        // todo: 3-letter abbrs
        TextDateTest(t, "08 janvier,   08 jan, 08 janv"
                        ", 08 février,   08 fév, 08 févr, 08 fev, 08 fevr"
                        ", 08 mars,      08 mar"
                        ", 08 avril,     08 avr"
                        ", 08 mai"
                        ", 08 juin", LANG_FRE,
                         Ct("[ 08 janvier, ] { } {08}.#.D { } {janvier}.@.M {, }"sv,
                            "[ 08 jan, ] { } {08}.#.D { } {jan}.@.M {, }"sv,
                            "[ 08 janv, ] { } {08}.#.D { } {janv}.@.M {, }"sv,
                            "[ 08 fevrier, ] { } {08}.#.D { } {fevrier}.@.M {, }"sv,
                            "[ 08 fev, ] { } {08}.#.D { } {fev}.@.M {, }"sv,
                            "[ 08 fevr, ] { } {08}.#.D { } {fevr}.@.M {, }"sv,
                            "[ 08 fev, ] { } {08}.#.D { } {fev}.@.M {, }"sv,
                            "[ 08 fevr, ] { } {08}.#.D { } {fevr}.@.M {, }"sv,
                            "[ 08 mars, ] { } {08}.#.D { } {mars}.@.M {, }"sv,
                            "[ 08 mar, ] { } {08}.#.D { } {mar}.@.M {, }"sv,
                            "[ 08 avril, ] { } {08}.#.D { } {avril}.@.M {, }"sv,
                            "[ 08 avr, ] { } {08}.#.D { } {avr}.@.M {, }"sv,
                            "[ 08 mai, ] { } {08}.#.D { } {mai}.@.M {, }"sv,
                            "[ 08 juin\0] { } {08}.#.D { } {juin}.@.M {\0}"sv
                            ));
        TextDateTest(t, "08 juillet,   08 juil"
                        ", 08 août,      08 aoû, 08 aou"
                        ", 08 septembre, 08 sep, 08 sept"
                        ", 08 octobre,   08 oct"
                        ", 08 novembre,  08 nov"
                        ", 08 décembre,  08 déc, 08 dec", LANG_FRE,
                        Ct("[ 08 juillet, ] { } {08}.#.D { } {juillet}.@.M {, }"sv,
                           "[ 08 juil, ] { } {08}.#.D { } {juil}.@.M {, }"sv,
                           "[ 08 aout, ] { } {08}.#.D { } {aout}.@.M {, }"sv,
                           "[ 08 aou, ] { } {08}.#.D { } {aou}.@.M {, }"sv,
                           "[ 08 aou, ] { } {08}.#.D { } {aou}.@.M {, }"sv,
                           "[ 08 septembre, ] { } {08}.#.D { } {septembre}.@.M {, }"sv,
                           "[ 08 sep, ] { } {08}.#.D { } {sep}.@.M {, }"sv,
                           "[ 08 sept, ] { } {08}.#.D { } {sept}.@.M {, }"sv,
                           "[ 08 octobre, ] { } {08}.#.D { } {octobre}.@.M {, }"sv,
                           "[ 08 oct, ] { } {08}.#.D { } {oct}.@.M {, }"sv,
                           "[ 08 novembre, ] { } {08}.#.D { } {novembre}.@.M {, }"sv,
                           "[ 08 nov, ] { } {08}.#.D { } {nov}.@.M {, }"sv,
                           "[ 08 decembre, ] { } {08}.#.D { } {decembre}.@.M {, }"sv,
                           "[ 08 dec, ] { } {08}.#.D { } {dec}.@.M {, }"sv,
                           "[ 08 dec\0] { } {08}.#.D { } {dec}.@.M {\0}"sv
                           ));

        TextDateTest(t, "08 gennaio,   08 gen"
                        ", 08 febbraio,  08 feb"
                        ", 08 marzo,     08 mar"
                        ", 08 aprile,    08 apr"
                        ", 08 maggio,    08 mag"
                        ", 08 giugno,    08 giu", LANG_ITA,
                        Ct("[ 08 gennaio, ] { } {08}.#.D { } {gennaio}.@.M {, }"sv,
                           "[ 08 gen, ] { } {08}.#.D { } {gen}.@.M {, }"sv,
                           "[ 08 febbraio, ] { } {08}.#.D { } {febbraio}.@.M {, }"sv,
                           "[ 08 feb, ] { } {08}.#.D { } {feb}.@.M {, }"sv,
                           "[ 08 marzo, ] { } {08}.#.D { } {marzo}.@.M {, }"sv,
                           "[ 08 mar, ] { } {08}.#.D { } {mar}.@.M {, }"sv,
                           "[ 08 aprile, ] { } {08}.#.D { } {aprile}.@.M {, }"sv,
                           "[ 08 apr, ] { } {08}.#.D { } {apr}.@.M {, }"sv,
                           "[ 08 maggio, ] { } {08}.#.D { } {maggio}.@.M {, }"sv,
                           "[ 08 mag, ] { } {08}.#.D { } {mag}.@.M {, }"sv,
                           "[ 08 giugno, ] { } {08}.#.D { } {giugno}.@.M {, }"sv,
                           "[ 08 giu\0] { } {08}.#.D { } {giu}.@.M {\0}"sv
                           ));
        TextDateTest(t, "08 luglio,    08 lug"
                        ", 08 agosto,    08 ago"
                        ", 08 settembre, 08 set"
                        ", 08 ottobre,   08 ott"
                        ", 08 novembre,  08 nov"
                        ", 08 dicembre,  08 dic", LANG_ITA,
                        Ct("[ 08 luglio, ] { } {08}.#.D { } {luglio}.@.M {, }"sv,
                           "[ 08 lug, ] { } {08}.#.D { } {lug}.@.M {, }"sv,
                           "[ 08 agosto, ] { } {08}.#.D { } {agosto}.@.M {, }"sv,
                           "[ 08 ago, ] { } {08}.#.D { } {ago}.@.M {, }"sv,
                           "[ 08 settembre, ] { } {08}.#.D { } {settembre}.@.M {, }"sv,
                           "[ 08 set, ] { } {08}.#.D { } {set}.@.M {, }"sv,
                           "[ 08 ottobre, ] { } {08}.#.D { } {ottobre}.@.M {, }"sv,
                           "[ 08 ott, ] { } {08}.#.D { } {ott}.@.M {, }"sv,
                           "[ 08 novembre, ] { } {08}.#.D { } {novembre}.@.M {, }"sv,
                           "[ 08 nov, ] { } {08}.#.D { } {nov}.@.M {, }"sv,
                           "[ 08 dicembre, ] { } {08}.#.D { } {dicembre}.@.M {, }"sv,
                           "[ 08 dic\0] { } {08}.#.D { } {dic}.@.M {\0}"sv
                           ));

        TextDateTest(t, "08 Қаңтар,    08 Қаңт, 08 Қңт"
                        ", 08 Ақпан,     08 Ақп"
                        ", 08 Наурыз,    08 Нау"
                        ", 08 Сәуір,     08 Сәу"
                        ", 08 мамыр,     08 мам"
                        ", 08 маусым,    08 мау", LANG_KAZ,
                        Ct("[ 08 кантар, ] { } {08}.#.D { } {кантар}.@.M {, }"sv,
                           "[ 08 кант, ] { } {08}.#.D { } {кант}.@.M {, }"sv,
                           "[ 08 кнт, ] { } {08}.#.D { } {кнт}.@.M {, }"sv,
                           "[ 08 акпан, ] { } {08}.#.D { } {акпан}.@.M {, }"sv,
                           "[ 08 акп, ] { } {08}.#.D { } {акп}.@.M {, }"sv,
                           "[ 08 наурыз, ] { } {08}.#.D { } {наурыз}.@.M {, }"sv,
                           "[ 08 нау, ] { } {08}.#.D { } {нау}.@.M {, }"sv,
                           "[ 08 cауiр, ] { } {08}.#.D { } {cауiр}.@.M {, }"sv,
                           "[ 08 cау, ] { } {08}.#.D { } {cау}.@.M {, }"sv,
                           "[ 08 мамыр, ] { } {08}.#.D { } {мамыр}.@.M {, }"sv,
                           "[ 08 мам, ] { } {08}.#.D { } {мам}.@.M {, }"sv,
                           "[ 08 мауcым, ] { } {08}.#.D { } {мауcым}.@.M {, }"sv,
                           "[ 08 мау\0] { } {08}.#.D { } {мау}.@.M {\0}"sv
                           ));
        TextDateTest(t, "08 шiлде,     08 шiлд, 08 шлд"
                        ", 08 тамыз,     08 там"
                        ", 08 Қыркүйек,  08 Қырк, 08 Қрк"
                        ", 08 Қазан,     08 Қаз"
                        ", 08 Қараша,    08 Қар"
                        ", 08 Желтоқсан, 08 жел", LANG_KAZ,
                        Ct("[ 08 шiлде, ] { } {08}.#.D { } {шiлде}.@.M {, }"sv,
                           "[ 08 шiлд, ] { } {08}.#.D { } {шiлд}.@.M {, }"sv,
                           "[ 08 шлд, ] { } {08}.#.D { } {шлд}.@.M {, }"sv,
                           "[ 08 тамыз, ] { } {08}.#.D { } {тамыз}.@.M {, }"sv,
                           "[ 08 там, ] { } {08}.#.D { } {там}.@.M {, }"sv,
                           "[ 08 кыркуйек, ] { } {08}.#.D { } {кыркуйек}.@.M {, }"sv,
                           "[ 08 кырк, ] { } {08}.#.D { } {кырк}.@.M {, }"sv,
                           "[ 08 крк, ] { } {08}.#.D { } {крк}.@.M {, }"sv,
                           "[ 08 казан, ] { } {08}.#.D { } {казан}.@.M {, }"sv,
                           "[ 08 каз, ] { } {08}.#.D { } {каз}.@.M {, }"sv,
                           "[ 08 караша, ] { } {08}.#.D { } {караша}.@.M {, }"sv,
                           "[ 08 кар, ] { } {08}.#.D { } {кар}.@.M {, }"sv,
                           "[ 08 желтокcан, ] { } {08}.#.D { } {желтокcан}.@.M {, }"sv,
                           "[ 08 жел\0] { } {08}.#.D { } {жел}.@.M {\0}"sv
                           ));

        TextDateTest(t, "08 Styczeń,     08 Sty, 08 stycznia"
                        ", 08 luty,        08 lut, 08 lutego"
                        ", 08 marzec,      08 mar, 08 marca"
                        ", 08 kwiecień,    08 Kwi, 08 kwietnia"
                        ", 08 maj,         08 maja", LANG_POL,
                        Ct("[ 08 styczen, ] { } {08}.#.D { } {styczen}.@.M {, }"sv,
                           "[ 08 sty, ] { } {08}.#.D { } {sty}.@.M {, }"sv,
                           "[ 08 stycznia, ] { } {08}.#.D { } {stycznia}.@.M {, }"sv,
                           "[ 08 luty, ] { } {08}.#.D { } {luty}.@.M {, }"sv,
                           "[ 08 lut, ] { } {08}.#.D { } {lut}.@.M {, }"sv,
                           "[ 08 lutego, ] { } {08}.#.D { } {lutego}.@.M {, }"sv,
                           "[ 08 marzec, ] { } {08}.#.D { } {marzec}.@.M {, }"sv,
                           "[ 08 mar, ] { } {08}.#.D { } {mar}.@.M {, }"sv,
                           "[ 08 marca, ] { } {08}.#.D { } {marca}.@.M {, }"sv,
                           "[ 08 kwiecien, ] { } {08}.#.D { } {kwiecien}.@.M {, }"sv,
                           "[ 08 kwi, ] { } {08}.#.D { } {kwi}.@.M {, }"sv,
                           "[ 08 kwietnia, ] { } {08}.#.D { } {kwietnia}.@.M {, }"sv,
                           "[ 08 maj, ] { } {08}.#.D { } {maj}.@.M {, }"sv,
                           "[ 08 maja\0] { } {08}.#.D { } {maja}.@.M {\0}"sv
                           ));
        TextDateTest(t, "08 czerwiec,    08 cze, 08 czerwca"
                        ", 08 lipiec,      08 lip, 08 lipca"
                        ", 08 sierpień,    08 sie, 08 sierpnia"
                        ", 08 wrzesień,    08 Wrz, 08 września"
                        ", 08 październik, 08 paź, 08 października", LANG_POL,
                        Ct("[ 08 czerwiec, ] { } {08}.#.D { } {czerwiec}.@.M {, }"sv,
                           "[ 08 cze, ] { } {08}.#.D { } {cze}.@.M {, }"sv,
                           "[ 08 czerwca, ] { } {08}.#.D { } {czerwca}.@.M {, }"sv,
                           "[ 08 lipiec, ] { } {08}.#.D { } {lipiec}.@.M {, }"sv,
                           "[ 08 lip, ] { } {08}.#.D { } {lip}.@.M {, }"sv,
                           "[ 08 lipca, ] { } {08}.#.D { } {lipca}.@.M {, }"sv,
                           "[ 08 sierpien, ] { } {08}.#.D { } {sierpien}.@.M {, }"sv,
                           "[ 08 sie, ] { } {08}.#.D { } {sie}.@.M {, }"sv,
                           "[ 08 sierpnia, ] { } {08}.#.D { } {sierpnia}.@.M {, }"sv,
                           "[ 08 wrzesien, ] { } {08}.#.D { } {wrzesien}.@.M {, }"sv,
                           "[ 08 wrz, ] { } {08}.#.D { } {wrz}.@.M {, }"sv,
                           "[ 08 wrzesnia, ] { } {08}.#.D { } {wrzesnia}.@.M {, }"sv,
                           "[ 08 pazdziernik, ] { } {08}.#.D { } {pazdziernik}.@.M {, }"sv,
                           "[ 08 paz, ] { } {08}.#.D { } {paz}.@.M {, }"sv,
                           "[ 08 pazdziernika\0] { } {08}.#.D { } {pazdziernika}.@.M {\0}"sv
                           ));
        TextDateTest(t, "08 listopad,    08 lis, 08 listopada"
                        ", 08 grudzień,    08 gru, 08 grudnia", LANG_POL,
                        Ct("[ 08 listopad, ] { } {08}.#.D { } {listopad}.@.M {, }"sv,
                           "[ 08 lis, ] { } {08}.#.D { } {lis}.@.M {, }"sv,
                           "[ 08 listopada, ] { } {08}.#.D { } {listopada}.@.M {, }"sv,
                           "[ 08 grudzien, ] { } {08}.#.D { } {grudzien}.@.M {, }"sv,
                           "[ 08 gru, ] { } {08}.#.D { } {gru}.@.M {, }"sv,
                           "[ 08 grudnia\0] { } {08}.#.D { } {grudnia}.@.M {\0}"sv
                           ));

        TextDateTest(t, "08 январь,   08 января,   08 янв"
                        ", 08 февраль,  08 февраля,  08 фев, 08 февр"
                        ", 08 март,     08 марта,    08 мар"
                        ", 08 апрель,   08 апреля,   08 апр"
                        ", 08 май,      08 мая", LANG_RUS,
                        Ct("[ 08 январь, ] { } {08}.#.D { } {январь}.@.M {, }"sv,
                           "[ 08 января, ] { } {08}.#.D { } {января}.@.M {, }"sv,
                           "[ 08 янв, ] { } {08}.#.D { } {янв}.@.M {, }"sv,
                           "[ 08 февраль, ] { } {08}.#.D { } {февраль}.@.M {, }"sv,
                           "[ 08 февраля, ] { } {08}.#.D { } {февраля}.@.M {, }"sv,
                           "[ 08 фев, ] { } {08}.#.D { } {фев}.@.M {, }"sv,
                           "[ 08 февр, ] { } {08}.#.D { } {февр}.@.M {, }"sv,
                           "[ 08 март, ] { } {08}.#.D { } {март}.@.M {, }"sv,
                           "[ 08 марта, ] { } {08}.#.D { } {марта}.@.M {, }"sv,
                           "[ 08 мар, ] { } {08}.#.D { } {мар}.@.M {, }"sv,
                           "[ 08 апрель, ] { } {08}.#.D { } {апрель}.@.M {, }"sv,
                           "[ 08 апреля, ] { } {08}.#.D { } {апреля}.@.M {, }"sv,
                           "[ 08 апр, ] { } {08}.#.D { } {апр}.@.M {, }"sv,
                           "[ 08 май, ] { } {08}.#.D { } {май}.@.M {, }"sv,
                           "[ 08 мая\0] { } {08}.#.D { } {мая}.@.M {\0}"sv
                           ));
        TextDateTest(t, "08 июнь,     08 июня,     08 июн"
                        ", 08 июль,     08 июля,     08 июл"
                        ", 08 август,   08 августа,  08 авг"
                        ", 08 сентябрь, 08 сентября, 08 сен, 08 сент"
                        ", 08 октябрь,  08 октября,  08 окт", LANG_RUS,
                        Ct("[ 08 июнь, ] { } {08}.#.D { } {июнь}.@.M {, }"sv,
                           "[ 08 июня, ] { } {08}.#.D { } {июня}.@.M {, }"sv,
                           "[ 08 июн, ] { } {08}.#.D { } {июн}.@.M {, }"sv,
                           "[ 08 июль, ] { } {08}.#.D { } {июль}.@.M {, }"sv,
                           "[ 08 июля, ] { } {08}.#.D { } {июля}.@.M {, }"sv,
                           "[ 08 июл, ] { } {08}.#.D { } {июл}.@.M {, }"sv,
                           "[ 08 авгуcт, ] { } {08}.#.D { } {авгуcт}.@.M {, }"sv,
                           "[ 08 авгуcта, ] { } {08}.#.D { } {авгуcта}.@.M {, }"sv,
                           "[ 08 авг, ] { } {08}.#.D { } {авг}.@.M {, }"sv,
                           "[ 08 cентябрь, ] { } {08}.#.D { } {cентябрь}.@.M {, }"sv,
                           "[ 08 cентября, ] { } {08}.#.D { } {cентября}.@.M {, }"sv,
                           "[ 08 cен, ] { } {08}.#.D { } {cен}.@.M {, }"sv,
                           "[ 08 cент, ] { } {08}.#.D { } {cент}.@.M {, }"sv,
                           "[ 08 октябрь, ] { } {08}.#.D { } {октябрь}.@.M {, }"sv,
                           "[ 08 октября, ] { } {08}.#.D { } {октября}.@.M {, }"sv,
                           "[ 08 окт\0] { } {08}.#.D { } {окт}.@.M {\0}"sv
                           ));
        TextDateTest(t, "08 ноябрь,   08 ноября,   08 ноя, 08 нояб"
                        ", 08 декабрь,  08 декабря,  08 дек", LANG_RUS,
                        Ct("[ 08 ноябрь, ] { } {08}.#.D { } {ноябрь}.@.M {, }"sv,
                           "[ 08 ноября, ] { } {08}.#.D { } {ноября}.@.M {, }"sv,
                           "[ 08 ноя, ] { } {08}.#.D { } {ноя}.@.M {, }"sv,
                           "[ 08 нояб, ] { } {08}.#.D { } {нояб}.@.M {, }"sv,
                           "[ 08 декабрь, ] { } {08}.#.D { } {декабрь}.@.M {, }"sv,
                           "[ 08 декабря, ] { } {08}.#.D { } {декабря}.@.M {, }"sv,
                           "[ 08 дек\0] { } {08}.#.D { } {дек}.@.M {\0}"sv
                           ));

        TextDateTest(t, "08 Ocak,    08 Oca"
                        ", 08 Şubat,   08 Şub"
                        ", 08 Mart,    08 Mar"
                        ", 08 Nisan,   08 Nis"
                        ", 08 Mayıs,   08 May"
                        ", 08 Haziran, 08 Haz", LANG_TUR,
                        Ct("[ 08 ocak, ] { } {08}.#.D { } {ocak}.@.M {, }"sv,
                           "[ 08 oca, ] { } {08}.#.D { } {oca}.@.M {, }"sv,
                           "[ 08 subat, ] { } {08}.#.D { } {subat}.@.M {, }"sv,
                           "[ 08 sub, ] { } {08}.#.D { } {sub}.@.M {, }"sv,
                           "[ 08 mart, ] { } {08}.#.D { } {mart}.@.M {, }"sv,
                           "[ 08 mar, ] { } {08}.#.D { } {mar}.@.M {, }"sv,
                           "[ 08 nisan, ] { } {08}.#.D { } {nisan}.@.M {, }"sv,
                           "[ 08 nis, ] { } {08}.#.D { } {nis}.@.M {, }"sv,
                           "[ 08 mayis, ] { } {08}.#.D { } {mayis}.@.M {, }"sv,
                           "[ 08 may, ] { } {08}.#.D { } {may}.@.M {, }"sv,
                           "[ 08 haziran, ] { } {08}.#.D { } {haziran}.@.M {, }"sv,
                           "[ 08 haz\0] { } {08}.#.D { } {haz}.@.M {\0}"sv
                           ));

        TextDateTest(t, "08 Temmuz,  08 Tem"
                        ", 08 Ağustos, 08 Ağu"
                        ", 08 Eylül,   08 Eyl"
                        ", 08 Ekim,    08 Ekm"
                        ", 08 Kasım,   08 Kas"
                        ", 08 Aralık,  08 Arl", LANG_TUR,
                        Ct("[ 08 temmuz, ] { } {08}.#.D { } {temmuz}.@.M {, }"sv,
                           "[ 08 tem, ] { } {08}.#.D { } {tem}.@.M {, }"sv,
                           "[ 08 agustos, ] { } {08}.#.D { } {agustos}.@.M {, }"sv,
                           "[ 08 agu, ] { } {08}.#.D { } {agu}.@.M {, }"sv,
                           "[ 08 eylul, ] { } {08}.#.D { } {eylul}.@.M {, }"sv,
                           "[ 08 eyl, ] { } {08}.#.D { } {eyl}.@.M {, }"sv,
                           "[ 08 ekim, ] { } {08}.#.D { } {ekim}.@.M {, }"sv,
                           "[ 08 ekm, ] { } {08}.#.D { } {ekm}.@.M {, }"sv,
                           "[ 08 kasim, ] { } {08}.#.D { } {kasim}.@.M {, }"sv,
                           "[ 08 kas, ] { } {08}.#.D { } {kas}.@.M {, }"sv,
                           "[ 08 aralik, ] { } {08}.#.D { } {aralik}.@.M {, }"sv,
                           "[ 08 arl\0] { } {08}.#.D { } {arl}.@.M {\0}"sv
                           ));

        TextDateTest(t, "08 січень,   08 Ciч,  08 січеня"
                        ", 08 лютий,    08 Лют,  08 лютого"
                        ", 08 березень, 08 Бер,  08 березня"
                        ", 08 квітень,  08 Квiт, 08 Квi, 08 квітня", LANG_UKR,
                        Ct("[ 08 ciчень, ] { } {08}.#.D { } {ciчень}.@.M {, }"sv,
                           "[ 08 ciч, ] { } {08}.#.D { } {ciч}.@.M {, }"sv,
                           "[ 08 ciченя, ] { } {08}.#.D { } {ciченя}.@.M {, }"sv,
                           "[ 08 лютий, ] { } {08}.#.D { } {лютий}.@.M {, }"sv,
                           "[ 08 лют, ] { } {08}.#.D { } {лют}.@.M {, }"sv,
                           "[ 08 лютого, ] { } {08}.#.D { } {лютого}.@.M {, }"sv,
                           "[ 08 березень, ] { } {08}.#.D { } {березень}.@.M {, }"sv,
                           "[ 08 бер, ] { } {08}.#.D { } {бер}.@.M {, }"sv,
                           "[ 08 березня, ] { } {08}.#.D { } {березня}.@.M {, }"sv,
                           "[ 08 квiтень, ] { } {08}.#.D { } {квiтень}.@.M {, }"sv,
                           "[ 08 квiт, ] { } {08}.#.D { } {квiт}.@.M {, }"sv,
                           "[ 08 квi, ] { } {08}.#.D { } {квi}.@.M {, }"sv,
                           "[ 08 квiтня\0] { } {08}.#.D { } {квiтня}.@.M {\0}"sv
                           ));
        TextDateTest(t, "08 травень,  08 Трав, 08 тра, 08 травня"
                        ", 08 червень,  08 Чер,  08 червня"
                        ", 08 липень,   08 Лип,  08 липня"
                        ", 08 серпень,  08 Серп, 08 сер, 08 серпня", LANG_UKR,
                        Ct("[ 08 травень, ] { } {08}.#.D { } {травень}.@.M {, }"sv,
                           "[ 08 трав, ] { } {08}.#.D { } {трав}.@.M {, }"sv,
                           "[ 08 тра, ] { } {08}.#.D { } {тра}.@.M {, }"sv,
                           "[ 08 травня, ] { } {08}.#.D { } {травня}.@.M {, }"sv,
                           "[ 08 червень, ] { } {08}.#.D { } {червень}.@.M {, }"sv,
                           "[ 08 чер, ] { } {08}.#.D { } {чер}.@.M {, }"sv,
                           "[ 08 червня, ] { } {08}.#.D { } {червня}.@.M {, }"sv,
                           "[ 08 липень, ] { } {08}.#.D { } {липень}.@.M {, }"sv,
                           "[ 08 лип, ] { } {08}.#.D { } {лип}.@.M {, }"sv,
                           "[ 08 липня, ] { } {08}.#.D { } {липня}.@.M {, }"sv,
                           "[ 08 cерпень, ] { } {08}.#.D { } {cерпень}.@.M {, }"sv,
                           "[ 08 cерп, ] { } {08}.#.D { } {cерп}.@.M {, }"sv,
                           "[ 08 cер, ] { } {08}.#.D { } {cер}.@.M {, }"sv,
                           "[ 08 cерпня\0] { } {08}.#.D { } {cерпня}.@.M {\0}"sv
                           ));
        TextDateTest(t, "08 вересень, 08 Вер,  08 вересня"
                        ", 08 жовтень,  08 Жовт, 08 жов, 08 жовтня"
                        ", 08 листопад, 08 Лист, 08 лис, 08 листопада"
                        ", 08 грудень,  08 Груд, 08 гру, 08 грудня", LANG_UKR,
                        Ct("[ 08 вереcень, ] { } {08}.#.D { } {вереcень}.@.M {, }"sv,
                           "[ 08 вер, ] { } {08}.#.D { } {вер}.@.M {, }"sv,
                           "[ 08 вереcня, ] { } {08}.#.D { } {вереcня}.@.M {, }"sv,
                           "[ 08 жовтень, ] { } {08}.#.D { } {жовтень}.@.M {, }"sv,
                           "[ 08 жовт, ] { } {08}.#.D { } {жовт}.@.M {, }"sv,
                           "[ 08 жов, ] { } {08}.#.D { } {жов}.@.M {, }"sv,
                           "[ 08 жовтня, ] { } {08}.#.D { } {жовтня}.@.M {, }"sv,
                           "[ 08 лиcтопад, ] { } {08}.#.D { } {лиcтопад}.@.M {, }"sv,
                           "[ 08 лиcт, ] { } {08}.#.D { } {лиcт}.@.M {, }"sv,
                           "[ 08 лиc, ] { } {08}.#.D { } {лиc}.@.M {, }"sv,
                           "[ 08 лиcтопада, ] { } {08}.#.D { } {лиcтопада}.@.M {, }"sv,
                           "[ 08 грудень, ] { } {08}.#.D { } {грудень}.@.M {, }"sv,
                           "[ 08 груд, ] { } {08}.#.D { } {груд}.@.M {, }"sv,
                           "[ 08 гру, ] { } {08}.#.D { } {гру}.@.M {, }"sv,
                           "[ 08 грудня\0] { } {08}.#.D { } {грудня}.@.M {\0}"sv
                           ));
    }

    Y_UNIT_TEST(TestGenericTextScan) {
        using namespace ND2;
        TTestContext t;

        TextDateTest(t, "№ 25 // 29.06.2009", LANG_UNK,
                     Ct("[ 29.06.2009\0] { } {29}.#.D {.} {06}.#.M {.} {2009}.#.Y {\0}"sv));
        TextDateTest(t, "12 : 22 ", LANG_UNK,
                     Ct("[ 12 : 22 ] { 12 : 22 }.T"sv));
        TextDateTest(t, "12 : 22 June 13, 2008", LANG_UNK,
                     Ct("[ 12 : 22 ] { 12 : 22 }.T"sv,
                        "[ june 13, 2008\0] { } {june}.@.M { } {13}.#.D {, } {2008}.#.Y {\0}"sv));
        TextDateTest(t, "( 0922 ) 31-01-98", LANG_UNK,
                     Ct("[ ( 0922 ) 31-01-98\0] { ( 0922 ) 31-01-98\0}.J"sv));
        TextDateTest(t, "10.11.12", LANG_UNK,
                     Ct("[ 10.11.12\0] { } {10}.#.D {.} {11}.#.M {.} {12}.#.Y {\0}"sv));
        TextDateTest(t, "10/11/12", LANG_UNK,
                     Ct("[ 10/11/12\0] { } {10}.#.D {/} {11}.#.M {/} {12}.#.Y {\0}"sv));
        TextDateTest(t, "2012.11.10", LANG_UNK,
                     Ct("[ 2012.11.10\0] { } {2012}.#.Y {.} {11}.#.M {.} {10}.#.D {\0}"sv));
        TextDateTest(t, "2012/11/10", LANG_UNK,
                     Ct("[ 2012/11/10\0] { } {2012}.#.Y {/} {11}.#.M {/} {10}.#.D {\0}"sv));
        TextDateTest(t, "2012/31/10", LANG_UNK,
                     Ct("[ 2012/31/10\0] { 2012/31/10\0}.J"sv));
        TextDateTest(t, "10 . 11 . 12", LANG_UNK,
                     Ct("[ 10 . 11 . 12\0] { } {10}.#.D { . } {11}.#.M { . } {12}.#.Y {\0}"sv));
        TextDateTest(t, "1 . 1 . 12", LANG_UNK,
                     Ct("[ 1 . 1 . 12\0] { } {1}.#.D { . } {1}.#.M { . } {12}.#.Y {\0}"sv));
        TextDateTest(t, "1 . 1 . 1", LANG_UNK,
                     Ct("[ 1 . 1 . 1\0] { 1 . 1 . 1\0}.J"sv));
        TextDateTest(t, "10 / 11 / 12", LANG_UNK,
                     Ct("[ 10 / 11 / 12\0] { } {10}.#.D { / } {11}.#.M { / } {12}.#.Y {\0}"sv));
        TextDateTest(t, "10 /11 /12 ", LANG_UNK,
                     Ct("[ 10 /11 /12 ] { } {10}.#.D { /} {11}.#.M { /} {12}.#.Y { }"sv));
        TextDateTest(t, "1 / 2 / 2003 ", LANG_UNK,
                     Ct("[ 1 / 2 / 2003 ] { } {1}.#.D { / } {2}.#.M { / } {2003}.#.Y { }"sv));
        TextDateTest(t, "2003 / 2 / 1 ", LANG_UNK,
                     Ct("[ 2003 / 2 / 1 ] { } {2003}.#.Y { / } {2}.#.M { / } {1}.#.D { }"sv));
        TextDateTest(t, "10-11-12", LANG_UNK,
                     Ct("[ 10-11-12\0] { } {10}.#.D {-} {11}.#.M {-} {12}.#.Y {\0}"sv));
        TextDateTest(t, "10-11-2012", LANG_UNK,
                     Ct("[ 10-11-2012\0] { } {10}.#.D {-} {11}.#.M {-} {2012}.#.Y {\0}"sv));
        TextDateTest(t, "10 . 11 .  12", LANG_UNK,
                     Ct("[ 10 . 11 . 12\0] { } {10}.#.D { . } {11}.#.M { . } {12}.#.Y {\0}"sv));
        TextDateTest(t, "10. 11. 2012", LANG_UNK,
                     Ct("[ 10. 11. 2012\0] { } {10}.#.D {. } {11}.#.M {. } {2012}.#.Y {\0}"sv));
        TextDateTest(t, "10.11.12 1999", LANG_UNK,
                     Ct("[ 10.11.12 ] { } {10}.#.D {.} {11}.#.M {.} {12}.#.Y { }"sv,
                        "[ 1999\0] { } {1999}.#.Y {\0}"sv));
        TextDateTest(t, "13.14.15 1999", LANG_UNK,
                     Ct("[ 13.14.15 ] { 13.14.15 }.J"sv,
                        "[ 1999\0] { } {1999}.#.Y {\0}"sv));
        TextDateTest(t, "10.11.12 10.11.12", LANG_UNK,
                     Ct("[ 10.11.12 ] { } {10}.#.D {.} {11}.#.M {.} {12}.#.Y { }"sv,
                        "[ 10.11.12\0] { } {10}.#.D {.} {11}.#.M {.} {12}.#.Y {\0}"sv));

        TextDateTest(t, "1'st of November, 2012", LANG_UNK,
                     Ct("[ 1\'st of november, 2012\0] { } {1}.#.D {\'st} { } {of } {november}.@.M {, } {2012}.#.Y {\0}"sv));
        TextDateTest(t, "1st Nov. 2012", LANG_UNK,
                     Ct("[ 1st nov. 2012\0] { } {1}.#.D {st} { } {nov}.@.M {.} { } {2012}.#.Y {\0}"sv));
        TextDateTest(t, "2nd nov 2012", LANG_UNK,
                     Ct("[ 2nd nov 2012\0] { } {2}.#.D {nd} { } {nov}.@.M { } {2012}.#.Y {\0}"sv));
        TextDateTest(t, "10 NOV 2012", LANG_UNK,
                     Ct("[ 10 nov 2012\0] { } {10}.#.D { } {nov}.@.M { } {2012}.#.Y {\0}"sv));

        TextDateTest(t, "07.11.04 14:46", LANG_UNK,
                     Ct("[ 07.11.04 ] { } {07}.#.D {.} {11}.#.M {.} {04}.#.Y { }"sv,
                        "[ 14:46\0] { 14:46\0}.T"sv));

        TextDateTest(t, "2001-2004©", LANG_UNK,
                     Ct("[ 2001-] { } {2001}.#.Y {-}"sv,
                        "[-2004©] {-} {2004}.#.Y {©}"sv));
        TextDateTest(t, "© 2007-2009", LANG_UNK,
                     Ct("[ 2007-] { } {2007}.#.Y {-}"sv,
                        "[-2009\0] {-} {2009}.#.Y {\0}"sv));

        TextDateTest(t, "June 13, 2008, 12:22:24 PM", LANG_UNK,
                     Ct("[ june 13, 2008,] { } {june}.@.M { } {13}.#.D {, } {2008}.#.Y {,}"sv,
                        "[ 12:22:24 pm\0] { 12:22:24 pm\0}.T"sv));

        TextDateTest(t, "September-2008", LANG_UNK,
                     Ct("[ september-2008\0] { } {september}.@.M {-} {2008}.#.Y {\0}"sv));
        TextDateTest(t, "2008-September", LANG_UNK,
                     Ct("[ 2008-september\0] { } {2008}.#.Y {-} {september}.@.M {\0}"sv));

        TextDateTest(t, "05nov2005", LANG_UNK,
                     Ct("[ 05nov2005\0] { } {05}.#.D {nov}.@.M {2005}.#.Y {\0}"sv));

        TextDateTest(t, "http://www.free-smile.info/smiles/5/2/5_2_4.gif", LANG_UNK,
                     Ct("[/smiles/5/2/5_] {/smiles/5/2/5_}.J"sv));

        TextDateTest(t, "5/2/06", LANG_UNK,
                     Ct("[ 5/2/06\0] { } {5}.#.D {/} {2}.#.M {/} {06}.#.Y {\0}"sv));

        TextDateTest(t, "2005/2/6", LANG_UNK,
                     Ct("[ 2005/2/6\0] { } {2005}.#.Y {/} {2}.#.M {/} {6}.#.D {\0}"sv));

        TextDateTest(t, "10.11.12.13", LANG_UNK,
                     Ct("[ 10.11.12.13\0] { 10.11.12.13\0}.J"sv));

//        TextDateTest(t, "This Week in Space 11 – March 12, 2010", LANG_UNK,
//                     Ct("[ 11 - march 12] { } {11}.#.D { - } {march}.@.M { } {12}.#.Y"sv,
//                        "[ 2010\0] { } {2010}.#.Y {\0}"sv));

        TextDateTest(t, "01.05.2009-01.07.2009", LANG_UNK,
                     Ct("[ 01.05.2009-] { } {01}.#.D {.} {05}.#.M {.} {2009}.#.Y {-}"sv,
                        "[-01.07.2009\0] {-} {01}.#.D {.} {07}.#.M {.} {2009}.#.Y {\0}"sv));

        TextDateTest(t, " <30/06/2009> ", LANG_UNK,
                     Ct("[<30/06/2009>] {<} {30}.#.D {/} {06}.#.M {/} {2009}.#.Y {>}"sv));

        TextDateTest(t, "by September 2007.", LANG_UNK,
                     Ct("[ september 2007.] { } {september}.@.M { } {2007}.#.Y {.}"sv));

        TextDateTest(t, "2-mar", LANG_UNK,
                     Ct("[ 2-mar\0] { } {2}.#.D {-} {mar}.@.M {\0}"sv));

        TextDateTest(t, "3/31/2011 3:02", LANG_UNK,
                     Ct("[ 3/31/2011 ] { } {3}.#.M {/} {31}.#.D {/} {2011}.#.Y { }"sv,
                        "[ 3:02\0] { 3:02\0}.T"sv));

        TextDateTest(t, "2012 » 12 » 21", LANG_UNK,
                     Ct("[ 2012 >> 12 >> 21\0] { } {2012}.#.Y { >> } {12}.#.M { >> } {21}.#.D {\0}"sv));

        TextDateTest(t, "March 29-March 30 - RAF planes bomb the Torrey Canyon and sink it", LANG_UNK,
                     Ct("[ march 29-march 30 ] { } {march}.@.M { } {29}.#.D {-} {march}.@.M { } {30}.#.D { }"sv));

        TextDateTest(t, "№ 03-11-02/19", LANG_UNK,
                     Ct("[ 03-11-02/19\0] { 03-11-02/19\0}.J"sv));

        TextDateTest(t, "26.12.2008 11:31:11 ", LANG_UNK,
                     Ct("[ 26.12.2008 ] { } {26}.#.D {.} {12}.#.M {.} {2008}.#.Y { }"sv,
                        "[ 11:31:11 ] { 11:31:11 }.T"sv));

        TextDateTest(t, "(86133) 3-12-40, 3-14-96", LANG_UNK,
                     Ct("[ (86133) 3-12-40, 3-14-96\0] { (86133) 3-12-40, 3-14-96\0}.J"sv));

        TextDateTest(t, "Лицензия НБ РБ №7 от 29.12.2008", LANG_UNK,
                     Ct("[ 29.12.2008\0] { } {29}.#.D {.} {12}.#.M {.} {2008}.#.Y {\0}"sv));

        TextDateTest(t, "ASUS Update v.7.14.02", LANG_UNK,
                     Ct("[ v.7.14.02\0] { v.7.14.02\0}.J"sv));

        TextDateTest(t, "1.may.09.Fine.", LANG_UNK,
                     Ct("[ 1.may.09.] { } {1}.#.D {.} {may}.@.M {.} {09}.#.Y {.}"sv));

        TextDateTest(t, "Good.1.may.09.", LANG_UNK,
                     Ct("[.1.may.09.] {.} {1}.#.D {.} {may}.@.M {.} {09}.#.Y {.}"sv));

        TextDateTest(t, "Интернет-магазин Ozon.ru - 12:12, 19.12.2008", LANG_UNK,
                     Ct("[ 12:12, 19.12.2008\0] { } {12:12}.T {, }.T {19}.#.D {.} {12}.#.M {.} {2008}.#.Y {\0}"sv));

        TextDateTest(t, "August 1, 2005 © MIT Send E-mail (Webmaster)", LANG_UNK,
                     Ct("[ august 1, 2005 ] { } {august}.@.M { } {1}.#.D {, } {2005}.#.Y { }"sv));

        TextDateTest(t, "22-Aug-2006 12:12", LANG_UNK,
                     Ct("[ 22-aug-2006 ] { } {22}.#.D {-} {aug}.@.M {-} {2006}.#.Y { }"sv,
                        "[ 12:12\0] { 12:12\0}.T"sv));

        TextDateTest(t, "쀘 posted on 2008-06-18 at 19:38:15 by nataly", LANG_UNK,
                     Ct("[ 2008-06-18 ] { } {2008}.#.Y {-} {06}.#.M {-} {18}.#.D { }"sv,
                        "[ 19:38:15 ] { 19:38:15 }.T"sv));

        TextDateTest(t, "Mumby published on November 3rd, 2010 @ 09:43:11 pm", LANG_UNK,
                     Ct("[ november 3rd, 2010 ] { } {november}.@.M { } {3}.#.D {rd} {, } {2010}.#.Y { }"sv,
                        "[ 09:43:11 pm\0] { 09:43:11 pm\0}.T"sv));

        TextDateTest(t, ". October 25, 2010 03:49 PM", LANG_UNK,
                     Ct("[ october 25, 2010 ] { } {october}.@.M { } {25}.#.D {, } {2010}.#.Y { }"sv,
                        "[ 03:49 pm\0] { 03:49 pm\0}.T"sv));
        TextDateTest(t, "4.8.2009, 16:13", LANG_UNK,
                     Ct("[ 4.8.2009,] { } {4}.#.D {.} {8}.#.M {.} {2009}.#.Y {,}"sv,
                        "[ 16:13\0] { 16:13\0}.T"sv));
        TextDateTest(t, "/ 10.2.2009, 15:25 ", LANG_UNK,
                     Ct("[ 10.2.2009,] { } {10}.#.D {.} {2}.#.M {.} {2009}.#.Y {,}"sv,
                        "[ 15:25 ] { 15:25 }.T"sv));
        TextDateTest(t, "/ 14-Oct-2000 20:42 - ", LANG_UNK,
                     Ct("[ 14-oct-2000 ] { } {14}.#.D {-} {oct}.@.M {-} {2000}.#.Y { }"sv,
                        "[ 20:42 ] { 20:42 }.T"sv));

        TextDateTest(t, "VN:F [1.9.10_1130]", LANG_UNK,
                     Ct("[[1.9.] {[1.9.}.J"sv,
                        "[_1130]] {_1130]}.J"sv));
        // todo: weird scanner bug
//        TextDateTest(t, "Help File: November 1995 12/1", LANG_UNK,
//                     Ct(TStringBuf()));
//        TextDateTest(t, "Help File: November 1995 12/11/95 S15643", LANG_UNK,
//                     Ct(TStringBuf()));
    }

    Y_UNIT_TEST(TestRusTextScan) {
        using namespace ND2;
        TTestContext t;
        ////////////////////////////////////////////////////////////////
        // Russia
        ////////////////////////////////////////////////////////////////

        TextDateTest(t, "«25» сентября 2008 года", LANG_RUS,
                     Ct("[ cентября 2008 года\0] { } {cентября}.@.M { } {2008}.#.Y { г} {о} {д} {а} {\0}"sv));
        TextDateTest(t, "c 25 августа по 11 сентября 2012", LANG_RUS,
                     Ct("[ 25 авгуcта по 11 cентября 2012\0] { } {25}.#.D { } {авгуcта}.@.M { по } {11}.#.D { } {cентября}.@.M { } {2012}.#.Y {\0}"sv));
        TextDateTest(t, "25 августа – 11 сентября 2012", LANG_RUS,
                     Ct("[ 25 авгуcта - 11 cентября 2012\0] { } {25}.#.D { } {авгуcта}.@.M { - } {11}.#.D { } {cентября}.@.M { } {2012}.#.Y {\0}"sv));
        TextDateTest(t, "10.10.09г.", LANG_RUS,
                     Ct("[ 10.10.09г] { } {10}.#.D {.} {10}.#.M {.} {09}.#.Y {г}"sv));
        TextDateTest(t, "ноябрь, 1е, 2012г", LANG_RUS,
                     Ct("[ ноябрь, 1е, 2012г\0] { } {ноябрь}.@.M {, } {1}.#.D {е} {, } {2012}.#.Y {г} {\0}"sv));
        TextDateTest(t, "10-ого Ноября 2012", LANG_RUS,
                     Ct("[ 10-ого ноября 2012\0] { } {10}.#.D {-} {ого} { } {ноября}.@.M { } {2012}.#.Y {\0}"sv));
        TextDateTest(t, "10ое, ноябрь 2012", LANG_RUS,
                     Ct("[ 10ое, ноябрь 2012\0] { } {10}.#.D {ое} {, } {ноябрь}.@.M { } {2012}.#.Y {\0}"sv));
        TextDateTest(t, "10 НОЯБ. 2012", LANG_RUS,
                     Ct("[ 10 нояб. 2012\0] { } {10}.#.D { } {нояб}.@.M {. } {2012}.#.Y {\0}"sv));
        TextDateTest(t, "10 НОЯБ 2012", LANG_RUS,
                     Ct("[ 10 нояб 2012\0] { } {10}.#.D { } {нояб}.@.M { } {2012}.#.Y {\0}"sv));
        TextDateTest(t, "10 ноя 99", LANG_RUS,
                     Ct("[ 10 ноя 99\0] { } {10}.#.D { } {ноя}.@.M { } {99}.#.Y {\0}"sv));
        TextDateTest(t, "10 НОЯБ 12", LANG_RUS,
                     Ct("[ 10 нояб 12\0] { } {10}.#.D { } {нояб}.@.M { } {12}.#.Y {\0}"sv));
        TextDateTest(t, "1999г.", LANG_RUS,
                     Ct("[ 1999г] { } {1999}.#.Y {г}"sv));
        TextDateTest(t, "Пятница, 8 мая 2009 г.", LANG_RUS,
                     Ct("[ 8 мая 2009 г.\0] { } {8}.#.D { } {мая}.@.M { } {2009}.#.Y { г} {.} {\0}"sv));
        TextDateTest(t, "май 99г.", LANG_RUS,
                     Ct("[ май 99г.\0] { } {май}.@.M { } {99}.#.Y {г} {.} {\0}"sv));
        TextDateTest(t, "99г, май", LANG_RUS,
                     Ct("[ 99г, май\0] { } {99}.#.Y {г} {, } {май}.@.M {\0}"sv));
        TextDateTest(t, "Последнее обновление: 07.11.04 14:46", LANG_RUS,
                     Ct("[ 07.11.04 ] { } {07}.#.D {.} {11}.#.M {.} {04}.#.Y { }"sv,
                        "[ 14:46\0] { 14:46\0}.T"sv));
        TextDateTest(t, "Все права защищены 2009", LANG_RUS,
                     Ct("[ 2009\0] { } {2009}.#.Y {\0}"sv));
        TextDateTest(t, "Дата регистрации: 2008-05-16 | Подробная информация", LANG_RUS,
                     Ct("[ 2008-05-16 ] { } {2008}.#.Y {-} {05}.#.M {-} {16}.#.D { }"sv));
        TextDateTest(t, "Наш телефон 8 (0112) 10-02-98", LANG_RUS,
                     Ct("[ телефон 8 (0112) 10-02-98\0] { телефон 8 (0112) 10-02-98\0}.J"sv));
        TextDateTest(t, "Товар был добавлен в наш каталог 21 Июня 2006г.", LANG_RUS,
                     Ct("[ 21 июня 2006г.\0] { } {21}.#.D { } {июня}.@.M { } {2006}.#.Y {г} {.} {\0}"sv));
        TextDateTest(t, "Товар был добавлен в наш каталог 21 June 2006г.", LANG_RUS,
                     Ct("[ 21 june 2006г] { } {21}.#.D { } {june}.@.M { } {2006}.#.Y {г}"sv));
        TextDateTest(t, "4 мая 2009 ", LANG_RUS,
                     Ct("[ 4 мая 2009 ] { } {4}.#.D { } {мая}.@.M { } {2009}.#.Y { }"sv));
        TextDateTest(t, "2008-Сентябрь", LANG_RUS,
                     Ct("[ 2008-cентябрь\0] { } {2008}.#.Y {-} {cентябрь}.@.M {\0}"sv));
        TextDateTest(t, "2008 Сентябрь - С мыслью по жизни", LANG_RUS,
                     Ct("[ 2008 cентябрь ] { } {2008}.#.Y { } {cентябрь}.@.M { }"sv));
        TextDateTest(t, "2008 , Сентябрь", LANG_RUS,
                     Ct("[ 2008 , cентябрь\0] { } {2008}.#.Y { , } {cентябрь}.@.M {\0}"sv));
        TextDateTest(t, "Сентябрь-2008г", LANG_RUS,
                     Ct("[ cентябрь-2008г\0] { } {cентябрь}.@.M {-} {2008}.#.Y {г} {\0}"sv));
        TextDateTest(t, "12 Апр, 12 ч. и 9 мин.", LANG_RUS,
                     Ct("[ 12 апр, ] { } {12}.#.D { } {апр}.@.M {, }"sv,
                        "[ 12 ч. и 9 мин.\0] { 12 ч. и 9 мин.\0}.T"sv));
        TextDateTest(t, "фотку своей ауди в обмен предложишь))) Mug-Hunter  [24.11.2005 12:18:28]", LANG_RUS,
                     Ct("[[24.11.2005 ] {[} {24}.#.D {.} {11}.#.M {.} {2005}.#.Y { }"sv,
                        "[ 12:18:28]] { 12:18:28]}.T"sv));
        TextDateTest(t, "родился 22 ноября 1962 года в г. Москве.", LANG_RUS,
                     Ct("[ 22 ноября 1962 года ] { } {22}.#.D { } {ноября}.@.M { } {1962}.#.Y { г} {о} {д} {а} { }"sv));
        TextDateTest(t, "15Сентября2007", LANG_RUS,
                     Ct("[ 15cентября2007\0] { } {15}.#.D {cентября}.@.M {2007}.#.Y {\0}"sv));
        TextDateTest(t, "СССР (12.07.1938 г. «Строитель Востока»", LANG_RUS,
                     Ct("[(12.07.1938 ] {(} {12}.#.D {.} {07}.#.M {.} {1938}.#.Y { }"sv));
        TextDateTest(t, "Добавлено: Вс Мар 12, 2006 21:51", LANG_RUS,
                     Ct("[ мар 12, 2006 ] { } {мар}.@.M { } {12}.#.D {, } {2006}.#.Y { }"sv,
                        "[ 21:51\0] { 21:51\0}.T"sv));
        TextDateTest(t, "7 Дек 06", LANG_RUS,
                     Ct("[ 7 дек 06\0] { } {7}.#.D { } {дек}.@.M { } {06}.#.Y {\0}"sv));
        TextDateTest(t, "17 августа, 15:06", LANG_RUS,
                     Ct("[ 17 авгуcта, ] { } {17}.#.D { } {авгуcта}.@.M {, }"sv,
                        "[ 15:06\0] { 15:06\0}.T"sv));
        TextDateTest(t, "01.08.2009Спорт", LANG_RUS,
                     Ct("[ 01.08.2009c] { } {01}.#.D {.} {08}.#.M {.} {2009}.#.Y {c}"sv));
        TextDateTest(t, "3 мая'09", LANG_RUS,
                     Ct("[ 3 мая'09\0] { } {3}.#.D { } {мая}.@.M {'} {09}.#.Y {\0}"sv));
        TextDateTest(t, "3мая", LANG_RUS,
                     Ct("[ 3мая\0] { } {3}.#.D {мая}.@.M {\0}"sv));
        TextDateTest(t, "май,3", LANG_RUS,
                     Ct("[ май,3\0] { } {май}.@.M {,} {3}.#.D {\0}"sv));
        TextDateTest(t, "28 апреля 2009.", LANG_RUS,
                     Ct("[ 28 апреля 2009.] { } {28}.#.D { } {апреля}.@.M { } {2009}.#.Y {.}"sv));
        TextDateTest(t, "28 апреля'09, 02.10", LANG_RUS,
                     Ct("[ 28 апреля'09,] { } {28}.#.D { } {апреля}.@.M {'} {09}.#.Y {,}"sv,
                        "[ 02.10\0] { 02.10\0}.T"sv));
        TextDateTest(t, "28 апреля'09, 02.10 e", LANG_RUS,
                     Ct("[ 28 апреля'09,] { } {28}.#.D { } {апреля}.@.M {'} {09}.#.Y {,}"sv,
                        "[ 02.10 ] { 02.10 }.T"sv));
        TextDateTest(t, ": 28 апреля 2009.", LANG_RUS,
                     Ct("[ 28 апреля 2009.] { } {28}.#.D { } {апреля}.@.M { } {2009}.#.Y {.}"sv));
        TextDateTest(t, "Последнее изменение этой страницы: 17:44, 28 апреля 2009.", LANG_RUS,
                     Ct("[ 17:44,] { 17:44,}.T"sv,
                        "[ 28 апреля 2009.] { } {28}.#.D { } {апреля}.@.M { } {2009}.#.Y {.}"sv));
        TextDateTest(t, "Последнее изменение этой страницы: 17 - 21, апрель, 12, 2009.", LANG_RUS,
                     Ct("[ 17 - 21,] { 17 - 21,}.T"sv,
                        "[ апрель, 12, 2009.] { } {апрель}.@.M {, } {12}.#.D {, } {2009}.#.Y {.}"sv));
        TextDateTest(t, "на февраль 2007.", LANG_RUS,
                     Ct("[ февраль 2007.] { } {февраль}.@.M { } {2007}.#.Y {.}"sv));
        TextDateTest(t, "2002-сент.2003", LANG_RUS,
                     Ct("[ 2002-cент.2003\0] { } {2002}.#.Y {-} {cент}.@.M {.} {2003}.#.Y {\0}"sv));
        TextDateTest(t, "сент.2003-2005", LANG_RUS,
                     Ct("[ cент.2003-2005\0] { } {cент}.@.M {.} {2003}.#.Y {-} {2005}.#.Y {\0}"sv));
        TextDateTest(t, "Погода на май месяц 2011 года по Владимирской области.", LANG_RUS,
                     Ct("[ май меcяц 2011 года ] { } {май}.@.M { меcяц} { } {2011}.#.Y { г} {о} {д} {а} { }"sv));
        TextDateTest(t, "Об изменениях в программах БухСофт 2009 можно ознакомиться", LANG_RUS,
                     Ct("[ 2009 ] { } {2009}.#.Y { }"sv));
        TextDateTest(t, "Итогом этой инициативы стало проведение 24 и 25 ноября 2005 года Всероссийского форума", LANG_RUS,
                     Ct("[ 24 и 25 ноября 2005 года ] { } {24}.#.D { } {и } {25}.#.D { } {ноября}.@.M { } {2005}.#.Y { г} {о} {д} {а} { }"sv));
        TextDateTest(t, "© 1999-2009, Маркетинговая", LANG_RUS,
                     Ct("[ 1999-] { } {1999}.#.Y {-}"sv,
                        "[-2009,] {-} {2009}.#.Y {,}"sv));
        TextDateTest(t, "Reply #1 : 21 Январь 2009", LANG_RUS,
                     Ct("[ 21 январь 2009\0] { } {21}.#.D { } {январь}.@.M { } {2009}.#.Y {\0}"sv));
//        TextDateTest(t, "Восхождение на Мера пик, октябрь 2007/Mera2-117", LANG_RUS,
//                     Ct("[ октябрь 2007/] { } {октябрь}.@.M { } {2007}.#.Y {/}"sv));
        TextDateTest(t, "AndMan@nakley-ka.ru. 30.06.2009 19:20", LANG_RUS,
                     Ct("[ 30.06.2009 ] { } {30}.#.D {.} {06}.#.M {.} {2009}.#.Y { }"sv,
                        "[ 19:20\0] { 19:20\0}.T"sv));
        TextDateTest(t, "09:53 - 31 января 2006 года", LANG_RUS,
                     Ct("[ 09:53 ] { 09:53 }.T"sv,
                        "[ 31 января 2006 года\0] { } {31}.#.D { } {января}.@.M { } {2006}.#.Y { г} {о} {д} {а} {\0}"sv));
        TextDateTest(t, "23.6.2011. 14:08", LANG_RUS,
                     Ct("[ 23.6.2011.] { } {23}.#.D {.} {6}.#.M {.} {2011}.#.Y {.}"sv,
                        "[ 14:08\0] { 14:08\0}.T"sv));
        TextDateTest(t, "Май   Июнь 2011   Июль", LANG_RUS,
                     Ct("[ май июнь 2011 ] { } {май}.@.M { } {июнь}.@.M { } {2011}.#.Y { }"sv));
        TextDateTest(t, "20 2 2008", LANG_RUS,
                     Ct("[ 20 2 2008\0] { } {20}.#.D { } {2}.#.M { } {2008}.#.Y {\0}"sv));
        TextDateTest(t, "23.12 22:27", LANG_RUS,
                     Ct("[ 23.12 22:27\0] { } {23}.D {.} {12}.M { } {22:27}.T {\0}"sv));
        TextDateTest(t, "Просмотров: 2246  28 марта 2008", LANG_RUS,
                     Ct("[ 2246 ] { 2246 }.J"sv,
                        "[ 28 марта 2008\0] { } {28}.#.D { } {марта}.@.M { } {2008}.#.Y {\0}"sv));

        TextDateTest(t, "июнь 2008 г., март 2009 г.", LANG_RUS,
                     Ct("[ июнь 2008 г., март 2009 г.\0] { } {июнь}.@.M { } {2008}.#.Y { г.} {, } {март}.@.M { } {2009}.#.Y { г} {.} {\0}"sv));
        TextDateTest(t, "Окт. 1939 — июль 1941 — Букрин, Андрей Николаевич \n"
                     "Июль 1941 — июль 1942 — Нестеров, Сергей Васеньевич \n"
                     "Июль 1942 — авг. 1944 — Павлов, Василий Дмитриевич \n"
                     "Авг. 1944 — нояб. 1946 — Беляев, Павел Степанович \n", LANG_RUS,
                     Ct("[ окт. 1939 - июль 1941 ] { } {окт}.@.M {. } {1939}.#.Y { - } {июль}.@.M { } {1941}.#.Y { }"sv,
                        "[ июль 1941 - июль 1942 ] { } {июль}.@.M { } {1941}.#.Y { - } {июль}.@.M { } {1942}.#.Y { }"sv,
                        "[ июль 1942 - авг. 1944 ] { } {июль}.@.M { } {1942}.#.Y { - } {авг}.@.M {. } {1944}.#.Y { }"sv,
                        "[ авг. 1944 - нояб. 1946 ] { } {авг}.@.M {. } {1944}.#.Y { - } {нояб}.@.M {. } {1946}.#.Y { }"sv));

        TextDateTest(t, "\"Российская газета\" - Столичный выпуск №5561 (185)\n23.08.2011, 00:24 ", LANG_RUS,
                     Ct("[№5561 ] {№5561 }.J"sv,
                        "[(185)] {(185)}.J"sv,
                        "[ 23.08.2011,] { } {23}.#.D {.} {08}.#.M {.} {2011}.#.Y {,}"sv,
                        "[ 00:24 ] { 00:24 }.T"sv));

        TextDateTest(t, "17.02 Главная+дорога++17.02.2007.avi (170,13 мегабайт)", LANG_RUS,
                     Ct("[ 17.02 ] { 17.02 }.T"sv,
                        "[+17.02.2007.] {+} {17}.#.D {.} {02}.#.M {.} {2007}.#.Y {.}"sv,
                        "[(170,] {(170,}.J"sv));
        // todo: weird scanner bug
        // TextDateTest(t, "Октябрь 2010 (15)", LANG_RUS);
    }

    Y_UNIT_TEST(TestKazTextScan) {
        using namespace ND2;
        TTestContext t;

        ////////////////////////////////////////////////////////////////
        // Kazakhstan
        ////////////////////////////////////////////////////////////////

        TextDateTest(t, "2011 » Қаңтар » 28", LANG_KAZ,
                     Ct("[ 2011 >> кантар >> 28\0] { } {2011}.#.Y { >> } {кантар}.@.M { >> } {28}.#.D {\0}"sv));
        TextDateTest(t, "2011 » Ақпан » 11", LANG_KAZ,
                     Ct("[ 2011 >> акпан >> 11\0] { } {2011}.#.Y { >> } {акпан}.@.M { >> } {11}.#.D {\0}"sv));
        TextDateTest(t, "2010 » Қыркүйек » 10", LANG_KAZ,
                     Ct("[ 2010 >> кыркуйек >> 10\0] { } {2010}.#.Y { >> } {кыркуйек}.@.M { >> } {10}.#.D {\0}"sv));
        TextDateTest(t, "2010 » Қараша » 5", LANG_KAZ,
                     Ct("[ 2010 >> караша >> 5\0] { } {2010}.#.Y { >> } {караша}.@.M { >> } {5}.#.D {\0}"sv));
        TextDateTest(t, "2010 » Қазан » 29", LANG_KAZ,
                     Ct("[ 2010 >> казан >> 29\0] { } {2010}.#.Y { >> } {казан}.@.M { >> } {29}.#.D {\0}"sv));
        TextDateTest(t, "2010 » Желтоқсан » 15", LANG_KAZ,
                     Ct("[ 2010 >> желтокcан >> 15\0] { } {2010}.#.Y { >> } {желтокcан}.@.M { >> } {15}.#.D {\0}"sv));
        TextDateTest(t, "14 жел 2009, 14:51", LANG_KAZ,
                     Ct("[ 14 жел 2009,] { } {14}.#.D { } {жел}.@.M { } {2009}.#.Y {,}"sv,
                        "[ 14:51\0] { 14:51\0}.T"sv));
        TextDateTest(t, "12 қараша", LANG_KAZ,
                     Ct("[ 12 караша\0] { } {12}.#.D { } {караша}.@.M {\0}"sv));
        TextDateTest(t, "10-Мамыр, 2010", LANG_KAZ,
                     Ct("[ 10-мамыр, 2010\0] { } {10}.#.D {-} {мамыр}.@.M {, } {2010}.#.Y {\0}"sv));
        TextDateTest(t, "1 Қырк. 2009", LANG_KAZ,
                     Ct("[ 1 кырк. 2009\0] { } {1}.#.D { } {кырк}.@.M {.} { } {2009}.#.Y {\0}"sv));

// todo: kaz months
//        TextDateTest(t, "2010 жылдың 14 желтоқсанында", LANG_KAZ,
//                     Ct(" "sv));
    }

    Y_UNIT_TEST(TestBlrTextScan) {
        using namespace ND2;
        TTestContext t;

        ////////////////////////////////////////////////////////////////
        // Belorussia
        ////////////////////////////////////////////////////////////////

        TextDateTest(t, "18 Красавік 2010", LANG_BEL,
                     Ct("[ 18 краcавiк 2010\0] { } {18}.#.D { } {краcавiк}.@.M { } {2010}.#.Y {\0}"sv));
        TextDateTest(t, "2 Красавік 2011", LANG_BEL,
                     Ct("[ 2 краcавiк 2011\0] { } {2}.#.D { } {краcавiк}.@.M { } {2011}.#.Y {\0}"sv));
        TextDateTest(t, "24 снежня 2010", LANG_BEL,
                     Ct("[ 24 cнежня 2010\0] { } {24}.#.D { } {cнежня}.@.M { } {2010}.#.Y {\0}"sv));
        TextDateTest(t, "28 студзеня 2010", LANG_BEL,
                     Ct("[ 28 cтудзеня 2010\0] { } {28}.#.D { } {cтудзеня}.@.M { } {2010}.#.Y {\0}"sv));
        TextDateTest(t, "29 чэрвеня 2010", LANG_BEL,
                     Ct("[ 29 червеня 2010\0] { } {29}.#.D { } {червеня}.@.M { } {2010}.#.Y {\0}"sv));
        TextDateTest(t, "6 Люты 2011", LANG_BEL,
                     Ct("[ 6 люты 2011\0] { } {6}.#.D { } {люты}.@.M { } {2011}.#.Y {\0}"sv));
        TextDateTest(t, "Серада, 02 Студзеня 2008 Года", LANG_BEL,
                     Ct("[ 02 cтудзеня 2008 ] { } {02}.#.D { } {cтудзеня}.@.M { } {2008}.#.Y { }"sv));
        TextDateTest(t, "06 студз. 2008 01:15", LANG_BEL,
                     Ct("[ 06 cтудз. 2008 ] { } {06}.#.D { } {cтудз}.@.M {.} { } {2008}.#.Y { }"sv,
                        "[ 01:15\0] { 01:15\0}.T"sv));
        TextDateTest(t, "Субота, 15 травня 2010, 22:35", LANG_BEL,
                     Ct("[ 15 травня 2010,] { } {15}.#.D { } {травня}.@.M { } {2010}.#.Y {,}"sv,
                        "[ 22:35\0] { 22:35\0}.T"sv));
        TextDateTest(t, "1-Жнв-2010 09:35 pm", LANG_BEL,
                     Ct("[ 1-жнв-2010 ] { } {1}.#.D {-} {жнв}.@.M {-} {2010}.#.Y { }"sv,
                        "[ 09:35 pm\0] { 09:35 pm\0}.T"sv));
        TextDateTest(t, "студзеня 4, 2011", LANG_BEL,
                     Ct("[ cтудзеня 4, 2011\0] { } {cтудзеня}.@.M { } {4}.#.D {, } {2011}.#.Y {\0}"sv));
        TextDateTest(t, "12 Крс 2007", LANG_BEL,
                     Ct("[ 12 крc 2007\0] { } {12}.#.D { } {крc}.@.M { } {2007}.#.Y {\0}"sv));
        TextDateTest(t, "Студзень 12, 2011", LANG_BEL,
                     Ct("[ cтудзень 12, 2011\0] { } {cтудзень}.@.M { } {12}.#.D {, } {2011}.#.Y {\0}"sv));
    }

    Y_UNIT_TEST(TestUkrTextScan) {
        using namespace ND2;
        TTestContext t;

        ////////////////////////////////////////////////////////////////
        // Ukraine
        ////////////////////////////////////////////////////////////////

        TextDateTest(t, "Сер жовтня 17, 2007 17:24:40", LANG_UKR,
                     Ct("[ жовтня 17, 2007 ] { } {жовтня}.@.M { } {17}.#.D {, } {2007}.#.Y { }"sv,
                        "[ 17:24:40\0] { 17:24:40\0}.T"sv));
        TextDateTest(t, "Суб вересня 22, 2007 12:21:15", LANG_UKR,
                     Ct("[ вереcня 22, 2007 ] { } {вереcня}.@.M { } {22}.#.D {, } {2007}.#.Y { }"sv,
                        "[ 12:21:15\0] { 12:21:15\0}.T"sv));
        TextDateTest(t, "Квітень 8, 2010 - 08:01", LANG_UKR,
                     Ct("[ квiтень 8, 2010 ] { } {квiтень}.@.M { } {8}.#.D {, } {2010}.#.Y { }"sv,
                        "[ 08:01\0] { 08:01\0}.T"sv));
        TextDateTest(t, "Грудень 6, 2010 - 13:31", LANG_UKR,
                     Ct("[ грудень 6, 2010 ] { } {грудень}.@.M { } {6}.#.D {, } {2010}.#.Y { }"sv,
                        "[ 13:31\0] { 13:31\0}.T"sv));
        TextDateTest(t, "Березень 9th, 2009", LANG_UKR,
                     Ct("[ березень 9th, 2009\0] { } {березень}.@.M { } {9}.#.D {th} {, } {2009}.#.Y {\0}"sv));
        TextDateTest(t, "9-04-2011, 01:10", LANG_UKR,
                     Ct("[ 9-04-2011,] { } {9}.#.D {-} {04}.#.M {-} {2011}.#.Y {,}"sv,
                        "[ 01:10\0] { 01:10\0}.T"sv));
        TextDateTest(t, "5.1.2008, 12:57", LANG_UKR,
                     Ct("[ 5.1.2008,] { } {5}.#.D {.} {1}.#.M {.} {2008}.#.Y {,}"sv,
                        "[ 12:57\0] { 12:57\0}.T"sv));
        TextDateTest(t, "5 Лютий, 2010 - 00:34", LANG_UKR,
                     Ct("[ 5 лютий, 2010 ] { } {5}.#.D { } {лютий}.@.M {, } {2010}.#.Y { }"sv,
                        "[ 00:34\0] { 00:34\0}.T"sv));
        TextDateTest(t, "30 вер. 2010", LANG_UKR,
                     Ct("[ 30 вер. 2010\0] { } {30}.#.D { } {вер}.@.M {.} { } {2010}.#.Y {\0}"sv));
        TextDateTest(t, "29-Травень-2009 02:57:53", LANG_UKR,
                     Ct("[ 29-травень-2009 ] { } {29}.#.D {-} {травень}.@.M {-} {2009}.#.Y { }"sv,
                        "[ 02:57:53\0] { 02:57:53\0}.T"sv));
        TextDateTest(t, "2010 » Травень » 21", LANG_UKR,
                     Ct("[ 2010 >> травень >> 21\0] { } {2010}.#.Y { >> } {травень}.@.M { >> } {21}.#.D {\0}"sv));
        TextDateTest(t, "2010 » Липень » 20", LANG_UKR,
                     Ct("[ 2010 >> липень >> 20\0] { } {2010}.#.Y { >> } {липень}.@.M { >> } {20}.#.D {\0}"sv));
        TextDateTest(t, "18 травень 2010", LANG_UKR,
                     Ct("[ 18 травень 2010\0] { } {18}.#.D { } {травень}.@.M { } {2010}.#.Y {\0}"sv));
        TextDateTest(t, "18 Лютого 2011, 11:11:15", LANG_UKR,
                     Ct("[ 18 лютого 2011,] { } {18}.#.D { } {лютого}.@.M { } {2011}.#.Y {,}"sv,
                        "[ 11:11:15\0] { 11:11:15\0}.T"sv));
        TextDateTest(t, "15.09.2009, 11:58:26", LANG_UKR,
                     Ct("[ 15.09.2009,] { } {15}.#.D {.} {09}.#.M {.} {2009}.#.Y {,}"sv,
                        "[ 11:58:26\0] { 11:58:26\0}.T"sv));
        TextDateTest(t, "04 лютого 2011", LANG_UKR,
                     Ct("[ 04 лютого 2011\0] { } {04}.#.D { } {лютого}.@.M { } {2011}.#.Y {\0}"sv));
    }

    Y_UNIT_TEST(TestPolTextScan) {
        using namespace ND2;
        TTestContext t;

        ////////////////////////////////////////////////////////////////
        // Poland
        ////////////////////////////////////////////////////////////////

        TextDateTest(t, "wtorek, 14 kwietnia 2009 - 21:45", LANG_POL,
                     Ct("[ 14 kwietnia 2009 ] { } {14}.#.D { } {kwietnia}.@.M { } {2009}.#.Y { }"sv,
                        "[ 21:45\0] { 21:45\0}.T"sv));
        TextDateTest(t, "22:13, 5 kwi 2011", LANG_POL,
                     Ct("[ 22:13,] { 22:13,}.T"sv,
                        "[ 5 kwi 2011\0] { } {5}.#.D { } {kwi}.@.M { } {2011}.#.Y {\0}"sv));
        TextDateTest(t, "16 czerwiec 2005 00:09", LANG_POL,
                     Ct("[ 16 czerwiec 2005 ] { } {16}.#.D { } {czerwiec}.@.M { } {2005}.#.Y { }"sv,
                        "[ 00:09\0] { 00:09\0}.T"sv));
        TextDateTest(t, "16 czerwiec 2007 - 14:05", LANG_POL,
                     Ct("[ 16 czerwiec 2007 ] { } {16}.#.D { } {czerwiec}.@.M { } {2007}.#.Y { }"sv,
                        "[ 14:05\0] { 14:05\0}.T"sv));
        TextDateTest(t, "3 paź 2010, 0:22", LANG_POL,
                     Ct("[ 3 paz 2010,] { } {3}.#.D { } {paz}.@.M { } {2010}.#.Y {,}"sv,
                        "[ 0:22\0] { 0:22\0}.T"sv));
        TextDateTest(t, "2010-11-13, 00:33", LANG_POL,
                     Ct("[ 2010-11-13,] { } {2010}.#.Y {-} {11}.#.M {-} {13}.#.D {,}"sv,
                        "[ 00:33\0] { 00:33\0}.T"sv));
        TextDateTest(t, "Pn lip 14, 2003 11:14 am", LANG_POL,
                     Ct("[ lip 14, 2003 ] { } {lip}.@.M { } {14}.#.D {, } {2003}.#.Y { }"sv,
                        "[ 11:14 am\0] { 11:14 am\0}.T"sv));
        TextDateTest(t, "5 września 2007, 22:07", LANG_POL,
                     Ct("[ 5 wrzesnia 2007,] { } {5}.#.D { } {wrzesnia}.@.M { } {2007}.#.Y {,}"sv,
                        "[ 22:07\0] { 22:07\0}.T"sv));
        TextDateTest(t, "Wrzesień 6, 2007 at 3:30 pm", LANG_POL,
                     Ct("[ wrzesien 6, 2007 ] { } {wrzesien}.@.M { } {6}.#.D {, } {2007}.#.Y { }"sv,
                        "[ 3:30 pm\0] { 3:30 pm\0}.T"sv));
        TextDateTest(t, "29 czerwca 2009, 23:00", LANG_POL,
                     Ct("[ 29 czerwca 2009,] { } {29}.#.D { } {czerwca}.@.M { } {2009}.#.Y {,}"sv,
                        "[ 23:00\0] { 23:00\0}.T"sv));
        TextDateTest(t, "Czerwiec 29, 2009", LANG_POL,
                     Ct("[ czerwiec 29, 2009\0] { } {czerwiec}.@.M { } {29}.#.D {, } {2009}.#.Y {\0}"sv));
        TextDateTest(t, "14 września 2010 10:57", LANG_POL,
                     Ct("[ 14 wrzesnia 2010 ] { } {14}.#.D { } {wrzesnia}.@.M { } {2010}.#.Y { }"sv,
                        "[ 10:57\0] { 10:57\0}.T"sv));
        TextDateTest(t, "23 sierpnia 2010", LANG_POL,
                     Ct("[ 23 sierpnia 2010\0] { } {23}.#.D { } {sierpnia}.@.M { } {2010}.#.Y {\0}"sv));
        TextDateTest(t, "czwartek, 02 września 2010 08:56", LANG_POL,
                     Ct("[ 02 wrzesnia 2010 ] { } {02}.#.D { } {wrzesnia}.@.M { } {2010}.#.Y { }"sv,
                        "[ 08:56\0] { 08:56\0}.T"sv));
        TextDateTest(t, "niedziela, 6 Wrzesień 2009", LANG_POL,
                     Ct("[ 6 wrzesien 2009\0] { } {6}.#.D { } {wrzesien}.@.M { } {2009}.#.Y {\0}"sv));
        TextDateTest(t, "17 września 2009r.", LANG_POL,
                     Ct("[ 17 wrzesnia 2009r] { } {17}.#.D { } {wrzesnia}.@.M { } {2009}.#.Y {r}"sv));
        TextDateTest(t, "niedziela, 31 października 2010 15:51", LANG_POL,
                     Ct("[ 31 pazdziernika 2010 ] { } {31}.#.D { } {pazdziernika}.@.M { } {2010}.#.Y { }"sv,
                        "[ 15:51\0] { 15:51\0}.T"sv));
        TextDateTest(t, "Wtor. 10 sierpień 2010 20h15", LANG_POL,
                     Ct("[ 10 sierpien 2010 ] { } {10}.#.D { } {sierpien}.@.M { } {2010}.#.Y { }"sv,
                        "[ 20h15\0] { 20h15\0}.T"sv));
        TextDateTest(t, "Wtorek 10 sierpień 2010 20h15", LANG_POL,
                     Ct("[ 10 sierpien 2010 ] { } {10}.#.D { } {sierpien}.@.M { } {2010}.#.Y { }"sv,
                        "[ 20h15\0] { 20h15\0}.T"sv));
        TextDateTest(t, "Sierpień 10, 2010", LANG_POL,
                     Ct("[ sierpien 10, 2010\0] { } {sierpien}.@.M { } {10}.#.D {, } {2010}.#.Y {\0}"sv));
        TextDateTest(t, "22 Sierpień 2009", LANG_POL,
                     Ct("[ 22 sierpien 2009\0] { } {22}.#.D { } {sierpien}.@.M { } {2009}.#.Y {\0}"sv));
    }

    Y_UNIT_TEST(TestCzeTextScan) {
        using namespace ND2;
        TTestContext t;

        ////////////////////////////////////////////////////////////////
        // Czech
        ////////////////////////////////////////////////////////////////

        TextDateTest(t, "17. červen 2010", LANG_CZE,
                     Ct("[ 17. cerven 2010\0] { } {17}.#.D {.} { } {cerven}.@.M { } {2010}.#.Y {\0}"sv));
        TextDateTest(t, "úte 13. led 2009 23:10:37", LANG_CZE,
                     Ct("[ 13. led 2009 ] { } {13}.#.D {.} { } {led}.@.M { } {2009}.#.Y { }"sv,
                        "[ 23:10:37\0] { 23:10:37\0}.T"sv));
        TextDateTest(t, "17 Říj 2010 - 12:04", LANG_CZE,
                     Ct("[ 17 rij 2010 ] { } {17}.#.D { } {rij}.@.M { } {2010}.#.Y { }"sv,
                        "[ 12:04\0] { 12:04\0}.T"sv));
        TextDateTest(t, "1. května 2009", LANG_CZE,
                     Ct("[ 1. kvetna 2009\0] { } {1}.#.D {.} { } {kvetna}.@.M { } {2009}.#.Y {\0}"sv));
        TextDateTest(t, "Září, 2009", LANG_CZE,
                     Ct("[ zari, 2009\0] { } {zari}.@.M {, } {2009}.#.Y {\0}"sv));
        TextDateTest(t, "31. května 2009 v 13:57", LANG_CZE,
                     Ct("[ 31. kvetna 2009 ] { } {31}.#.D {.} { } {kvetna}.@.M { } {2009}.#.Y { }"sv,
                        "[ 13:57\0] { 13:57\0}.T"sv));
        TextDateTest(t, "31. ledna 2009 v 17:28", LANG_CZE,
                     Ct("[ 31. ledna 2009 ] { } {31}.#.D {.} { } {ledna}.@.M { } {2009}.#.Y { }"sv,
                        "[ 17:28\0] { 17:28\0}.T"sv));
        TextDateTest(t, "čtvrtek, 29. ledna 2009", LANG_CZE,
                     Ct("[ 29. ledna 2009\0] { } {29}.#.D {.} { } {ledna}.@.M { } {2009}.#.Y {\0}"sv));
        TextDateTest(t, "16. srpna 2008 v 17:06", LANG_CZE,
                     Ct("[ 16. srpna 2008 ] { } {16}.#.D {.} { } {srpna}.@.M { } {2008}.#.Y { }"sv,
                        "[ 17:06\0] { 17:06\0}.T"sv));
        TextDateTest(t, "1 Zář 2010 13:58", LANG_CZE,
                     Ct("[ 1 zar 2010 ] { } {1}.#.D { } {zar}.@.M { } {2010}.#.Y { }"sv,
                        "[ 13:58\0] { 13:58\0}.T"sv));
        TextDateTest(t, "22 Lis 2010 20:19", LANG_CZE,
                     Ct("[ 22 lis 2010 ] { } {22}.#.D { } {lis}.@.M { } {2010}.#.Y { }"sv,
                        "[ 20:19\0] { 20:19\0}.T"sv));
        TextDateTest(t, "Březen 7, 2011", LANG_CZE,
                     Ct("[ brezen 7, 2011\0] { } {brezen}.@.M { } {7}.#.D {, } {2011}.#.Y {\0}"sv));
        TextDateTest(t, "Pondělí, Březen 7 2011", LANG_CZE,
                     Ct("[ brezen 7 2011\0] { } {brezen}.@.M { } {7}.#.D { } {2011}.#.Y {\0}"sv));
        TextDateTest(t, "Červenec 2010", LANG_CZE,
                     Ct("[ cervenec 2010\0] { } {cervenec}.@.M { } {2010}.#.Y {\0}"sv));
        TextDateTest(t, "24. července 2010 v 5:45", LANG_CZE,
                     Ct("[ 24. cervence 2010 ] { } {24}.#.D {.} { } {cervence}.@.M { } {2010}.#.Y { }"sv,
                        "[ 5:45\0] { 5:45\0}.T"sv));
    }

    Y_UNIT_TEST(TestTurTextScan) {
        using namespace ND2;
        TTestContext t;

        ////////////////////////////////////////////////////////////////
        // Turkey
        ////////////////////////////////////////////////////////////////

        TextDateTest(t, "Şubat 12, 2010", LANG_TUR,
                     Ct("[ subat 12, 2010\0] { } {subat}.@.M { } {12}.#.D {, } {2010}.#.Y {\0}"sv));
        TextDateTest(t, "Nisan 21, 2009", LANG_TUR,
                     Ct("[ nisan 21, 2009\0] { } {nisan}.@.M { } {21}.#.D {, } {2009}.#.Y {\0}"sv));
        TextDateTest(t, "Nisan 2011", LANG_TUR,
                     Ct("[ nisan 2011\0] { } {nisan}.@.M { } {2011}.#.Y {\0}"sv));
        TextDateTest(t, "30 Mayıs 2006", LANG_TUR,
                     Ct("[ 30 mayis 2006\0] { } {30}.#.D { } {mayis}.@.M { } {2006}.#.Y {\0}"sv));
        TextDateTest(t, "29-06-2009, 19:47", LANG_TUR,
                     Ct("[ 29-06-2009,] { } {29}.#.D {-} {06}.#.M {-} {2009}.#.Y {,}"sv,
                        "[ 19:47\0] { 19:47\0}.T"sv));
        TextDateTest(t, "26 Eylül 2010", LANG_TUR,
                     Ct("[ 26 eylul 2010\0] { } {26}.#.D { } {eylul}.@.M { } {2010}.#.Y {\0}"sv));
        TextDateTest(t, "26 Aralık 2009, 10:19:26", LANG_TUR,
                     Ct("[ 26 aralik 2009,] { } {26}.#.D { } {aralik}.@.M { } {2009}.#.Y {,}"sv,
                        "[ 10:19:26\0] { 10:19:26\0}.T"sv));
        TextDateTest(t, "24 Eylül 2010", LANG_TUR,
                     Ct("[ 24 eylul 2010\0] { } {24}.#.D { } {eylul}.@.M { } {2010}.#.Y {\0}"sv));
        TextDateTest(t, "22 Nisan 2009 - 2:27pm", LANG_TUR,
                     Ct("[ 22 nisan 2009 ] { } {22}.#.D { } {nisan}.@.M { } {2009}.#.Y { }"sv,
                        "[ 2:27pm\0] { 2:27pm\0}.T"sv));
        TextDateTest(t, "6 Nis 2011", LANG_TUR,
                     Ct("[ 6 nis 2011\0] { } {6}.#.D { } {nis}.@.M { } {2011}.#.Y {\0}"sv));
        TextDateTest(t, "22 Eylül 2010", LANG_TUR,
                     Ct("[ 22 eylul 2010\0] { } {22}.#.D { } {eylul}.@.M { } {2010}.#.Y {\0}"sv));
        TextDateTest(t, "18.Oca.2010", LANG_TUR,
                     Ct("[ 18.oca.2010\0] { } {18}.#.D {.} {oca}.@.M {.} {2010}.#.Y {\0}"sv));
        TextDateTest(t, "14 Ocak 2008 19:07", LANG_TUR,
                     Ct("[ 14 ocak 2008 ] { } {14}.#.D { } {ocak}.@.M { } {2008}.#.Y { }"sv,
                        "[ 19:07\0] { 19:07\0}.T"sv));
        TextDateTest(t, "11 Şub 2011 21:18", LANG_TUR,
                     Ct("[ 11 sub 2011 ] { } {11}.#.D { } {sub}.@.M { } {2011}.#.Y { }"sv,
                        "[ 21:18\0] { 21:18\0}.T"sv));
        TextDateTest(t, "04 Kas 2009 21:48", LANG_TUR,
                     Ct("[ 04 kas 2009 ] { } {04}.#.D { } {kas}.@.M { } {2009}.#.Y { }"sv,
                        "[ 21:48\0] { 21:48\0}.T"sv));
        TextDateTest(t, "09 Oca 2010", LANG_TUR,
                     Ct("[ 09 oca 2010\0] { } {09}.#.D { } {oca}.@.M { } {2010}.#.Y {\0}"sv));
        TextDateTest(t, "08-11-2010 / 18:49:53", LANG_TUR,
                     Ct("[ 08-11-2010 ] { } {08}.#.D {-} {11}.#.M {-} {2010}.#.Y { }"sv,
                        "[ 18:49:53\0] { 18:49:53\0}.T"sv));
        TextDateTest(t, "06 Nisan 2011", LANG_TUR,
                     Ct("[ 06 nisan 2011\0] { } {06}.#.D { } {nisan}.@.M { } {2011}.#.Y {\0}"sv));
        TextDateTest(t, "Aralık, 2010", LANG_TUR,
                     Ct("[ aralik, 2010\0] { } {aralik}.@.M {, } {2010}.#.Y {\0}"sv));
        TextDateTest(t, "06 Nis 2011 - 05:33", LANG_TUR,
                     Ct("[ 06 nis 2011 ] { } {06}.#.D { } {nis}.@.M { } {2011}.#.Y { }"sv,
                        "[ 05:33\0] { 05:33\0}.T"sv));
        TextDateTest(t, "Jan 2011", LANG_TUR,
                     Ct("[ jan 2011\0] { } {jan}.@.M { } {2011}.#.Y {\0}"sv));
        TextDateTest(t, "14:01 - Pazartesi, Haziran 1, 2009", LANG_TUR,
                     Ct("[ 14:01 ] { 14:01 }.T"sv,
                        "[ haziran 1, 2009\0] { } {haziran}.@.M { } {1}.#.D {, } {2009}.#.Y {\0}"sv));
        TextDateTest(t, "01 April 2011", LANG_TUR,
                     Ct("[ 01 april 2011\0] { } {01}.#.D { } {april}.@.M { } {2011}.#.Y {\0}"sv));
        TextDateTest(t, "14. kasım. 2010", LANG_TUR,
                     Ct("[ 14. kasim. 2010\0] { } {14}.#.D {.} { } {kasim}.@.M {.} { } {2010}.#.Y {\0}"sv));
        TextDateTest(t, "Protokollerin 10 Ekimde imzalanması bekleniyor", LANG_TUR,
                     Ct("[ 10 ekimde ] { } {10}.#.D { } {ekim}.@.M {de} { }"sv));

    }

    Y_UNIT_TEST(TestGerTextScan) {
        using namespace ND2;
        TTestContext t;

        ////////////////////////////////////////////////////////////////
        // Germany
        ////////////////////////////////////////////////////////////////

        TextDateTest(t, "18.01.2011, 10:38 Uhr", LANG_GER,
                     Ct("[ 18.01.2011,] { } {18}.#.D {.} {01}.#.M {.} {2011}.#.Y {,}"sv,
                        "[ 10:38 ] { 10:38 }.T"sv));
        TextDateTest(t, "25. Mai 2006", LANG_GER,
                     Ct("[ 25. mai 2006\0] { } {25}.#.D {.} { } {mai}.@.M { } {2006}.#.Y {\0}"sv));
        TextDateTest(t, "6. April 2011, 19:37 Uhr", LANG_GER,
                     Ct("[ 6. april 2011,] { } {6}.#.D {.} { } {april}.@.M { } {2011}.#.Y {,}"sv,
                        "[ 19:37 ] { 19:37 }.T"sv));
        TextDateTest(t, "Dienstag, 10. Oktober 2006 14:12", LANG_GER,
                     Ct("[ 10. oktober 2006 ] { } {10}.#.D {.} { } {oktober}.@.M { } {2006}.#.Y { }"sv,
                        "[ 14:12\0] { 14:12\0}.T"sv));
        TextDateTest(t, "Februar 27, 2010", LANG_GER,
                     Ct("[ februar 27, 2010\0] { } {februar}.@.M { } {27}.#.D {, } {2010}.#.Y {\0}"sv));
        TextDateTest(t, "Januar 20, 2009", LANG_GER,
                     Ct("[ januar 20, 2009\0] { } {januar}.@.M { } {20}.#.D {, } {2009}.#.Y {\0}"sv));
        TextDateTest(t, "November 7th, 2010 - 19:35", LANG_GER,
                     Ct("[ november 7th, 2010 ] { } {november}.@.M { } {7}.#.D {th} {, } {2010}.#.Y { }"sv,
                        "[ 19:35\0] { 19:35\0}.T"sv));
        TextDateTest(t, "Sonnabend, den 23. April", LANG_GER,
                     Ct("[ 23. april\0] { } {23}.#.D {. } {april}.@.M {\0}"sv));
        TextDateTest(t, "1. Okt. 2008", LANG_GER,
                     Ct("[ 1. okt. 2008\0] { } {1}.#.D {.} { } {okt}.@.M {.} { } {2008}.#.Y {\0}"sv));
        TextDateTest(t, "So 10. Jan 2010, 23:08", LANG_GER,
                     Ct("[ 10. jan 2010,] { } {10}.#.D {.} { } {jan}.@.M { } {2010}.#.Y {,}"sv,
                        "[ 23:08\0] { 23:08\0}.T"sv));
        TextDateTest(t, "Mi 5. Jan 2011, 15:07", LANG_GER,
                     Ct("[ 5. jan 2011,] { } {5}.#.D {.} { } {jan}.@.M { } {2011}.#.Y {,}"sv,
                        "[ 15:07\0] { 15:07\0}.T"sv));
        TextDateTest(t, "Fr 7. Jan 2011, 15:23", LANG_GER,
                     Ct("[ 7. jan 2011,] { } {7}.#.D {.} { } {jan}.@.M { } {2011}.#.Y {,}"sv,
                        "[ 15:23\0] { 15:23\0}.T"sv));
        TextDateTest(t, "Montag, 27. Dezember 2010", LANG_GER,
                     Ct("[ 27. dezember 2010\0] { } {27}.#.D {.} { } {dezember}.@.M { } {2010}.#.Y {\0}"sv));
        TextDateTest(t, "22.04.2011 um 10:34 Uhr", LANG_GER,
                     Ct("[ 22.04.2011 ] { } {22}.#.D {.} {04}.#.M {.} {2011}.#.Y { }"sv,
                        "[ 10:34 ] { 10:34 }.T"sv));
        TextDateTest(t, "31 März 2011", LANG_GER,
                     Ct("[ 31 maerz 2011\0] { } {31}.#.D { } {maerz}.@.M { } {2011}.#.Y {\0}"sv));
        TextDateTest(t, "22. Februar 2011", LANG_GER,
                     Ct("[ 22. februar 2011\0] { } {22}.#.D {.} { } {februar}.@.M { } {2011}.#.Y {\0}"sv));
        TextDateTest(t, "23 August 2010", LANG_GER,
                     Ct("[ 23 august 2010\0] { } {23}.#.D { } {august}.@.M { } {2010}.#.Y {\0}"sv));
        TextDateTest(t, "19 Februar 2007", LANG_GER,
                     Ct("[ 19 februar 2007\0] { } {19}.#.D { } {februar}.@.M { } {2007}.#.Y {\0}"sv));
        TextDateTest(t, "Feb. 2nd, 2011 at 10:50 AM", LANG_GER,
                     Ct("[ feb. 2nd, 2011 ] { } {feb}.@.M {.} { } {2}.#.D {nd} {, } {2011}.#.Y { }"sv,
                        "[ 10:50 am\0] { 10:50 am\0}.T"sv));

    }

    Y_UNIT_TEST(TestFrnTextScan) {
        using namespace ND2;
        TTestContext t;

        ////////////////////////////////////////////////////////////////
        // France
        ////////////////////////////////////////////////////////////////
        TextDateTest(t, "Mar 4 Jan - 20:28", LANG_FRE,
                     Ct("[ 4 jan - ] { } {4}.#.D { } {jan}.@.M { - }"sv,
                        "[ 20:28\0] { 20:28\0}.T"sv));
        TextDateTest(t, "Mar 4/01/10 - 20:28", LANG_FRE,
                     Ct("[ 4/01/10 ] { } {4}.#.D {/} {01}.#.M {/} {10}.#.Y { }"sv,
                        "[ 20:28\0] { 20:28\0}.T"sv));
        TextDateTest(t, "vendredi 22 octobre 2010 à 09:45", LANG_FRE,
                     Ct("[ 22 octobre 2010 ] { } {22}.#.D { } {octobre}.@.M { } {2010}.#.Y { }"sv,
                        "[ 09:45\0] { 09:45\0}.T"sv));
        TextDateTest(t, "sam, 01/01/2011 - 14:28", LANG_FRE,
                     Ct("[ 01/01/2011 ] { } {01}.#.D {/} {01}.#.M {/} {2011}.#.Y { }"sv,
                        "[ 14:28\0] { 14:28\0}.T"sv));
        TextDateTest(t, "octobre 14th, 2010", LANG_FRE,
                     Ct("[ octobre 14th, 2010\0] { } {octobre}.@.M { } {14}.#.D {th} {, } {2010}.#.Y {\0}"sv));
        TextDateTest(t, "Mercredi 27 mai 2009", LANG_FRE,
                     Ct("[ 27 mai 2009\0] { } {27}.#.D { } {mai}.@.M { } {2009}.#.Y {\0}"sv));
        TextDateTest(t, "le 21/06/2009 à 08h07", LANG_FRE,
                     Ct("[ 21/06/2009 ] { } {21}.#.D {/} {06}.#.M {/} {2009}.#.Y { }"sv,
                        "[ 08h07\0] { 08h07\0}.T"sv));
        TextDateTest(t, "mercredi 19 septembre 2007 à 00:00", LANG_FRE,
                     Ct("[ 19 septembre 2007 ] { } {19}.#.D { } {septembre}.@.M { } {2007}.#.Y { }"sv,
                        "[ 00:00\0] { 00:00\0}.T"sv));
        TextDateTest(t, "mardi 5 octobre 2010", LANG_FRE,
                     Ct("[ 5 octobre 2010\0] { } {5}.#.D { } {octobre}.@.M { } {2010}.#.Y {\0}"sv));
        TextDateTest(t, "Lundi, 01 Septembre 2008 00:00", LANG_FRE,
                     Ct("[ 01 septembre 2008 ] { } {01}.#.D { } {septembre}.@.M { } {2008}.#.Y { }"sv,
                        "[ 00:00\0] { 00:00\0}.T"sv));
        TextDateTest(t, "le mardi 22 novembre 2005", LANG_FRE,
                     Ct("[ 22 novembre 2005\0] { } {22}.#.D { } {novembre}.@.M { } {2005}.#.Y {\0}"sv));
        TextDateTest(t, "le Mar 13 Mar - 15:27", LANG_FRE,
                     Ct("[ 13 mar - ] { } {13}.#.D { } {mar}.@.M { - }"sv,
                        "[ 15:27\0] { 15:27\0}.T"sv));
        TextDateTest(t, "le 3 janvier 2005", LANG_FRE,
                     Ct("[ 3 janvier 2005\0] { } {3}.#.D { } {janvier}.@.M { } {2005}.#.Y {\0}"sv));
        TextDateTest(t, "le 07-11-2010 à 15:11:31", LANG_FRE,
                     Ct("[ 07-11-2010 ] { } {07}.#.D {-} {11}.#.M {-} {2010}.#.Y { }"sv,
                        "[ 15:11:31\0] { 15:11:31\0}.T"sv));
        TextDateTest(t, "le 06 Jan 2010 12:33", LANG_FRE,
                     Ct("[ 06 jan 2010 ] { } {06}.#.D { } {jan}.@.M { } {2010}.#.Y { }"sv,
                        "[ 12:33\0] { 12:33\0}.T"sv));
        TextDateTest(t, "le 01 février 2010 à 12:27", LANG_FRE,
                     Ct("[ 01 fevrier 2010 ] { } {01}.#.D { } {fevrier}.@.M { } {2010}.#.Y { }"sv,
                        "[ 12:27\0] { 12:27\0}.T"sv));
        TextDateTest(t, "jeudi 24 mars 2011", LANG_FRE,
                     Ct("[ 24 mars 2011\0] { } {24}.#.D { } {mars}.@.M { } {2011}.#.Y {\0}"sv));
        TextDateTest(t, "Jeu Nov 26, 2009 1:25", LANG_FRE,
                     Ct("[ nov 26, 2009 ] { } {nov}.@.M { } {26}.#.D {, } {2009}.#.Y { }"sv,
                        "[ 1:25\0] { 1:25\0}.T"sv));
        TextDateTest(t, "dimanche 16 janvier 2011", LANG_FRE,
                     Ct("[ 16 janvier 2011\0] { } {16}.#.D { } {janvier}.@.M { } {2011}.#.Y {\0}"sv));
        TextDateTest(t, "dimanche 11 mai 2008", LANG_FRE,
                     Ct("[ 11 mai 2008\0] { } {11}.#.D { } {mai}.@.M { } {2008}.#.Y {\0}"sv));
        TextDateTest(t, "Dim 20 Mai - 0:10", LANG_FRE,
                     Ct("[ 20 mai - ] { } {20}.#.D { } {mai}.@.M { - }"sv,
                        "[ 0:10\0] { 0:10\0}.T"sv));
        TextDateTest(t, "December 7, 2009", LANG_FRE,
                     Ct("[ december 7, 2009\0] { } {december}.@.M { } {7}.#.D {, } {2009}.#.Y {\0}"sv));
        TextDateTest(t, "April 5 2011", LANG_FRE,
                     Ct("[ april 5 2011\0] { } {april}.@.M { } {5}.#.D { } {2011}.#.Y {\0}"sv));
        TextDateTest(t, "Avril 7, 2011", LANG_FRE,
                     Ct("[ avril 7, 2011\0] { } {avril}.@.M { } {7}.#.D {, } {2011}.#.Y {\0}"sv));
        TextDateTest(t, "7 mars", LANG_FRE,
                     Ct("[ 7 mars\0] { } {7}.#.D { } {mars}.@.M {\0}"sv));
        TextDateTest(t, "25 Mars 2010", LANG_FRE,
                     Ct("[ 25 mars 2010\0] { } {25}.#.D { } {mars}.@.M { } {2010}.#.Y {\0}"sv));
        TextDateTest(t, "12 février 2007 - 13:46", LANG_FRE,
                     Ct("[ 12 fevrier 2007 ] { } {12}.#.D { } {fevrier}.@.M { } {2007}.#.Y { }"sv,
                        "[ 13:46\0] { 13:46\0}.T"sv));
        TextDateTest(t, "19-mai 04", LANG_FRE,
                     Ct("[ 19-mai 04\0] { } {19}.#.D {-} {mai}.@.M { } {04}.#.Y {\0}"sv));
        TextDateTest(t, "12 avr. 2011", LANG_FRE,
                     Ct("[ 12 avr. 2011\0] { } {12}.#.D { } {avr}.@.M {.} { } {2011}.#.Y {\0}"sv));
        TextDateTest(t, "12 aoû 2005 à 22:01", LANG_FRE,
                     Ct("[ 12 aou 2005 ] { } {12}.#.D { } {aou}.@.M { } {2005}.#.Y { }"sv,
                        "[ 22:01\0] { 22:01\0}.T"sv));
        TextDateTest(t, "15/04/2011 à 00h00", LANG_FRE,
                     Ct("[ 15/04/2011 ] { } {15}.#.D {/} {04}.#.M {/} {2011}.#.Y { }"sv,
                        "[ 00h00\0] { 00h00\0}.T"sv));
    }

    Y_UNIT_TEST(TestSpaTextScan) {
        using namespace ND2;
        TTestContext t;

        ////////////////////////////////////////////////////////////////
        // Spain
        ////////////////////////////////////////////////////////////////

        TextDateTest(t, "Thu Apr 7 3:09:50 2011", LANG_SPA,
                     Ct("[ apr 7 3:09:50 2011\0] { } {apr}.@.M { } {7}.#.D { } {3:09:50}.T { } {2011}.#.Y {\0}"sv));
        TextDateTest(t, "viernes 05 de septiembre de 2008", LANG_SPA,
                     Ct("[ 05 de septiembre de 2008\0] { } {05}.#.D { } {de } {septiembre}.@.M { de } {2008}.#.Y {\0}"sv));
        TextDateTest(t, "septiembre 22nd, 2008", LANG_SPA,
                     Ct("[ septiembre 22nd, 2008\0] { } {septiembre}.@.M { } {22}.#.D {nd} {, } {2008}.#.Y {\0}"sv));
        TextDateTest(t, "sep 23, 2008 at 1:21", LANG_SPA,
                     Ct("[ sep 23, 2008 ] { } {sep}.@.M { } {23}.#.D {, } {2008}.#.Y { }"sv,
                        "[ 1:21\0] { 1:21\0}.T"sv));
        TextDateTest(t, "sep 20", LANG_SPA,
                     Ct("[ sep 20\0] { } {sep}.@.M { } {20}.#.D {\0}"sv));
        TextDateTest(t, "15 abril 2010", LANG_SPA,
                     Ct("[ 15 abril 2010\0] { } {15}.#.D { } {abril}.@.M { } {2010}.#.Y {\0}"sv));
        TextDateTest(t, "octubre 30th, 2009", LANG_SPA,
                     Ct("[ octubre 30th, 2009\0] { } {octubre}.@.M { } {30}.#.D {th} {, } {2009}.#.Y {\0}"sv));
        TextDateTest(t, "Miércoles, 20 de Octubre de 2010 15:17", LANG_SPA,
                     Ct("[ 20 de octubre de 2010 ] { } {20}.#.D { } {de } {octubre}.@.M { de } {2010}.#.Y { }"sv,
                        "[ 15:17\0] { 15:17\0}.T"sv));
        TextDateTest(t, "miércoles 27 de mayo de 2009", LANG_SPA,
                     Ct("[ 27 de mayo de 2009\0] { } {27}.#.D { } {de } {mayo}.@.M { de } {2009}.#.Y {\0}"sv));
        TextDateTest(t, "Miercoles 16 de Febrero de 2011", LANG_SPA,
                     Ct("[ 16 de febrero de 2011\0] { } {16}.#.D { } {de } {febrero}.@.M { de } {2011}.#.Y {\0}"sv));
        TextDateTest(t, "miércoles 05 diciembre de 2007, 21:43", LANG_SPA,
                     Ct("[ 05 diciembre de 2007,] { } {05}.#.D { } {diciembre}.@.M { de } {2007}.#.Y {,}"sv,
                        "[ 21:43\0] { 21:43\0}.T"sv));
        TextDateTest(t, "07 octubre 2005", LANG_SPA,
                     Ct("[ 07 octubre 2005\0] { } {07}.#.D { } {octubre}.@.M { } {2005}.#.Y {\0}"sv));
        TextDateTest(t, "martes 17 de noviembre de 2009", LANG_SPA,
                     Ct("[ 17 de noviembre de 2009\0] { } {17}.#.D { } {de } {noviembre}.@.M { de } {2009}.#.Y {\0}"sv));
        TextDateTest(t, "March 28th, 2011", LANG_SPA,
                     Ct("[ march 28th, 2011\0] { } {march}.@.M { } {28}.#.D {th} {, } {2011}.#.Y {\0}"sv));
        TextDateTest(t, "Lunes 07 de Junio 21:23", LANG_SPA,
                     Ct("[ 07 de junio ] { } {07}.#.D { } {de } {junio}.@.M { }"sv,
                        "[ 21:23\0] { 21:23\0}.T"sv));
        TextDateTest(t, "Jueves, Abril 07, 2011", LANG_SPA,
                     Ct("[ abril 07, 2011\0] { } {abril}.@.M { } {07}.#.D {, } {2011}.#.Y {\0}"sv));
        TextDateTest(t, "Jueves, 03 de Abril de 2008 06:45", LANG_SPA,
                     Ct("[ 03 de abril de 2008 ] { } {03}.#.D { } {de } {abril}.@.M { de } {2008}.#.Y { }"sv,
                        "[ 06:45\0] { 06:45\0}.T"sv));
        TextDateTest(t, "jueves 24 mayo de 2007 a las 08:03.", LANG_SPA,
                     Ct("[ 24 mayo de 2007 ] { } {24}.#.D { } {mayo}.@.M { de } {2007}.#.Y { }"sv,
                        "[ 08:03.] { 08:03.}.T"sv));
        TextDateTest(t, "03 marzo 2007", LANG_SPA,
                     Ct("[ 03 marzo 2007\0] { } {03}.#.D { } {marzo}.@.M { } {2007}.#.Y {\0}"sv));
        TextDateTest(t, "el 19 Ene, 2010", LANG_SPA,
                     Ct("[ 19 ene, 2010\0] { } {19}.#.D { } {ene}.@.M {, } {2010}.#.Y {\0}"sv));
        TextDateTest(t, "el 18 jun, 2009", LANG_SPA,
                     Ct("[ 18 jun, 2009\0] { } {18}.#.D { } {jun}.@.M {, } {2009}.#.Y {\0}"sv));
        TextDateTest(t, "el 12 de marzo de 2009", LANG_SPA,
                     Ct("[ 12 de marzo de 2009\0] { } {12}.#.D { } {de } {marzo}.@.M { de } {2009}.#.Y {\0}"sv));
        TextDateTest(t, "Apr 6 12:29:34 2011", LANG_SPA,
                     Ct("[ apr 6 12:29:34 2011\0] { } {apr}.@.M { } {6}.#.D { } {12:29:34}.T { } {2011}.#.Y {\0}"sv));
        TextDateTest(t, "agosto 17, 2010", LANG_SPA,
                     Ct("[ agosto 17, 2010\0] { } {agosto}.@.M { } {17}.#.D {, } {2010}.#.Y {\0}"sv));
        TextDateTest(t, "agosto 2010", LANG_SPA,
                     Ct("[ agosto 2010\0] { } {agosto}.@.M { } {2010}.#.Y {\0}"sv));
        TextDateTest(t, "abril 8th, 2010", LANG_SPA,
                     Ct("[ abril 8th, 2010\0] { } {abril}.@.M { } {8}.#.D {th} {, } {2010}.#.Y {\0}"sv));
        TextDateTest(t, "7 abril, 2011", LANG_SPA,
                     Ct("[ 7 abril, 2011\0] { } {7}.#.D { } {abril}.@.M {, } {2011}.#.Y {\0}"sv));
        TextDateTest(t, "30 gen. 2011", LANG_CAT,
                     Ct("[ 30 gen. 2011\0] { } {30}.#.D { } {gen}.@.M {.} { } {2011}.#.Y {\0}"sv));
        TextDateTest(t, "30 de gener 2011", LANG_CAT,
                     Ct("[ 30 de gener 2011\0] { } {30}.#.D { } {de } {gener}.@.M { } {2011}.#.Y {\0}"sv));
        TextDateTest(t, "22 Març, 2007 21:26", LANG_CAT,
                     Ct("[ 22 marc, 2007 ] { } {22}.#.D { } {marc}.@.M {, } {2007}.#.Y { }"sv,
                        "[ 21:26\0] { 21:26\0}.T"sv));
        TextDateTest(t, "23 oct ,2009", LANG_SPA,
                     Ct("[ 23 oct ,2009\0] { } {23}.#.D { } {oct}.@.M { ,} {2009}.#.Y {\0}"sv));
        TextDateTest(t, "22 de marzo de 2007", LANG_SPA,
                     Ct("[ 22 de marzo de 2007\0] { } {22}.#.D { } {de } {marzo}.@.M { de } {2007}.#.Y {\0}"sv));
        TextDateTest(t, "19 Oct 2010", LANG_SPA,
                     Ct("[ 19 oct 2010\0] { } {19}.#.D { } {oct}.@.M { } {2010}.#.Y {\0}"sv));
        TextDateTest(t, "19 noviembre 2009", LANG_SPA,
                     Ct("[ 19 noviembre 2009\0] { } {19}.#.D { } {noviembre}.@.M { } {2009}.#.Y {\0}"sv));
        TextDateTest(t, "jueves, 07 abril 2011", LANG_SPA,
                     Ct("[ 07 abril 2011\0] { } {07}.#.D { } {abril}.@.M { } {2011}.#.Y {\0}"sv));
        TextDateTest(t, "16-Apr-2008", LANG_SPA,
                     Ct("[ 16-apr-2008\0] { } {16}.#.D {-} {apr}.@.M {-} {2008}.#.Y {\0}"sv));
        TextDateTest(t, "08 jul 2010 | 17:34 MSK", LANG_SPA,
                     Ct("[ 08 jul 2010 ] { } {08}.#.D { } {jul}.@.M { } {2010}.#.Y { }"sv,
                        "[ 17:34 ] { 17:34 }.T"sv));
        TextDateTest(t, "15 Junio de 2010 | 01:39", LANG_SPA,
                     Ct("[ 15 junio de 2010 ] { } {15}.#.D { } {junio}.@.M { de } {2010}.#.Y { }"sv,
                        "[ 01:39\0] { 01:39\0}.T"sv));
        TextDateTest(t, "8/27/2003 12:14", LANG_SPA,
                     Ct("[ 8/27/2003 ] { } {8}.#.M {/} {27}.#.D {/} {2003}.#.Y { }"sv,
                        "[ 12:14\0] { 12:14\0}.T"sv));
        TextDateTest(t, "Jueves 07 de Abril de 2011", LANG_SPA,
                     Ct("[ 07 de abril de 2011\0] { } {07}.#.D { } {de } {abril}.@.M { de } {2011}.#.Y {\0}"sv));
    }

    Y_UNIT_TEST(TestItaTextScan) {
        using namespace ND2;
        TTestContext t;

        ////////////////////////////////////////////////////////////////
        // Italy
        ////////////////////////////////////////////////////////////////

        TextDateTest(t, "January 10, 2011, 08:29:44 AM", LANG_ITA,
                     Ct("[ january 10, 2011,] { } {january}.@.M { } {10}.#.D {, } {2011}.#.Y {,}"sv,
                        "[ 08:29:44 am\0] { 08:29:44 am\0}.T"sv));
        TextDateTest(t, "Giovedì 07 Ottobre 2010 10:05", LANG_ITA,
                     Ct("[ 07 ottobre 2010 ] { } {07}.#.D { } {ottobre}.@.M { } {2010}.#.Y { }"sv,
                        "[ 10:05\0] { 10:05\0}.T"sv));
        TextDateTest(t, "9 Novembre 2009", LANG_ITA,
                     Ct("[ 9 novembre 2009\0] { } {9}.#.D { } {novembre}.@.M { } {2009}.#.Y {\0}"sv));
        TextDateTest(t, "27 Luglio 2010", LANG_ITA,
                     Ct("[ 27 luglio 2010\0] { } {27}.#.D { } {luglio}.@.M { } {2010}.#.Y {\0}"sv));
        TextDateTest(t, "26 Gennaio 2011", LANG_ITA,
                     Ct("[ 26 gennaio 2011\0] { } {26}.#.D { } {gennaio}.@.M { } {2011}.#.Y {\0}"sv));
        TextDateTest(t, "23 dicembre 2010", LANG_ITA,
                     Ct("[ 23 dicembre 2010\0] { } {23}.#.D { } {dicembre}.@.M { } {2010}.#.Y {\0}"sv));
        TextDateTest(t, "22/02/2008 - 19.13", LANG_ITA,
                     Ct("[ 22/02/2008 ] { } {22}.#.D {/} {02}.#.M {/} {2008}.#.Y { }"sv,
                        "[ 19.13\0] { 19.13\0}.T"sv));
        TextDateTest(t, "22 settembre 2007", LANG_ITA,
                     Ct("[ 22 settembre 2007\0] { } {22}.#.D { } {settembre}.@.M { } {2007}.#.Y {\0}"sv));
        TextDateTest(t, "22 November 2004", LANG_ITA,
                     Ct("[ 22 november 2004\0] { } {22}.#.D { } {november}.@.M { } {2004}.#.Y {\0}"sv));
        TextDateTest(t, "22 nov 2009", LANG_ITA,
                     Ct("[ 22 nov 2009\0] { } {22}.#.D { } {nov}.@.M { } {2009}.#.Y {\0}"sv));
        TextDateTest(t, "21 settembre 2010 ore 14.18", LANG_ITA,
                     Ct("[ 21 settembre 2010 ] { } {21}.#.D { } {settembre}.@.M { } {2010}.#.Y { }"sv,
                        "[ 14.18\0] { 14.18\0}.T"sv));
        TextDateTest(t, "18 Luglio '09", LANG_ITA,
                     Ct("[ 18 luglio '09\0] { } {18}.#.D { } {luglio}.@.M { '} {09}.#.Y {\0}"sv));
        TextDateTest(t, "15 marzo 2005", LANG_ITA,
                     Ct("[ 15 marzo 2005\0] { } {15}.#.D { } {marzo}.@.M { } {2005}.#.Y {\0}"sv));
        TextDateTest(t, "1 febbraio 2009", LANG_ITA,
                     Ct("[ 1 febbraio 2009\0] { } {1}.#.D { } {febbraio}.@.M { } {2009}.#.Y {\0}"sv));
        TextDateTest(t, "﻿10/09/2010, 10:50", LANG_ITA,
                     Ct("[ 10/09/2010,] { } {10}.#.D {/} {09}.#.M {/} {2010}.#.Y {,}"sv,
                        "[ 10:50\0] { 10:50\0}.T"sv));
        TextDateTest(t, "3 Gen '11", LANG_ITA,
                     Ct("[ 3 gen '11\0] { } {3}.#.D { } {gen}.@.M { '} {11}.#.Y {\0}"sv));
        TextDateTest(t, "19th-Apr-2011 03:28 pm", LANG_ITA,
                     Ct("[ 19th-apr-2011 ] { } {19}.#.D {th} {-} {apr}.@.M {-} {2011}.#.Y { }"sv,
                        "[ 03:28 pm\0] { 03:28 pm\0}.T"sv));

    }

    Y_UNIT_TEST(TestRemoveDates) {
        using namespace ND2;
        TDater dater(false, true);
        TString userRequest = "фердинанд мультфильм 2017";
        SetBaseline(dater, userRequest);
    }
}
