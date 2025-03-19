#include <library/cpp/testing/unittest/registar.h>

#include <util/datetime/base.h>
#include <library/cpp/digest/md5/md5.h>
#include <library/cpp/digest/old_crc/crc.h>

#include <kernel/indexer/directindex/directindex.h>
#include <kernel/indexer/disamber/disamber.h>
#include <kernel/indexer/posindex/invcreator.h>
#include <ysite/directtext/textarchive/createarc.h>
#include <kernel/tarc/iface/tarcio.h>
#include <kernel/walrus/advmerger.h>
#include <kernel/keyinv/indexfile/indexstoragefactory.h>
#include <ysite/directtext/freqs/freqs.h>

//#ifdef UNIT_ASSERT_STRINGS_EQUAL
//#undef UNIT_ASSERT_STRINGS_EQUAL
//#endif
//#define UNIT_ASSERT_STRINGS_EQUAL(a, b) Cout << "    UNIT_ASSERT_STRINGS_EQUAL(\"" << b << "\", " << #b << ");" << Endl;

class TDirectIndexDisambTest2: public TTestBase {
    UNIT_TEST_SUITE(TDirectIndexDisambTest2);
        UNIT_TEST(Test);
    UNIT_TEST_SUITE_END();
public:
    void Test();
private:
    void StoreText(NIndexerCore::TDirectIndex& directIndex, const TUtf16String& text, RelevLevel relev) {
        directIndex.StoreText(text.c_str(), text.size(), relev);
    }
    void StoreText(NIndexerCore::TDirectIndex& directIndex, const TUtf16String& text, RelevLevel relev, const NIndexerCore::TDirectIndex::TAttribute& sentAttr) {
        directIndex.StoreText(text.c_str(), text.size(), relev, sentAttr);
    }

};

void TDirectIndexDisambTest2::Test() {
    NIndexerCore::TIndexStorageFactory storage;
    storage.InitIndexResources("indexO", "./", "indexO-");
    {
        NIndexerCore::TDTCreatorConfig dtcCfg;
        NIndexerCore::TDirectTextCreator dtc(dtcCfg, TLangMask(LANG_RUS, LANG_ENG), LANG_RUS);
        NIndexerCore::TInvCreatorConfig invCfg(50);
        NIndexerCore::TInvCreatorDTCallback ic(&storage, invCfg);
        TFreqCalculator freqCalculator(false);
        NIndexerCore::TDirectIndexWithArchive dIndex(dtc, "indexarc");
        NIndexerCore::TDefaultDisamber disamb;
        dIndex.SetInvCreator(&ic);
        dIndex.SetDisamber(&disamb);
        dIndex.AddDirectTextCallback(&freqCalculator);

        dIndex.AddDoc(0, LANG_RUS);

        dIndex.StoreDocAttr(NIndexerCore::TDirectIndex::TWAttribute("author", u"Мищенко"));
        dIndex.StoreDocAttr(NIndexerCore::TDirectIndex::TAttribute("url", "www.yandex.ru/test/"));

        time_t date;
        ParseRFC822DateTimeDeprecated("Fri, 4 Mar 2005 19:34:45 UT", date);
        dIndex.StoreDocDateTimeAttr("date", date);

        dIndex.OpenZone("title");
        StoreText(dIndex, u"Заголовок!", BEST_RELEV);
        dIndex.CloseZone("title");

        /*
        Внутренний токенизатор рвет предложения на последовательности [termpunct.space.ytitle],
        которая недостижима в конце строки - аргумента StoreText().
        Однако парсер отправляет токенизироваться последовательность
        между тегами, рвущими параграф, и сам добавляет конец предложения после.
        Чтобы имитировать поведение парсера, TDirectIndex в конце текста генерирует конец предложения.
        */
        StoreText(dIndex, u"Оно дело дело шахтёра сегодня днём. В день шахтёра было ясно.", MID_RELEV);

        UNIT_ASSERT_VALUES_EQUAL(dIndex.GetWordCount(), (ui32)12); // части мультитокена считаются отдельно

        UNIT_ASSERT_VALUES_EQUAL(dIndex.GetWordCount(), ui32(12)); // части мультитокена считаются отдельно

        TFullArchiveDocHeader docHeader;
        TDocInfoEx docInfo;
        TFullDocAttrs docAttrs;
        docInfo.DocHeader = &docHeader;
        strcpy(docHeader.Url, "www.yandex.ru/test/");
        docInfo.ModTime = date;
        docHeader.Encoding = CODES_WIN;
        docHeader.MimeType = MIME_HTML;

        docAttrs.AddAttr("info", "Тестовый пример снятия омонимии", TFullDocAttrs::AttrArcText); // UTF8 text

        dIndex.CommitDoc(&docInfo, &docAttrs);
    } // Тут dIndex разрушается и происходит flush записанного им архива

    MergePortions(storage, YNDEX_VERSION_CURRENT);

    // printkeys.exe -w -o o.txt indexkey indexinv
    // tarcview.exe -e -w -m -o a.txt index

    char buf[50];
    UNIT_ASSERT_STRINGS_EQUAL("7a87cebc2850e7fc6cc1eca5fe193e7d", MD5::File("indexOkey", buf));
    UNIT_ASSERT_STRINGS_EQUAL("5fe1caca7feabb408f08c5fc177b3788", MD5::File("indexOinv", buf));

    UNIT_ASSERT(unlink("indexOkey") == 0);
    UNIT_ASSERT(unlink("indexOinv") == 0);
}

UNIT_TEST_SUITE_REGISTRATION(TDirectIndexDisambTest2);

/*
$ bin/printkeys -cfz indexkey indexinv | iconv -f cp1251 -t utf-8
##_DOC_IDF_SUM 1 4
    [0.2041.28.3.6]
##_DOC_LENS 1 2
    [0.0.1.3.9]
##_HEADING_IDF_SUM 1 2
    [0.0.0.0.0]
##_HEADING_LENS 1 2
    [0.0.0.0.0]
##_LOW_TEXT_IDF_SUM 1 2
    [0.0.0.0.0]
##_LOW_TEXT_LENS 1 2
    [0.0.0.0.0]
##_MAX_FREQ 1 2
    [0.0.0.0.6]
##_NORMAL_TEXT_IDF_SUM 1 4
    [0.2049.9.2.13]
##_NORMAL_TEXT_LENS 1 2
    [0.0.1.3.8]
##_TITLE_IDF_SUM 1 4
    [0.1117.22.0.9]
##_TITLE_LENS 1 2
    [0.0.0.0.1]
#author="мищенко 1 2
    [0.0.0.0.0]
#date="20050304 1 4
    [0.17.13.1.5]
#replier="druxa 1 3
    [0.4.6.0.0]
#replier="veged 1 3
    [0.2.10.0.0]
#url="www.yandex.ru/test/ 1 2
    [0.0.0.0.0]
(replies 1 3
    [0.2.10.0.0]
(reply 2 4
    [0.2.10.0.0]
    [0.4.6.0.0]
(title 1 3
    [0.1.1.0.0]
)replies 1 3
    [0.5.8.3.0]
)reply 2 4
    [0.3.4.3.0]
    [0.5.8.3.0]
)title 1 3
    [0.1.2.3.0]
00000000001 (1) 2 4
    [0.8.17.1.0]
    [0.8.27.1.0]
00000000002 (2 L, 2 L+, 2 R+, 2 LR+) 6 14
    [0.6.5.1.2]
    [0.6.6.1.1]
    [0.6.11.1.0]
    [0.6.21.1.3]
    [0.6.24.1.1]
    [0.6.25.1.2]
00000000004 (4, 4 R+) 2 6
    [0.8.16.1.0]
    [0.8.25.1.1]
00000000007 (07 L+R+) 1 3
    [0.6.17.1.0]
00000000010 (10 L+) 1 3
    [0.8.26.1.0]
00000000014 (14 L+R+) 1 3
    [0.6.18.1.0]
00000000016 (16 L, 16 R+) 2 5
    [0.8.34.1.1]
    [0.8.45.1.0]
00000000044 (44) 1 3
    [0.8.7.1.0]
00000000047 (47, 47 R) 2 5
    [0.7.2.1.1]
    [0.7.6.1.0]
00000000075 (75 L+R+) 1 3
    [0.8.43.1.0]
00000000241 (241 LR) 1 3
    [0.8.21.1.0]
00000000245 (245 LR+) 1 3
    [0.8.42.1.0]
00000001080 (1080 R) 1 3
    [0.7.9.1.0]
00000002003 (2003) 1 3
    [0.7.29.1.0]
00000002010 (2010 L+R+) 1 3
    [0.6.16.1.0]
00000003704 (3704 LR) 1 3
    [0.7.4.1.0]
actuate {en} (actuated L+) 1 3
    [0.8.13.1.0]
air {en} (air R+) 1 3
    [0.8.12.1.0]
alloy {en} (alloy) 1 3
    [0.8.36.1.0]
and {en} (and, and L+R+) 4 12
    [0.6.3.1.0]
    [0.6.27.1.1]
    [0.8.4.1.0]
    [0.8.38.1.0]
android {en} (Android, android L+R+) 2 6
    [0.6.4.1.0]
    [0.6.23.1.1]
axle {en} (axles) 1 3
    [0.8.8.1.0]
be {en} (was) 1 3
    [0.7.26.1.0]
build {en} (built R+) 1 3
    [0.8.10.1.0]
built-in {en} (built-in) 1 3
    [0.8.10.1.0]
case {en} (case) 1 3
    [0.8.24.1.0]
com {en} (com L+R+) 1 3
    [0.6.15.1.0]
d {en} (D L) 1 3
    [0.7.5.1.0]
dana {en} (Dana) 1 3
    [0.8.6.1.0]
diamond {en} (diamond) 1 3
    [0.8.30.1.0]
differential {en} (differential, differentials) 2 5
    [0.8.15.1.1]
    [0.8.28.1.0]
fame {en} (famed) 1 3
    [0.7.18.1.0]
famed {en} (famed) 1 3
    [0.7.18.1.0]
feature {en} (featured) 1 3
    [0.8.2.1.0]
for {en} (for) 1 3
    [0.7.16.1.0]
front {en} (front) 1 3
    [0.8.3.1.0]
froyo {en} (Froyo, froyo L+R+) 2 6
    [0.6.7.1.0]
    [0.6.26.1.1]
fullhd {en} (fullhd) 1 3
    [0.7.11.1.0]
gear {en} (gears) 1 3
    [0.8.29.1.0]
get {en} (gets L+) 1 3
    [0.6.29.1.0]
goodyear {en} (Goodyear) 1 3
    [0.8.39.1.0]
hd {en} (hd R, hd L+R) 2 6
    [0.6.10.1.0]
    [0.6.20.1.1]
htc {en} (htc, htc L+R+) 2 6
    [0.6.1.1.0]
    [0.6.19.1.1]
http {en} (http) 1 3
    [0.6.12.1.0]
in {en} (in, in L+) 3 7
    [0.7.21.1.0]
    [0.7.28.1.0]
    [0.8.11.1.1]
inch {en} (inch L+) 1 3
    [0.8.35.1.0]
intomobile {en} (intomobile L+R+) 1 3
    [0.6.14.1.0]
introduce {en} (introduced) 1 3
    [0.7.27.1.0]
it {en} (It) 1 3
    [0.8.1.1.0]
lcd {en} (lcd) 1 3
    [0.7.7.1.0]
lock {en} (locking) 1 3
    [0.8.14.1.0]
locking {en} (locking) 1 3
    [0.8.14.1.0]
love {en} (loved L+) 1 3
    [0.6.32.1.0]
low {en} (low R+) 1 3
    [0.8.18.1.0]
mountain {en} (Mountains) 1 3
    [0.7.25.1.0]
mtr {en} (mtr) 1 3
    [0.8.40.1.0]
name {en} (named) 1 3
    [0.7.15.1.0]
named {en} (named) 1 3
    [0.7.15.1.0]
nevada {en} (Nevada) 1 3
    [0.7.24.1.0]
nv {en} (nv R) 1 3
    [0.8.20.1.0]
off {en} (off L+R+) 1 3
    [0.6.31.1.0]
on {en} (on) 1 3
    [0.6.9.1.0]
or {en} (or L) 1 3
    [0.8.22.1.0]
p {en} (p L, P R) 2 6
    [0.7.10.1.0]
    [0.8.41.1.1]
panel {en} (panels) 1 3
    [0.8.33.1.0]
pfl {en} (pfl LR) 1 3
    [0.7.3.1.0]
philip {en} (Philips) 1 3
    [0.7.1.1.0]
philips {en} (Philips) 1 3
    [0.7.1.1.0]
plate {en} (plate) 1 3
    [0.8.31.1.0]
r {en} (R L+R) 1 3
    [0.8.44.1.0]
range {en} (range L+) 1 3
    [0.8.19.1.0]
rear {en} (rear) 1 3
    [0.8.5.1.0]
rocker {en} (rocker) 1 3
    [0.8.32.1.0]
rubicon {en} (Rubicon) 2 4
    [0.7.14.1.0]
    [0.7.19.1.0]
run {en} (running, running L+R+) 2 6
    [0.6.8.1.0]
    [0.6.22.1.1]
running {en} (running, running L+R+) 2 6
    [0.6.8.1.0]
    [0.6.22.1.1]
sense {en} (Sense, sense L+R+) 2 6
    [0.6.2.1.0]
    [0.6.28.1.1]
show {en} (show R+) 1 3
    [0.6.30.1.0]
show-off {en} (show-off R+) 1 3
    [0.6.30.1.0]
sierra {en} (Sierra) 1 3
    [0.7.23.1.0]
the {en} (the, The) 3 6
    [0.7.12.1.1]
    [0.7.17.1.0]
    [0.7.22.1.0]
tire {en} (tires) 1 3
    [0.8.46.1.0]
trail {en} (Trail) 1 3
    [0.7.20.1.0]
transfer {en} (transfer) 1 3
    [0.8.23.1.0]
tv {en} (tv) 1 3
    [0.7.8.1.0]
wheel {en} (wheels) 1 3
    [0.8.37.1.0]
with {en} (with) 1 3
    [0.8.9.1.0]
wrangler {en} (Wrangler) 1 3
    [0.7.13.1.0]
www {en} (www R+) 1 3
    [0.6.13.1.0]
авторитет {ru} (авторитетов) 1 3
    [0.2.9.1.0]
ассоциировать {ru} (ассоциирует) 1 3
    [0.2.4.1.0]
внешний {ru} (внешнего) 1 3
    [0.5.6.1.0]
восприятие {ru} (восприятия) 1 3
    [0.2.2.1.0]
выродить {ru} (вырождена) 1 3
    [0.3.3.1.0]
еще {ru} (еще) 1 3
    [0.4.2.1.0]
заголовок {ru} (Заголовок) 1 3
    [0.1.1.3.0]
закон {ru} (закон) 1 3
    [0.5.5.1.0]
информация {ru} (Информация) 1 3
    [0.5.1.1.0]
какой {ru} (какой R+) 1 3
    [0.4.3.1.0]
какой-то {ru} (какой-то) 1 3
    [0.4.3.1.0]
категорически {ru} (категорически) 1 3
    [0.5.2.1.0]
категорический {ru} (категорически) 1 3
    [0.5.2.1.0]
конфликт {ru} (конфликт) 1 3
    [0.2.5.1.0]
мир {ru} (мира) 1 3
    [0.5.7.1.0]
мира {ru} (мира) 1 3
    [0.5.7.1.0]
миро {ru} (мира) 1 3
    [0.5.7.1.0]
мнение {ru} (мнение) 1 3
    [0.2.8.1.0]
на {ru} (на) 1 3
    [0.2.7.1.0]
несмотря {ru} (несмотря) 1 3
    [0.2.6.1.0]
онтологический {ru} (онтологический) 1 3
    [0.5.4.1.0]
принцип {ru} (Принцип) 1 3
    [0.2.1.1.0]
ситуация {ru} (ситуация) 1 3
    [0.3.2.1.0]
современный {ru} (Современная) 1 3
    [0.3.1.1.0]
текст {ru} (текст) 1 3
    [0.4.5.1.0]
то {ru} (то L+) 1 3
    [0.4.4.1.0]
тот {ru} (то L+) 1 3
    [0.4.4.1.0]
транспонировать {ru} (транспонирует) 1 3
    [0.5.3.1.0]
тут {ru} (Тут) 1 3
    [0.4.1.1.0]
тута {ru} (Тут) 1 3
    [0.4.1.1.0]
философски {ru} (философски) 1 3
    [0.2.3.1.0]
философский {ru} (философски) 1 3
    [0.2.3.1.0]

$ bin/tarcview -aemzwxs index

~~~~~~ Document 0 ~~~~~~

SIZE=0
TIME=Fri Mar  4 22:34:45 2005
URL=www.yandex.ru/test/
MIMETYPE=text/html
CHARSET=windows-1251
info=Тестовый пример Андрея Мищенко из ARC-146

<p>
<title><w3>Заголовок</title></w3>!
Принцип восприятия философски ассоциирует конфликт, несмотря на мнение авторитетов.
Современная ситуация вырождена.    replier    veged
Тут еще какой-то текст.
Информация категорически транспонирует онтологический закон внешнего мира.    replier    druxa
HTC Sense and Android 2.2 Froyo running on HD2: http://www.intomobile.com/2010/07/14/htc-hd2-running-android-2-2-froyo-and-sense-gets-show-off-loved/.
Philips - 47PFL3704D - 47" LCD TV - 1080p (FullHD).The Wrangler Rubicon (named for the famed Rubicon Trail in the Sierra Nevada Mountains) was introduced in 2003.
It featured front and rear Dana 44 axles with built-in air-actuated locking differentials, 4:1 low-range NV241OR transfer case, 4.10:1 differential gears, diamond plate rocker panels, 16-inch alloy wheels, and Goodyear MTR P245/75-R16 tires.

*/

