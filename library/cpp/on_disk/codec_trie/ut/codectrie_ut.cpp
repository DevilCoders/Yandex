#include <library/cpp/on_disk/codec_trie/codectrie.h>
#include <library/cpp/on_disk/codec_trie/triebuilder.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/charset/wide.h>
#include <util/generic/hash.h>
#include <util/generic/buffer.h>
#include <util/string/util.h>
#include <util/string/builder.h>

namespace NCodecTrie {
    // an example of a complex piece of data to hold
    struct TComplexData {
        TUtf16String Text;
        ui64 Freq = 0;

        TComplexData() = default;

        TComplexData(TStringBuf akey)
            : Text(UTF8ToWide(akey))
            , Freq(ComputeHash(akey))
        {
        }

        bool operator==(const TComplexData& other) const {
            return Freq == other.Freq && Text == other.Text;
        }

        bool operator!=(const TComplexData& other) const {
            return !(*this == other);
        }
    };

}

template <>
void Out<NCodecTrie::TComplexData>(IOutputStream& out, TTypeTraits<NCodecTrie::TComplexData>::TFuncParam d) {
    out << "TComplexData{Text=" << d.Text << ",Freq=" << d.Freq << "}";
}

template <>
class TCompactTriePacker<NCodecTrie::TComplexData> {
    using TData = std::pair<TWtringBuf, ui64>;
    using TPacker = NPackers::TPairPacker<TWtringBuf, ui64>;

public:
    void UnpackLeaf(const char* buf, NCodecTrie::TComplexData& data) const {
        TData tmp;
        TPacker().UnpackLeaf(buf, tmp);
        data.Freq = tmp.second;
        data.Text = tmp.first;
    }

    void PackLeaf(char* buf, const NCodecTrie::TComplexData& data, size_t sz) const {
        TPacker().PackLeaf(buf, TData(data.Text, data.Freq), sz);
    }

    size_t MeasureLeaf(const NCodecTrie::TComplexData& data) const {
        return TPacker().MeasureLeaf(TData(data.Text, data.Freq));
    }

    size_t SkipLeaf(const char* buf) const {
        return TPacker().SkipLeaf(buf);
    }
};

namespace NCodecTrie {
    template <typename TVal>
    struct TConvert;

    template <>
    struct TConvert<TStringBuf> {
        static TStringBuf Do(TStringBuf key) {
            return key;
        }
    };

    template <>
    struct TConvert<ui64> {
        static ui64 Do(TStringBuf key) {
            return ComputeHash(key);
        }
    };

    template <>
    struct TConvert<TComplexData> {
        static TComplexData Do(TStringBuf key) {
            return TComplexData(key);
        }
    };

    template <typename TVal>
    struct TTest {
        using TTrie = NCodecTrie::TCodecTrie<TVal>;
        using TConstIterator = typename TTrie::TConstIterator;

        TTrie Trie;
        typename TTrie::TBuilder Builder;
        mutable TBuffer Buffer;
        mutable TBuffer AuxBuffer;
        TString Label;
        size_t Size = 0;

        TTest(const NCodecTrie::TCodecTrieConf& conf, TStringBuf lbl)
            : Builder(conf)
            , Label(lbl)
        {
        }

        void Add(TStringBuf key) {
            Builder.Add(key, TConvert<TVal>::Do(key));
            ++Size;
        }

        void Add(ui32 key) {
            Add(ToString(key));
        }

        void Commit() {
            Trie.Init(Builder.Compact());
        }

        TString ReportKey(TStringBuf key) const {
            return TStringBuilder() << "'" << key << "' @ " << Label << ", " << Trie.GetConf().ReportCodecs();
        }

        void Get(TStringBuf key) const {
            TVal val;
            UNIT_ASSERT_C(Trie.Get(key, val, Buffer, AuxBuffer), ReportKey(key));
            UNIT_ASSERT_VALUES_EQUAL_C(val, TConvert<TVal>::Do(key), ReportKey(key));

            if (Trie.SupportsPrefixLookups()) {
                TStringBuf prefix = key;
                do {
                    UNIT_ASSERT_C(Trie.HasPrefix(prefix), ReportKey(key));
                    prefix.Chop(1);
                } while (prefix);
            }
        }

        void Get(ui32 key) const {
            Get(ToString(key));
        }

        TConstIterator Iterator() const {
            return Trie.Iterator();
        }
    };

}

class TCodecTrieTest: public TTestBase {
    UNIT_TEST_SUITE(TCodecTrieTest);
    UNIT_TEST(TestTrie1)
    UNIT_TEST(TestTrie2)
    UNIT_TEST(TestTrie3)
    UNIT_TEST(TestTrie4)
    UNIT_TEST(TestTrie5)
    UNIT_TEST(TestTrie6)
    UNIT_TEST(TestTrie7)

    UNIT_TEST_SUITE_END();

private:
    static const char LTmpl[];

    template <typename TVal>
    void DoTestTrieIterator(const NCodecTrie::TTest<TVal>& test) {
        using namespace NCodecTrie;
        typename TTest<TVal>::TConstIterator it = test.Iterator();

        ui32 n = 0;
        TVal val = TVal();
        TString key;
        while (it.Next()) {
            ++n;
            UNIT_ASSERT_C(it.Current(key, val), test.ReportKey(key));
            UNIT_ASSERT_EQUAL_C(TConvert<TVal>::Do(key), val, test.ReportKey(key));
        }

        UNIT_ASSERT_VALUES_EQUAL_C(n, test.Size, test.Label);
    }

    template <typename TVal>
    void DoTestTrieBuild(NCodecTrie::TCodecTrieConf c, TString lbl) {
        using namespace NCodecTrie;

        TTest<TVal> test(c, lbl + " numbers");

        test.Add(0);
        test.Add(1);
        test.Add(11);
        test.Add(110);

        for (ui32 i = 31; i >= 4; --i) {
            test.Add(1100 + i);
        }

        test.Add(1100 + 32);
        test.Add(5);
        test.Add(4);
        test.Add(41);
        test.Add(410);

        test.Add("");

        for (ui32 i = 31; i >= 4; --i) {
            test.Add(4100 + i);
        }

        test.Add(4100 + 32);

        test.Commit();

        test.Get("");
        test.Get((ui32)0);
        test.Get(1);
        test.Get(11);
        test.Get(110);

        for (ui32 i = 4; i <= 32; ++i) {
            test.Get(1100 + i);
        }

        test.Get(5);
        test.Get(4);
        test.Get(41);
        test.Get(410);

        for (ui32 i = 4; i <= 32; ++i) {
            test.Get(4100 + i);
        }

        DoTestTrieIterator(test);
    }

    template <typename TVal>
    void DoTestTrie(NCodecTrie::TCodecTrieConf c, TString lbl) {
        TStringBuf values[]{
            "! сентября газета", "!(возмездие это)!", "!(материнский капитал)", "!(пермь березники)", "!биография | !жизнь / + розинг | зворыгин & изобретение | телевидение | "
                                                                                                      "электронно лучевая трубка",
            "!овсиенко николай павлович", "!путин", "\"i'm on you\" p. diddy тимати клип", "\"билайн\" представит собственный планшет", "\"в особо крупном размере\"", "\"викиликс\" джулиан ассанж", "\"вимм билль данн", "\"газэнергосеть астрахань", "\"газэнергосеть астрахань\"", "\"домодедово\" ту-154", "\"жилина\" \"спартак\" видео", "\"зелёнsq шершнm\"", "\"зелёного шершня\"", "\"золотой граммофон\" марины яблоковой", "\"золотой граммофон-2010\"", "\"калинниковы\"", "\"манчестер юнайтед\" (англия) \"валенсия\" (испания) 1:1 (0:1)", "\"маркер\"", "\"моника\" засыпает москву снегом", "\"моника\" снегопад", "\"о безопасности\",", "\"памятку\" для пассажиров воздушных международных рейсов", "\"петровский парк\" и \"ходынское поле\"", "\"путинская\" трава", "\"пятерочка\"купила \"копейку\"", "\"пятёрочка\" и \"копейка\" объединились", "\"реал\" \"осер\" 4:0", "\"речь мутко\"", "\"российский лес 2010\"", "\"ростехинвентаризация федеральное бти\" рубцов", "\"саня останется с нами\",", "\"следопыт\" реалити шоу", "\"слышишь\" молодые авторы", "\"стадион\"", "\"ходынское поле\" метро", "\"хроники нарнии\"", "\"чистая вода\"", "\"школа деда мороза\"", "# asus -1394", "# сторонники wikileaks", "#106#", "#11", "#8 какой цвет", "#если клиент", "$ 13,79", "$ xnj ,s dct ,skb ljdjkmys !!!", "$ в день", "$ диск компьютера", "$.ajax", "$125 000", "$курс", "% в си", "% влады", "% годовых", "% женщин и % мужчин в россии", "% занятости персонала", "% инфляции 2010", "% инфляции в 2010 г.", "% налога", "% налогов в 2010г.", "% общего количества", "% от числа", "% по налогу на прибыль организации", "%24", "%академия%", "%комарова%татьяна", "& в 1с", "&& (+не существует | !такой проблемы)", "&gt;&gt;&gt;скачать | download c cs strikez.clan.su&lt;&lt;&lt;", "&gt;hbq nbityrjd", "&lt; какой знак", "&lt; лицей | &lt; техническая школа# &lt; история#&lt; лицей сегодня#&lt; "
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           "перечень профессий#&lt; руководство лицея#&lt; прием учащихся#&lt; "
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           "контакты#&lt; схема проезда#&lt; фотогалереяистория создания лицея и"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           " основные этапы путиулица купчинская дом 28",
            "&lt;&lt;link&gt;&gt;", "&lt;/storage&gt;", "&lt;bfnkjy", "&lt;bktntd", "&lt;cr", "&lt;ddr3&gt;", "&lt;e[ufknthcrbq abyfycjdsq", "&lt;fcctqys", "&lt;fhcf", "&lt;fhctkjyf he,by", "&lt;firbhbz", "&lt;fyr djphj;ltybt", "&lt;fyr vjcrds", "&lt;fyr резерв", "&lt;fyufkjh", "&lt;index&gt;", "&lt;jkmifz jrhe;yfz rbtd", "&lt;kbpytws", "&lt;megafon&gt; интернет", "&lt;thtpybrb gthvcrbq rhfq", "&lt;tkjxrf", "&lt;беларусь это мы", "&lt;бокс, версия ibf"};

        using namespace NCodecTrie;
        TTest<TVal> test(c, lbl + " queries");

        for (const auto v : values) {
            test.Add(v);
        }

        test.Commit();

        for (const auto v : values) {
            test.Get(v);
        }

        DoTestTrieIterator(test);
    }

    void TestTrie(const NCodecTrie::TCodecTrieConf& c) {
        using namespace NCodecTrie;

        DoTestTrieBuild<ui64>(c, "ui64");
        DoTestTrie<ui64>(c, "ui64");

        DoTestTrieBuild<TStringBuf>(c, "TStringBuf");
        DoTestTrie<TStringBuf>(c, "TStringBuf");

        DoTestTrieBuild<TComplexData>(c, "TComplexData");
        DoTestTrie<TComplexData>(c, "TComplexData");
    }

    void TestTrie1() {
        using namespace NCodecTrie;

        TCodecTrieConf c;
        c.SetKeyCompression("none");
        c.SetValueCompression("none");

        TestTrie(c);
    }

    void TestTrie2() {
        using namespace NCodecTrie;
        TCodecTrieConf c;
        c.SetKeyCompression("huffman");
        c.SetValueCompression("none");

        TestTrie(c);
    }

    void TestTrie3() {
        using namespace NCodecTrie;
        TCodecTrieConf c;
        c.SetKeyCompression("lz4fast");
        c.SetValueCompression("none");

        TestTrie(c);
    }

    void TestTrie4() {
        using namespace NCodecTrie;

        TCodecTrieConf c;
        c.SetKeyCompression("none");
        c.SetValueCompression("huffman");

        TestTrie(c);
    }

    void TestTrie5() {
        using namespace NCodecTrie;
        TCodecTrieConf c;
        c.SetKeyCompression("huffman");
        c.SetValueCompression("huffman");

        TestTrie(c);
    }

    void TestTrie6() {
        using namespace NCodecTrie;
        TCodecTrieConf c;
        c.SetKeyCompression("lz4fast");
        c.SetValueCompression("huffman");

        TestTrie(c);
    }

    void TestTrie7() {
        using namespace NCodecTrie;
        TCodecTrieConf c;
        c.SetKeyCompression("lz4fast");
        c.SetValueCompression("lz4fast");

        TestTrie(c);
    }
};

const char TCodecTrieTest::LTmpl[] = "line %d of test %s";
UNIT_TEST_SUITE_REGISTRATION(TCodecTrieTest)
