#include <library/cpp/deprecated/solartrie/triebuilder_private.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/buffer.h>
#include <util/string/cast.h>
#include <util/string/util.h>

class TSolarTrieTest: public TTestBase {
    UNIT_TEST_SUITE(TSolarTrieTest);
    UNIT_TEST(TestTrie1)
    UNIT_TEST(TestTrie2)
    UNIT_TEST(TestTrie3)
    UNIT_TEST(TestTrie4)
    UNIT_TEST(TestTrie5)
    UNIT_TEST(TestTrie6)
    UNIT_TEST(TestTrie7)
    UNIT_TEST(TestTrie8)
    UNIT_TEST(TestTrie9)
    UNIT_TEST(TestTrie10)
    UNIT_TEST(TestTrie11)
    UNIT_TEST(TestTrie12)
    UNIT_TEST(TestTrie13)
    UNIT_TEST(TestTrie14)
    UNIT_TEST(TestTrie15)
    UNIT_TEST(TestTrie16)
    UNIT_TEST(TestTrie17)
    UNIT_TEST(TestTrie18)

    UNIT_TEST_SUITE_END();

private:
    static const char LTmpl[];

    void TestAdd(ui32 val, NSolarTrie::TSolarTrieBuilder::TImpl& trie, TString lbl) {
        TestAdd(ToString(val), trie, lbl);
    }

    void TestAdd(TStringBuf str, NSolarTrie::TSolarTrieBuilder::TImpl& trie, TString lbl) {
        lbl += " -> " + ToString(str);

        trie.Add(str, str);
    }

    template <typename TTrie>
    void TestGet(ui32 val, const TTrie& trie, TString lbl, bool shouldhave) {
        TestGet(ToString(val), trie, lbl, shouldhave);
    }

    template <typename TTrie>
    void TestGet(TStringBuf str, const TTrie& trie, TString lbl, bool shouldhave) {
        TBuffer v;

        if (shouldhave) {
            UNIT_ASSERT_C(trie.Get(str, v), (lbl + " KEY: " + str).data());
            UNIT_ASSERT_VALUES_EQUAL_C(TStringBuf(v.Data(), v.Size()), str, lbl.data());
        } else {
            UNIT_ASSERT_C(!trie.Get(str, v), (lbl + " KEY: " + str).data());
        }
    }

    void DoTestTrieBuild(NSolarTrie::TSolarTrieConf c, TString lbl) {
        using namespace NSolarTrie;

        TSolarTrieBuilder::TImpl builder(c);

        // Root

        UNIT_ASSERT_VALUES_EQUAL_C(builder.GetForks().size(), 1u, lbl.data());
        UNIT_ASSERT_VALUES_EQUAL_C(builder.GetBuckets().size(), 0u, lbl.data());

        // Root
        // ...B

        try {
            TestAdd(0, builder, Sprintf(LTmpl, __LINE__, lbl.data()));
            TestAdd(1, builder, Sprintf(LTmpl, __LINE__, lbl.data()));
            TestAdd(11, builder, Sprintf(LTmpl, __LINE__, lbl.data()));
            TestAdd(110, builder, Sprintf(LTmpl, __LINE__, lbl.data()));

            for (ui32 i = c.BucketMaxSize - 1; i >= 4; --i)
                TestAdd(1100 + i, builder, Sprintf(LTmpl, __LINE__, lbl.data()));

            // Root
            // ...B...F
            // .......F
            // .......B

            TestAdd(1100 + c.BucketMaxSize, builder, Sprintf(LTmpl, __LINE__, lbl.data()));

            // Root
            // ...B...F...B
            // .......F
            // .......B

            TestAdd(5, builder, Sprintf(LTmpl, __LINE__, lbl.data()));
            TestAdd(4, builder, Sprintf(LTmpl, __LINE__, lbl.data()));
            TestAdd(41, builder, Sprintf(LTmpl, __LINE__, lbl.data()));
            TestAdd(410, builder, Sprintf(LTmpl, __LINE__, lbl.data()));

            TestAdd("", builder, Sprintf(LTmpl, __LINE__, lbl.data()));

            for (ui32 i = c.BucketMaxSize - 1; i >= 4; --i) {
                TestAdd(4100 + i, builder, Sprintf(LTmpl, __LINE__, lbl.data()));
            }

            // Root
            // ...B...F...F...B
            // .......F...F
            // .......B...B
            TestAdd(4100 + c.BucketMaxSize, builder, Sprintf(LTmpl, __LINE__, lbl.data()));
            TestAdd(1100 + c.BucketMaxSize + 1, builder, Sprintf(LTmpl, __LINE__, lbl.data()));
            TestAdd(4100 + c.BucketMaxSize + 1, builder, Sprintf(LTmpl, __LINE__, lbl.data()));

            builder.Finalize();

        } catch (const yexception& e) {
            builder.Dump(Clog);
            throw;
        }

        TBlob b = builder.Compact();

        TSolarTrie::TImpl trie;
        trie.Wrap(b);

        try {
            TestGet("", trie, Sprintf(LTmpl, __LINE__, lbl.data()), true);
            TestGet((ui32)0, trie, Sprintf(LTmpl, __LINE__, lbl.data()), true);
            TestGet(1, trie, Sprintf(LTmpl, __LINE__, lbl.data()), true);
            TestGet(11, trie, Sprintf(LTmpl, __LINE__, lbl.data()), true);
            TestGet(110, trie, Sprintf(LTmpl, __LINE__, lbl.data()), true);

            TestGet(9, trie, Sprintf(LTmpl, __LINE__, lbl.data()), false);

            for (ui32 i = 4; i <= c.BucketMaxSize; ++i)
                TestGet(1100 + i, trie, Sprintf(LTmpl, __LINE__, lbl.data()), true);

            TestGet(5, trie, Sprintf(LTmpl, __LINE__, lbl.data()), true);
            TestGet(4, trie, Sprintf(LTmpl, __LINE__, lbl.data()), true);
            TestGet(41, trie, Sprintf(LTmpl, __LINE__, lbl.data()), true);
            TestGet(410, trie, Sprintf(LTmpl, __LINE__, lbl.data()), true);

            for (ui32 i = 4; i <= c.BucketMaxSize; ++i)
                TestGet(4100 + i, trie, Sprintf(LTmpl, __LINE__, lbl.data()) + " -> " + ToString(4100 + i), true);

        } catch (const yexception& e) {
            builder.Dump(Clog);
            trie.Dump(Clog);
            throw;
        }
    }

    void CheckTrie(const char** values, size_t sz, size_t asz, NSolarTrie::TSolarTrieConf c, TString lbl) {
        using namespace NSolarTrie;
        c.ResetCodecs();
        TSolarTrieBuilder::TImpl builder(c);

        try {
            for (ui32 i = 0; i < asz; ++i) {
                TestAdd(values[i], builder, Sprintf(LTmpl, __LINE__, lbl.data()));
            }

            builder.Finalize();
        } catch (const yexception& e) {
            builder.Dump(Clog);
            throw;
        }

        TBlob b = builder.Compact();

        TSolarTrie::TImpl trie;
        trie.Wrap(b);

        try {
            UNIT_ASSERT_VALUES_EQUAL_C(asz, trie.Size(), lbl.data());
            UNIT_ASSERT_VALUES_EQUAL_C(!asz, trie.Empty(), lbl.data());

            for (ui32 i = 0; i < sz; ++i) {
                TestGet(TStringBuf(values[i]), trie, Sprintf(LTmpl, __LINE__, lbl.data()) + " -> " + values[i], i < asz);
            }

            TSolarTrie::TConstIterator it = trie.Iterator();

            for (ui32 testn = 0; testn < 2; ++testn) {
                ui32 n = 0;
                TBuffer keylast, key, val;

                while (it.Next()) {
                    if (!(n % 10))
                        it = it.Clone();

                    ++n;
                    UNIT_ASSERT_C(it.Current(key, val), lbl.data());
                    TStringBuf lastk(keylast.Data(), keylast.Size());
                    TStringBuf k(key.Data(), key.Size());
                    UNIT_ASSERT_C(lastk != k, lbl.data());

                    UNIT_ASSERT_VALUES_EQUAL_C(
                        TStringBuf(key.Data(), key.Size()),
                        TStringBuf(val.Data(), val.Size()), lbl.data());

                    keylast.Assign(key.Data(), key.Size());
                }

                UNIT_ASSERT_VALUES_EQUAL_C(n, asz, lbl.data());
                it.Restart();
            }
        } catch (const yexception& e) {
            trie.Dump(Clog);
            throw;
        }
    }

    void DoTestTrie(NSolarTrie::TSolarTrieConf c, TString lbl) {
        const char* values[] = {
            "! сентября газета", "!(возмездие это)!", "!(материнский капитал)", "!(пермь березники)", "!биография | !жизнь / + розинг | зворыгин & изобретение | телевидение | электронно лучевая трубка", "!овсиенко николай павлович", "!путин", "\"i'm on you\" p. diddy тимати клип", "\"билайн\" представит собственный планшет", "\"в особо крупном размере\"", "\"викиликс\" джулиан ассанж", "\"вимм билль данн", "\"газэнергосеть астрахань", "\"газэнергосеть астрахань\"", "\"домодедово\" ту-154", "\"жилина\" \"спартак\" видео", "\"зелёнsq шершнm\"", "\"зелёного шершня\"", "\"золотой граммофон\" марины яблоковой", "\"золотой граммофон-2010\"", "\"калинниковы\"", "\"манчестер юнайтед\" (англия) \"валенсия\" (испания) 1:1 (0:1)", "\"маркер\"", "\"моника\" засыпает москву снегом", "\"моника\" снегопад", "\"о безопасности\",", "\"памятку\" для пассажиров воздушных международных рейсов", "\"петровский парк\" и \"ходынское поле\"", "\"путинская\" трава", "\"пятерочка\"купила \"копейку\"", "\"пятёрочка\" и \"копейка\" объединились", "\"реал\" \"осер\" 4:0", "\"речь мутко\"", "\"российский лес 2010\"", "\"ростехинвентаризация федеральное бти\" рубцов", "\"саня останется с нами\",", "\"следопыт\" реалити шоу", "\"слышишь\" молодые авторы", "\"стадион\"", "\"ходынское поле\" метро", "\"хроники нарнии\"", "\"чистая вода\"", "\"школа деда мороза\"", "# asus -1394", "# сторонники wikileaks", "#106#", "#11", "#8 какой цвет", "#если клиент", "$ 13,79", "$ xnj ,s dct ,skb ljdjkmys !!!", "$ в день", "$ диск компьютера", "$.ajax", "$125 000", "$курс", "% в си", "% влады", "% годовых", "% женщин и % мужчин в россии", "% занятости персонала", "% инфляции 2010", "% инфляции в 2010 г.", "% налога", "% налогов в 2010г.", "% общего количества", "% от числа", "% по налогу на прибыль организации", "%24", "%академия%", "%комарова%татьяна", "& в 1с", "&& (+не существует | !такой проблемы)", "&gt;&gt;&gt;скачать | download c cs strikez.clan.su&lt;&lt;&lt;", "&gt;hbq nbityrjd", "&lt; какой знак", "&lt; лицей | &lt; техническая школа# &lt; история#&lt; лицей сегодня#&lt; "
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          "перечень профессий#&lt; руководство лицея#&lt; прием учащихся#&lt; "
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          "контакты#&lt; схема проезда#&lt; фотогалереяистория создания лицея "
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          "и основные этапы путиулица купчинская дом 28",
            "&lt;&lt;link&gt;&gt;", "&lt;/storage&gt;", "&lt;bfnkjy", "&lt;bktntd", "&lt;cr", "&lt;ddr3&gt;", "&lt;e[ufknthcrbq abyfycjdsq", "&lt;fcctqys", "&lt;fhcf", "&lt;fhctkjyf he,by", "&lt;firbhbz", "&lt;fyr djphj;ltybt", "&lt;fyr vjcrds", "&lt;fyr резерв", "&lt;fyufkjh", "&lt;index&gt;", "&lt;jkmifz jrhe;yfz rbtd", "&lt;kbpytws", "&lt;megafon&gt; интернет", "&lt;thtpybrb gthvcrbq rhfq", "&lt;tkjxrf", "&lt;беларусь это мы", "&lt;бокс, версия ibf"};

        CheckTrie(values, Y_ARRAY_SIZE(values), 0, c, lbl);
        CheckTrie(values, Y_ARRAY_SIZE(values), Max<ui32>(Min<ui32>(c.BucketMaxPrefixSize, c.BucketMaxSize), 2) - 1, c, lbl);
        CheckTrie(values, Y_ARRAY_SIZE(values), Min<ui32>(c.BucketMaxPrefixSize, c.BucketMaxSize) + 1, c, lbl);
        CheckTrie(values, Y_ARRAY_SIZE(values), Max<ui32>(Max<ui32>(c.BucketMaxPrefixSize, c.BucketMaxSize), 2) - 1, c, lbl);
        CheckTrie(values, Y_ARRAY_SIZE(values), Max<ui32>(c.BucketMaxPrefixSize, c.BucketMaxSize) + 1, c, lbl);
        CheckTrie(values, Y_ARRAY_SIZE(values), Y_ARRAY_SIZE(values), c, lbl);
    }

    void DoTestTrie2(NSolarTrie::TSolarTrieConf c, TString lbl) {
        const char* values[] = {
            "000", "0000", "00000", "000000", "0000000", "00000000", "000000000", "0000000000", "00000000000"};

        CheckTrie(values, Y_ARRAY_SIZE(values), 0, c, lbl);
        CheckTrie(values, Y_ARRAY_SIZE(values), Min<ui32>(Y_ARRAY_SIZE(values), Max<ui32>(Min<ui32>(c.BucketMaxPrefixSize, c.BucketMaxSize), 2) - 1), c, lbl);
        CheckTrie(values, Y_ARRAY_SIZE(values), Min<ui32>(Y_ARRAY_SIZE(values), Min<ui32>(c.BucketMaxPrefixSize, c.BucketMaxSize) + 1), c, lbl);
        CheckTrie(values, Y_ARRAY_SIZE(values), Min<ui32>(Y_ARRAY_SIZE(values), Max<ui32>(Max<ui32>(c.BucketMaxPrefixSize, c.BucketMaxSize), 2) - 1), c, lbl);
        CheckTrie(values, Y_ARRAY_SIZE(values), Min<ui32>(Y_ARRAY_SIZE(values), Max<ui32>(c.BucketMaxPrefixSize, c.BucketMaxSize) + 1), c, lbl);
        CheckTrie(values, Y_ARRAY_SIZE(values), Y_ARRAY_SIZE(values), c, lbl);
    }

    void TestTrie1() {
        using namespace NSolarTrie;

        TSolarTrieConf c;
        c.RawValuesBlockSize = 10;
        c.SetKeyCompression("");
        c.SetSuffixCompression("");
        c.SetSuffixLengthCompression("");
        c.SetSuffixBlockCompression("");
        c.SetValueCompression("");
        c.SetValueLengthCompression("");
        c.SetValueBlockCompression("");

        DoTestTrieBuild(c, c.ReportCodecs());
        DoTestTrie(c, c.ReportCodecs());
        DoTestTrie2(c, c.ReportCodecs());
    }

    void TestTrie2() {
        using namespace NSolarTrie;

        TSolarTrieConf c;
        c.RawValuesBlockSize = 10;
        c.SetKeyCompression("");
        c.SetSuffixCompression("");
        c.SetSuffixLengthCompression("");
        c.SetSuffixBlockCompression("huffman");
        c.SetValueCompression("");
        c.SetValueLengthCompression("");
        c.SetValueBlockCompression("huffman");

        DoTestTrieBuild(c, c.ReportCodecs());
        DoTestTrie(c, c.ReportCodecs());
        DoTestTrie2(c, c.ReportCodecs());
    }

    void TestTrie3() {
        using namespace NSolarTrie;

        TSolarTrieConf c;
        c.RawValuesBlockSize = 10;
        c.SetKeyCompression("");
        c.SetSuffixCompression("huffman");
        c.SetSuffixLengthCompression("");
        c.SetSuffixBlockCompression("");
        c.SetValueCompression("huffman");
        c.SetValueLengthCompression("");
        c.SetValueBlockCompression("");

        DoTestTrieBuild(c, c.ReportCodecs());
        DoTestTrie(c, c.ReportCodecs());
        DoTestTrie2(c, c.ReportCodecs());
    }

    void TestTrie4() {
        using namespace NSolarTrie;

        TSolarTrieConf c;
        c.RawValuesBlockSize = 10;
        c.SetKeyCompression("");
        c.SetSuffixCompression("");
        c.SetSuffixLengthCompression("pfor-delta64-sorted");
        c.SetSuffixBlockCompression("");
        c.SetValueCompression("");
        c.SetValueLengthCompression("pfor-delta64-sorted:huffman");
        c.SetValueBlockCompression("");

        DoTestTrieBuild(c, c.ReportCodecs());
        DoTestTrie(c, c.ReportCodecs());
        DoTestTrie2(c, c.ReportCodecs());
    }

    void TestTrie5() {
        using namespace NSolarTrie;

        TSolarTrieConf c;
        c.RawValuesBlockSize = 10;
        c.SetKeyCompression("");
        c.SetSuffixCompression("huffman");
        c.SetSuffixLengthCompression("pfor-delta64-sorted");
        c.SetSuffixBlockCompression("");
        c.SetValueCompression("zlib");
        c.SetValueLengthCompression("pfor-delta64-sorted:huffman");
        c.SetValueBlockCompression("");

        DoTestTrieBuild(c, c.ReportCodecs());
        DoTestTrie(c, c.ReportCodecs());
        DoTestTrie2(c, c.ReportCodecs());
    }

    void TestTrie6() {
        using namespace NSolarTrie;

        TSolarTrieConf c;
        c.RawValuesBlockSize = 10;
        c.SetKeyCompression("");
        c.SetSuffixCompression("huffman");
        c.SetSuffixLengthCompression("pfor-delta64-sorted");
        c.SetSuffixBlockCompression("lz4");
        c.SetValueCompression("zlib");
        c.SetValueLengthCompression("pfor-delta64-sorted:huffman");
        c.SetValueBlockCompression("huffman");

        DoTestTrieBuild(c, c.ReportCodecs());
        DoTestTrie(c, c.ReportCodecs());
        DoTestTrie2(c, c.ReportCodecs());
    }

    void TestTrie7() {
        using namespace NSolarTrie;

        TSolarTrieConf c;
        c.RawValuesBlockSize = 10;
        c.SetKeyCompression("huffman");
        c.SetSuffixCompression("");
        c.SetSuffixLengthCompression("");
        c.SetSuffixBlockCompression("");
        c.SetValueCompression("");
        c.SetValueLengthCompression("");
        c.SetValueBlockCompression("");

        DoTestTrieBuild(c, c.ReportCodecs());
        DoTestTrie(c, c.ReportCodecs());
        DoTestTrie2(c, c.ReportCodecs());
    }

    void TestTrie8() {
        using namespace NSolarTrie;

        TSolarTrieConf c;
        c.RawValuesBlockSize = 10;
        c.SetKeyCompression("huffman");
        c.SetSuffixCompression("");
        c.SetSuffixLengthCompression("");
        c.SetSuffixBlockCompression("huffman");
        c.SetValueCompression("");
        c.SetValueLengthCompression("");
        c.SetValueBlockCompression("huffman");

        DoTestTrieBuild(c, c.ReportCodecs());
        DoTestTrie(c, c.ReportCodecs());
        DoTestTrie2(c, c.ReportCodecs());
    }

    void TestTrie9() {
        using namespace NSolarTrie;

        TSolarTrieConf c;
        c.RawValuesBlockSize = 10;
        c.SetKeyCompression("huffman");
        c.SetSuffixCompression("huffman");
        c.SetSuffixLengthCompression("");
        c.SetSuffixBlockCompression("");
        c.SetValueCompression("huffman");
        c.SetValueLengthCompression("");
        c.SetValueBlockCompression("");

        DoTestTrieBuild(c, c.ReportCodecs());
        DoTestTrie(c, c.ReportCodecs());
        DoTestTrie2(c, c.ReportCodecs());
    }

    void TestTrie10() {
        using namespace NSolarTrie;

        TSolarTrieConf c;
        c.RawValuesBlockSize = 10;
        c.SetKeyCompression("huffman");
        c.SetSuffixCompression("");
        c.SetSuffixLengthCompression("pfor-delta64-sorted");
        c.SetSuffixBlockCompression("");
        c.SetValueCompression("");
        c.SetValueLengthCompression("pfor-delta64-sorted:huffman");
        c.SetValueBlockCompression("");

        DoTestTrieBuild(c, c.ReportCodecs());
        DoTestTrie(c, c.ReportCodecs());
        DoTestTrie2(c, c.ReportCodecs());
    }

    void TestTrie11() {
        using namespace NSolarTrie;

        TSolarTrieConf c;
        c.RawValuesBlockSize = 10;
        c.SetKeyCompression("huffman");
        c.SetSuffixCompression("huffman");
        c.SetSuffixLengthCompression("pfor-delta64-sorted");
        c.SetSuffixBlockCompression("");
        c.SetValueCompression("zlib");
        c.SetValueLengthCompression("pfor-delta64-sorted:huffman");
        c.SetValueBlockCompression("");

        DoTestTrieBuild(c, c.ReportCodecs());
        DoTestTrie(c, c.ReportCodecs());
        DoTestTrie2(c, c.ReportCodecs());
    }

    void TestTrie12() {
        using namespace NSolarTrie;

        TSolarTrieConf c;
        c.RawValuesBlockSize = 10;
        c.SetKeyCompression("huffman");
        c.SetSuffixCompression("lz4");
        c.SetSuffixLengthCompression("pfor-delta64-sorted");
        c.SetSuffixBlockCompression("zlib");
        c.SetValueCompression("zlib");
        c.SetValueLengthCompression("pfor-delta64-sorted:huffman");
        c.SetValueBlockCompression("huffman");

        DoTestTrieBuild(c, c.ReportCodecs());
        DoTestTrie(c, c.ReportCodecs());
        DoTestTrie2(c, c.ReportCodecs());
    }

    void TestTrie13() {
        using namespace NSolarTrie;

        TSolarTrieConf c;
        c.RawValuesBlockSize = 10;
        c.SetKeyCompression("");
        c.SetSuffixCompression("");
        c.SetSuffixLengthCompression("pfor-delta32-sorted");
        c.SetSuffixBlockCompression("");
        c.SetValueCompression("");
        c.SetValueLengthCompression("pfor-delta32-sorted:huffman");
        c.SetValueBlockCompression("");

        DoTestTrieBuild(c, c.ReportCodecs());
        DoTestTrie(c, c.ReportCodecs());
        DoTestTrie2(c, c.ReportCodecs());
    }

    void TestTrie14() {
        using namespace NSolarTrie;

        TSolarTrieConf c;
        c.RawValuesBlockSize = 10;
        c.SetKeyCompression("");
        c.SetSuffixCompression("huffman");
        c.SetSuffixLengthCompression("pfor-delta32-sorted");
        c.SetSuffixBlockCompression("");
        c.SetValueCompression("zlib");
        c.SetValueLengthCompression("pfor-delta32-sorted:huffman");
        c.SetValueBlockCompression("");

        DoTestTrieBuild(c, c.ReportCodecs());
        DoTestTrie(c, c.ReportCodecs());
        DoTestTrie2(c, c.ReportCodecs());
    }

    void TestTrie15() {
        using namespace NSolarTrie;

        TSolarTrieConf c;
        c.RawValuesBlockSize = 10;
        c.SetKeyCompression("");
        c.SetSuffixCompression("lz4hc");
        c.SetSuffixLengthCompression("pfor-delta32-sorted");
        c.SetSuffixBlockCompression("lz4");
        c.SetValueCompression("zlib");
        c.SetValueLengthCompression("pfor-delta32-sorted:huffman");
        c.SetValueBlockCompression("huffman");

        DoTestTrieBuild(c, c.ReportCodecs());
        DoTestTrie(c, c.ReportCodecs());
        DoTestTrie2(c, c.ReportCodecs());
    }

    void TestTrie16() {
        using namespace NSolarTrie;

        TSolarTrieConf c;
        c.RawValuesBlockSize = 10;
        c.SetKeyCompression("huffman");
        c.SetSuffixCompression("");
        c.SetSuffixLengthCompression("pfor-delta32-sorted");
        c.SetSuffixBlockCompression("");
        c.SetValueCompression("");
        c.SetValueLengthCompression("pfor-delta32-sorted:huffman");
        c.SetValueBlockCompression("");

        DoTestTrieBuild(c, c.ReportCodecs());
        DoTestTrie(c, c.ReportCodecs());
        DoTestTrie2(c, c.ReportCodecs());
    }

    void TestTrie17() {
        using namespace NSolarTrie;

        TSolarTrieConf c;
        c.RawValuesBlockSize = 10;
        c.SetKeyCompression("huffman");
        c.SetSuffixCompression("huffman");
        c.SetSuffixLengthCompression("pfor-delta32-sorted");
        c.SetSuffixBlockCompression("");
        c.SetValueCompression("zlib");
        c.SetValueLengthCompression("pfor-delta32-sorted:huffman");
        c.SetValueBlockCompression("");

        DoTestTrieBuild(c, c.ReportCodecs());
        DoTestTrie(c, c.ReportCodecs());
        DoTestTrie2(c, c.ReportCodecs());
    }

    void TestTrie18() {
        using namespace NSolarTrie;

        TSolarTrieConf c;
        c.RawValuesBlockSize = 10;
        c.SetKeyCompression("huffman");
        c.SetSuffixCompression("lz4");
        c.SetSuffixLengthCompression("pfor-delta32-sorted");
        c.SetSuffixBlockCompression("lz4");
        c.SetValueCompression("zlib");
        c.SetValueLengthCompression("pfor-delta32-sorted:huffman");
        c.SetValueBlockCompression("huffman");

        DoTestTrieBuild(c, c.ReportCodecs());
        DoTestTrie(c, c.ReportCodecs());
        DoTestTrie2(c, c.ReportCodecs());
    }
};

const char TSolarTrieTest::LTmpl[] = "line %d of test %s";
UNIT_TEST_SUITE_REGISTRATION(TSolarTrieTest)
