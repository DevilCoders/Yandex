#include <kernel/remorph/misc/unicode/unicode_set.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/charset/unidata.h>
#include <util/charset/wide.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/yexception.h>
#include <util/stream/format.h>

using namespace NUnicode;

inline void TestCase_MakeCaseInsensitive(WC_TYPE t) {
    TUnicodeSet us(t);
    us.MakeCaseInsensitive();
    const TUtf16String lowerSet = u"abcdezабвгдеяαβγδεω";
    const TUtf16String upperSet = u"ABCDEZАБВГДЕЯΑΒΓΔΕΩ";
    const TUtf16String nonletterSet = u"0123456!@#&^().,";
    for (size_t i = 0; i < lowerSet.length(); ++i) {
        UNIT_ASSERT_C(us.Has(lowerSet[i]), ",char='" << WideToUTF8(~lowerSet + i, 1) << "'");
    }
    for (size_t i = 0; i < upperSet.length(); ++i) {
        UNIT_ASSERT_C(us.Has(upperSet[i]), ",char='" << WideToUTF8(~upperSet + i, 1) << "'");
    }
    for (size_t i = 0; i < nonletterSet.length(); ++i) {
        UNIT_ASSERT_C(!us.Has(nonletterSet[i]), ",char='" << WideToUTF8(~nonletterSet + i, 1) << "'");
    }
    UNIT_ASSERT(us.Has(0x000001C5)); // ǅ
}

inline void TestRealEquality(const TUnicodeSet& us1, const TUnicodeSet& us2) {
    for (wchar32 c = 0; c < TUnicodeSet::CODEPOINT_HIGH; ++c) {
        UNIT_ASSERT_EQUAL_C(us1.Has(c), us2.Has(c),
                            ", for U+" << ::Hex(c) << " " << ::UTF32ToWide(&c, 1u));
    }
}

Y_UNIT_TEST_SUITE(UnicodeSet) {
    Y_UNIT_TEST(SetRange) {
        TUnicodeSet us('a', 'z');
        for (wchar32 c = 'a'; c <= 'z'; ++c) {
            UNIT_ASSERT_C(us.Has(c), ",char='" << char(c) << "'");
        }
        UNIT_ASSERT(!us.Has('a' - 1));
        UNIT_ASSERT(!us.Has('z' + 1));
        UNIT_ASSERT(!us.Has('0'));
        UNIT_ASSERT(!us.Has('.'));
        UNIT_ASSERT(!us.Has('A'));
    }

    Y_UNIT_TEST(SetString) {
        const TUtf16String strSet = u"abc123[]";
        TUnicodeSet us(strSet);
        for (size_t i = 0; i < strSet.length(); ++i) {
            UNIT_ASSERT_C(us.Has(strSet[i]), ",char='" << WideToUTF8(~strSet + i, 1) << "'");
        }
        UNIT_ASSERT(!us.Has('d'));
        UNIT_ASSERT(!us.Has('.'));
        UNIT_ASSERT(!us.Has('7'));
    }

    Y_UNIT_TEST(SetCategory) {
        TUnicodeSet us(Ll_LOWER);
        const TUtf16String lowerSet = u"abcdezабвгдеяαβγδεω";
        const TUtf16String upperSet = u"ABCDEZАБВГДЕЯΑΒΓΔΕΩ";
        const TUtf16String nonletterSet = u"0123456!@#&^().,";
        for (size_t i = 0; i < lowerSet.length(); ++i) {
            UNIT_ASSERT_C(us.Has(lowerSet[i]), ",char='" << WideToUTF8(~lowerSet + i, 1) << "'");
        }
        for (size_t i = 0; i < upperSet.length(); ++i) {
            UNIT_ASSERT_C(!us.Has(upperSet[i]), ",char='" << WideToUTF8(~upperSet + i, 1) << "'");
        }
        for (size_t i = 0; i < nonletterSet.length(); ++i) {
            UNIT_ASSERT_C(!us.Has(nonletterSet[i]), ",char='" << WideToUTF8(~nonletterSet + i, 1) << "'");
        }

        us.Set(Lu_UPPER);
        for (size_t i = 0; i < lowerSet.length(); ++i) {
            UNIT_ASSERT_C(!us.Has(lowerSet[i]), ",char='" << WideToUTF8(~lowerSet + i, 1) << "'");
        }
        for (size_t i = 0; i < upperSet.length(); ++i) {
            UNIT_ASSERT_C(us.Has(upperSet[i]), ",char='" << WideToUTF8(~upperSet + i, 1) << "'");
        }
        for (size_t i = 0; i < nonletterSet.length(); ++i) {
            UNIT_ASSERT_C(!us.Has(nonletterSet[i]), ",char='" << WideToUTF8(~nonletterSet + i, 1) << "'");
        }

        us.Set(Lt_TITLE);
        UNIT_ASSERT(us.Has(0x000001C5)); // ǅ

        UNIT_ASSERT_EXCEPTION(us.SetCategory(TStringBuf("unknown")), yexception);
        UNIT_ASSERT_EQUAL(us, TUnicodeSet(Lt_TITLE));

        us.SetCategory(TStringBuf("L"));
        for (size_t i = 0; i < lowerSet.length(); ++i) {
            UNIT_ASSERT_C(us.Has(lowerSet[i]), ",char='" << WideToUTF8(~lowerSet + i, 1) << "'");
        }
        for (size_t i = 0; i < upperSet.length(); ++i) {
            UNIT_ASSERT_C(us.Has(upperSet[i]), ",char='" << WideToUTF8(~upperSet + i, 1) << "'");
        }
        for (size_t i = 0; i < nonletterSet.length(); ++i) {
            UNIT_ASSERT_C(!us.Has(nonletterSet[i]), ",char='" << WideToUTF8(~nonletterSet + i, 1) << "'");
        }
        UNIT_ASSERT(us.Has(0x000001C5)); // ǅ
    }

    Y_UNIT_TEST(InvertCategory) {
        TUnicodeSet us(Ll_LOWER);
        us.Invert();
        const TUtf16String lowerSet = u"abcdezабвгдеяαβγδεω";
        const TUtf16String upperSet = u"ABCDEZАБВГДЕЯΑΒΓΔΕΩ";
        const TUtf16String nonletterSet = u"0123456!@#&^().,";
        for (size_t i = 0; i < lowerSet.length(); ++i) {
            UNIT_ASSERT_C(!us.Has(lowerSet[i]), ",char='" << WideToUTF8(~lowerSet + i, 1) << "'");
        }
        for (size_t i = 0; i < upperSet.length(); ++i) {
            UNIT_ASSERT_C(us.Has(upperSet[i]), ",char='" << WideToUTF8(~upperSet + i, 1) << "'");
        }
        for (size_t i = 0; i < nonletterSet.length(); ++i) {
            UNIT_ASSERT_C(us.Has(nonletterSet[i]), ",char='" << WideToUTF8(~nonletterSet + i, 1) << "'");
        }
        us.Clear().Add(0, 32).Add('~', TUnicodeSet::CODEPOINT_HIGH - 1).Invert();
        UNIT_ASSERT(us.Has(33) && us.Has(34) && us.Has('~' - 1));
        UNIT_ASSERT(!us.Has(0) && !us.Has(1) && !us.Has(32) && !us.Has('~') && !us.Has(TUnicodeSet::CODEPOINT_HIGH - 1));
    }

    Y_UNIT_TEST(InvertEmpty) {
        TUnicodeSet us;
        UNIT_ASSERT(us.Empty());
        us.Invert();
        UNIT_ASSERT(!us.Empty());
        UNIT_ASSERT(us.Has(0));
        UNIT_ASSERT(us.Has(TUnicodeSet::CODEPOINT_HIGH - 1));
        UNIT_ASSERT(!us.Has(TUnicodeSet::CODEPOINT_HIGH));
    }

    Y_UNIT_TEST(InvertRanges) {
        TUnicodeSet us(0, 32);
        us.Invert();
        UNIT_ASSERT(!us.Has(0));
        UNIT_ASSERT(!us.Has(10));
        UNIT_ASSERT(!us.Has(32));

        UNIT_ASSERT(us.Has(33));
        UNIT_ASSERT(us.Has(TUnicodeSet::CODEPOINT_HIGH - 1));
        UNIT_ASSERT(!us.Has(TUnicodeSet::CODEPOINT_HIGH));

        us.Set(33, 127).Invert();

        UNIT_ASSERT(us.Has(0));
        UNIT_ASSERT(us.Has(10));
        UNIT_ASSERT(us.Has(32));

        UNIT_ASSERT(!us.Has(33));
        UNIT_ASSERT(!us.Has(100));
        UNIT_ASSERT(!us.Has(127));

        UNIT_ASSERT(us.Has(128));
        UNIT_ASSERT(us.Has(300));
        UNIT_ASSERT(us.Has(TUnicodeSet::CODEPOINT_HIGH - 1));
        UNIT_ASSERT(!us.Has(TUnicodeSet::CODEPOINT_HIGH));

        us.Set(0xFFFF, TUnicodeSet::CODEPOINT_HIGH).Invert();
        UNIT_ASSERT(us.Has(0));
        UNIT_ASSERT(us.Has(0xFFFE));

        UNIT_ASSERT(!us.Has(0xFFFF));
        UNIT_ASSERT(!us.Has(0x10000));
        UNIT_ASSERT(!us.Has(TUnicodeSet::CODEPOINT_HIGH - 1));
        UNIT_ASSERT(!us.Has(TUnicodeSet::CODEPOINT_HIGH));
    }

    Y_UNIT_TEST(ConvertToString) {
        TUnicodeSet us;
        us.Set('a', 'z');
        UNIT_ASSERT_EQUAL_C(TStringBuf("[a-z]"), us.ToString(), ", actual: " << us.ToString());

        us.Set('A', 'A');
        UNIT_ASSERT_EQUAL_C(TStringBuf("[A]"), us.ToString(), ", actual: " << us.ToString());

        us.Set(u"abcduz");
        UNIT_ASSERT_EQUAL_C(TStringBuf("[a-duz]"), us.ToString(), ", actual: " << us.ToString());

        us.Set(0, 'a');
        UNIT_ASSERT_EQUAL_C(TStringBuf("[\\x00-a]"), us.ToString(), ", actual: " << us.ToString());

        us.Clear().Add(' ').Add('\t');
        UNIT_ASSERT_EQUAL_C(TStringBuf("[\\x09\\x20]"), us.ToString(), ", actual: " << us.ToString());

        us.Clear().Invert();
        UNIT_ASSERT_EQUAL_C(TStringBuf("[\\x00-\\U0010FFFF]"), us.ToString(), ", actual: " << us.ToString());

        // Two-char range should be printed without dash
        us.Clear().Add('a', 'b');
        UNIT_ASSERT_EQUAL_C(TStringBuf("[ab]"), us.ToString(), ", actual: " << us.ToString());

        // Special symbols like ^ \ or - should be escaped
        us.Clear().Add(u"^abc-01234\\");
        UNIT_ASSERT_EQUAL_C(TStringBuf("[\\x2D0-4\\x5C\\x5Ea-c]"), us.ToString(), ", actual: " << us.ToString());
    }

    Y_UNIT_TEST(AddChar) {
        TUnicodeSet us;
        us.Set('a', 'z').Add('A').Add('B').Add('C');
        UNIT_ASSERT_EQUAL_C(TStringBuf("[A-Ca-z]"), us.ToString(), ", actual: " << us.ToString());
        UNIT_ASSERT(us.Has('A') && us.Has('B') && us.Has('d'));
        UNIT_ASSERT(!us.Has('D') && !us.Has('0'));

        us.Set('a', 'z').Add('a').Add('b').Add('z');
        UNIT_ASSERT_EQUAL_C(TStringBuf("[a-z]"), us.ToString(), ", actual: " << us.ToString());
        UNIT_ASSERT(us.Has('a') && us.Has('e') && us.Has('z'));
        UNIT_ASSERT(!us.Has('D') && !us.Has('0'));

        us.Set('b', 'z').Add('a');
        UNIT_ASSERT_EQUAL_C(TStringBuf("[a-z]"), us.ToString(), ", actual: " << us.ToString());
        UNIT_ASSERT(us.Has('a') && us.Has('b') && us.Has('z'));
        UNIT_ASSERT(!us.Has('D') && !us.Has('0'));

        us.Set('a', 'y').Add('z');
        UNIT_ASSERT_EQUAL_C(TStringBuf("[a-z]"), us.ToString(), ", actual: " << us.ToString());
        UNIT_ASSERT(us.Has('a') && us.Has('y') && us.Has('z'));
        UNIT_ASSERT(!us.Has('D') && !us.Has('0'));

        us.Set(1, 'z').Add(0);
        UNIT_ASSERT_EQUAL_C(TStringBuf("[\\x00-z]"), us.ToString(), ", actual: " << us.ToString());
        UNIT_ASSERT(us.Has('a') && us.Has('y') && us.Has('z') && us.Has(0) && us.Has(9));
        UNIT_ASSERT(!us.Has('z' + 1));

        us.Set('a', TUnicodeSet::CODEPOINT_HIGH).Add(TUnicodeSet::CODEPOINT_HIGH - 1);
        UNIT_ASSERT_EQUAL_C(TStringBuf("[a-\\U0010FFFF]"), us.ToString(), ", actual: " << us.ToString());
        UNIT_ASSERT(us.Has('a') && us.Has('y') && us.Has('z') && us.Has(TUnicodeSet::CODEPOINT_HIGH - 1));
        UNIT_ASSERT(!us.Has(0) && !us.Has(9));

        us.Set('a', TUnicodeSet::CODEPOINT_HIGH - 1).Add(TUnicodeSet::CODEPOINT_HIGH - 1);
        UNIT_ASSERT_EQUAL_C(TStringBuf("[a-\\U0010FFFF]"), us.ToString(), ", actual: " << us.ToString());
        UNIT_ASSERT(us.Has('a') && us.Has('y') && us.Has('z') && us.Has(TUnicodeSet::CODEPOINT_HIGH - 1));
        UNIT_ASSERT(!us.Has(0) && !us.Has(9));

        us.Set('a', TUnicodeSet::CODEPOINT_HIGH - 2).Add(TUnicodeSet::CODEPOINT_HIGH - 1);
        UNIT_ASSERT_EQUAL_C(TStringBuf("[a-\\U0010FFFF]"), us.ToString(), ", actual: " << us.ToString());
        UNIT_ASSERT(us.Has('a') && us.Has('y') && us.Has('z') && us.Has(TUnicodeSet::CODEPOINT_HIGH - 1));
        UNIT_ASSERT(!us.Has(0) && !us.Has(9));

        us.Set('a', 'f').Add('k', 'z').Add('i');
        UNIT_ASSERT_EQUAL_C(TStringBuf("[a-fik-z]"), us.ToString(), ", actual: " << us.ToString());
        UNIT_ASSERT(us.Has('a') && us.Has('f') && us.Has('i') && us.Has('k') && us.Has('l') && us.Has('z'));
        UNIT_ASSERT(!us.Has('A') && !us.Has('g') && !us.Has('j') && !us.Has('~'));

        us.Set('a', 'f').Add('k', 'z').Add('g');
        UNIT_ASSERT_EQUAL_C(TStringBuf("[a-gk-z]"), us.ToString(), ", actual: " << us.ToString());
        UNIT_ASSERT(us.Has('a') && us.Has('f') && us.Has('g') && us.Has('k') && us.Has('l') && us.Has('z'));
        UNIT_ASSERT(!us.Has('A') && !us.Has('j') && !us.Has('~'));

        us.Set('a', 'f').Add('k', 'z').Add('j');
        UNIT_ASSERT_EQUAL_C(TStringBuf("[a-fj-z]"), us.ToString(), ", actual: " << us.ToString());
        UNIT_ASSERT(us.Has('a') && us.Has('f') && us.Has('k') && us.Has('j') && us.Has('l') && us.Has('z'));
        UNIT_ASSERT(!us.Has('A') && !us.Has('g') && !us.Has('~'));

        us.Set('a', 'f').Add('h', 'z').Add('g');
        UNIT_ASSERT_EQUAL_C(TStringBuf("[a-z]"), us.ToString(), ", actual: " << us.ToString());
        UNIT_ASSERT(us.Has('a') && us.Has('e') && us.Has('z'));
        UNIT_ASSERT(!us.Has('D') && !us.Has('0'));
    }

    Y_UNIT_TEST(AddRange) {
        TUnicodeSet us;

        // [[oo]  [XX]  [oo] T]
        us.Set('a', 'd').Add('x', 'z').Add('g', 'k');
        UNIT_ASSERT_EQUAL_C(TStringBuf("[a-dg-kx-z]"), us.ToString(), ", actual: " << us.ToString());

        // [[ooXX]  [oo] T]
        us.Set('a', 'd').Add('x', 'z').Add('e', 'j');
        UNIT_ASSERT_EQUAL_C(TStringBuf("[a-jx-z]"), us.ToString(), ", actual: " << us.ToString());

        // [[oo]  [XXoo] T]
        us.Set('a', 'd').Add('x', 'z').Add('p', 'w');
        UNIT_ASSERT_EQUAL_C(TStringBuf("[a-dp-z]"), us.ToString(), ", actual: " << us.ToString());

        // [[XX] [oo] [oo] T]
        us.Set('x', 'z').Add('g', 'k').Add('a', 'd');
        UNIT_ASSERT_EQUAL_C(TStringBuf("[a-dg-kx-z]"), us.ToString(), ", actual: " << us.ToString());

        // [[oo] [oo] [XX]T]
        us.Set('a', 'd').Add('x', 'z').Add('~', TUnicodeSet::CODEPOINT_HIGH - 1);
        UNIT_ASSERT_EQUAL_C(TStringBuf("[a-dx-z~-\\U0010FFFF]"), us.ToString(), ", actual: " << us.ToString());

        // [[ooXXoo] T]
        us.Set('a', 'z').Add('g', 'k');
        UNIT_ASSERT_EQUAL_C(TStringBuf("[a-z]"), us.ToString(), ", actual: " << us.ToString());

        // [[oo][XX][oo] T]
        us.Set('a', 'f').Add('l', 'z').Add('g', 'k');
        UNIT_ASSERT_EQUAL_C(TStringBuf("[a-z]"), us.ToString(), ", actual: " << us.ToString());

        // [[oo][XX]T]
        us.Set('a', 'y').Add('z', TUnicodeSet::CODEPOINT_HIGH - 1);
        UNIT_ASSERT_EQUAL_C(TStringBuf("[a-\\U0010FFFF]"), us.ToString(), ", actual: " << us.ToString());

        // [   [ooo] [ooo] T]
        //   [XXXXXXXXX]
        us.Set('d', 'f').Add('o', 'r').Add('a', 'p');
        UNIT_ASSERT_EQUAL_C(TStringBuf("[a-r]"), us.ToString(), ", actual: " << us.ToString());

        // [   [ooo] [ooo] T]
        //      [XXXXXXX]
        us.Set('d', 'f').Add('o', 'r').Add('e', 'p');
        UNIT_ASSERT_EQUAL_C(TStringBuf("[d-r]"), us.ToString(), ", actual: " << us.ToString());

        // [ [ooo] [ooo]  T]
        //    [XXXXXXXXX]
        us.Set('d', 'f').Add('o', 'r').Add('e', 't');
        UNIT_ASSERT_EQUAL_C(TStringBuf("[d-t]"), us.ToString(), ", actual: " << us.ToString());

        // [  [ooo] [ooo]  T]
        //   [XXXXXXXXXXX]
        us.Set('d', 'f').Add('o', 'r').Add('b', 't');
        UNIT_ASSERT_EQUAL_C(TStringBuf("[b-t]"), us.ToString(), ", actual: " << us.ToString());

        // [  [ooo] [ooo]  T]
        //   [XXXXXX]
        us.Set('d', 'f').Add('o', 'r').Add('a', 'n');
        UNIT_ASSERT_EQUAL_C(TStringBuf("[a-r]"), us.ToString(), ", actual: " << us.ToString());

        // [  [ooo] [ooo]  T]
        //   [XXXXXXXXXXXX]
        us.Set('d', 'f').Add('o', 'r').Add('a', TUnicodeSet::CODEPOINT_HIGH - 1);
        UNIT_ASSERT_EQUAL_C(TStringBuf("[a-\\U0010FFFF]"), us.ToString(), ", actual: " << us.ToString());

        // [ [ooo] [ooo]  T]
        //       [XXXXXX]
        us.Set('b', 'f').Add('o', 'r').Add('g', 't');
        UNIT_ASSERT_EQUAL_C(TStringBuf("[b-t]"), us.ToString(), ", actual: " << us.ToString());

        // [ [ooo] [ooo]  T]
        //       [XXXXXXX]
        us.Set('b', 'f').Add('o', 'r').Add('g', TUnicodeSet::CODEPOINT_HIGH - 1);
        UNIT_ASSERT_EQUAL_C(TStringBuf("[b-\\U0010FFFF]"), us.ToString(), ", actual: " << us.ToString());
    }

    Y_UNIT_TEST(MixedSet) {
        UNIT_ASSERT_EQUAL(TUnicodeSet().AddCategory(TStringBuf("Z")).Add('a', 'z'), TUnicodeSet().Add('a', 'z').AddCategory(TStringBuf("Z")));
        UNIT_ASSERT_EQUAL(TUnicodeSet().AddCategory(TStringBuf("Z")).Add(' '), TUnicodeSet().Add(' ').AddCategory(TStringBuf("Z")));
        UNIT_ASSERT_EQUAL(TUnicodeSet().Add(Nd_DIGIT).Add(Nl_LETTER), TUnicodeSet().Add(Nl_LETTER).Add(Nd_DIGIT));
    }

    Y_UNIT_TEST(AddCategory) {
        TUnicodeSet us(Ll_LOWER);
        us.Add(Lu_UPPER);
        us.Add(Lt_TITLE);
        const TUtf16String lowerSet = u"abcdezабвгдеяαβγδεω";
        const TUtf16String upperSet = u"ABCDEZАБВГДЕЯΑΒΓΔΕΩ";
        const TUtf16String nonletterSet = u"0123456!@#&^().,";
        for (size_t i = 0; i < lowerSet.length(); ++i) {
            UNIT_ASSERT_C(us.Has(lowerSet[i]), ",char='" << WideToUTF8(~lowerSet + i, 1) << "'");
        }
        for (size_t i = 0; i < upperSet.length(); ++i) {
            UNIT_ASSERT_C(us.Has(upperSet[i]), ",char='" << WideToUTF8(~upperSet + i, 1) << "'");
        }
        for (size_t i = 0; i < nonletterSet.length(); ++i) {
            UNIT_ASSERT_C(!us.Has(nonletterSet[i]), ",char='" << WideToUTF8(~nonletterSet + i, 1) << "'");
        }
        UNIT_ASSERT(us.Has(0x000001C5)); // ǅ

        us.Clear();
        us.Add('a');
        UNIT_ASSERT_EXCEPTION(us.AddCategory(TStringBuf("unknown")), yexception);
        UNIT_ASSERT_EQUAL(us, TUnicodeSet('a', 'a'));

        us.AddCategory(TStringBuf("L"));
        for (size_t i = 0; i < lowerSet.length(); ++i) {
            UNIT_ASSERT_C(us.Has(lowerSet[i]), ",char='" << WideToUTF8(~lowerSet + i, 1) << "'");
        }
        for (size_t i = 0; i < upperSet.length(); ++i) {
            UNIT_ASSERT_C(us.Has(upperSet[i]), ",char='" << WideToUTF8(~upperSet + i, 1) << "'");
        }
        for (size_t i = 0; i < nonletterSet.length(); ++i) {
            UNIT_ASSERT_C(!us.Has(nonletterSet[i]), ",char='" << WideToUTF8(~nonletterSet + i, 1) << "'");
        }
        UNIT_ASSERT(us.Has(0x000001C5)); // ǅ
    }

    Y_UNIT_TEST(MakeCaseInsensitive) {
        ::TestCase_MakeCaseInsensitive(Lu_UPPER);
        ::TestCase_MakeCaseInsensitive(Ll_LOWER);

        UNIT_ASSERT_EQUAL(TUnicodeSet(Nd_DIGIT).MakeCaseInsensitive(), TUnicodeSet(Nd_DIGIT));
    }

    Y_UNIT_TEST(Copy) {
        TUnicodeSet us(123, 487);
        TUnicodeSet usCopy = us; // Short buffer
        UNIT_ASSERT_EQUAL(us, usCopy);
        us.Add(488, 488);
        UNIT_ASSERT_UNEQUAL(us, usCopy);

        us.Set(u"abcdef9123456789!@#$%^&*(\"");
        usCopy = us; // Dynamic buffer
        UNIT_ASSERT_EQUAL(us, usCopy);
        us.Add('g', 'g');
        UNIT_ASSERT_UNEQUAL(us, usCopy);

        us.Set(Ll_LOWER);
        usCopy = us; // Static buffer
        UNIT_ASSERT_EQUAL(us, usCopy);
        us.Add('`', '`');
        UNIT_ASSERT_UNEQUAL(us, usCopy);
    }

    Y_UNIT_TEST(Parse) {
        TUnicodeSet us;

        // Empty set
        UNIT_ASSERT_EQUAL(us.Parse(u"[]"), TUnicodeSet());

        // Sequences and ranges
        UNIT_ASSERT_EQUAL_C(us.Parse(u"[aswz]").ToString(), TStringBuf("[aswz]"), ", actual: " << us.ToString());
        UNIT_ASSERT_EQUAL_C(us.Parse(u"[abcd]").ToString(), TStringBuf("[a-d]"), ", actual: " << us.ToString());
        UNIT_ASSERT_EQUAL_C(us.Parse(u"[ace-hA-Z]").ToString(), TStringBuf("[A-Zace-h]"), ", actual: " << us.ToString());

        // Negation
        UNIT_ASSERT_EQUAL_C(us.Parse(u"[^a-z]").ToString(), TStringBuf("[\\x00-`{-\\U0010FFFF]"), ", actual: " << us.ToString());
        UNIT_ASSERT_EQUAL_C(us.Parse(u"[a-z^]").ToString(), TStringBuf("[\\x5Ea-z]"), ", actual: " << us.ToString());

        // Escapes
        UNIT_ASSERT_EQUAL_C(us.Parse(u"[\\a\\b\\t\\n\\v\\f\\r]").ToString(), TStringBuf("[\\x07-\\x0D]"), ", actual: " << us.ToString());
        UNIT_ASSERT_EQUAL_C(us.Parse(u"[%a%b%t%n%v%f%r]").ToString(), TStringBuf("[\\x07-\\x0D]"), ", actual: " << us.ToString());
        UNIT_ASSERT_EQUAL_C(us.Parse(u"[\a\b\t\n\v\f\r]").ToString(), TStringBuf("[\\x07-\\x0D]"), ", actual: " << us.ToString());
        UNIT_ASSERT_EQUAL_C(us.Parse(u"[\\^\\-\\\\\\%\\]]").ToString(), TStringBuf("[%\\x2D\\x5C-\\x5E]"), ", actual: " << us.ToString());
        UNIT_ASSERT_EQUAL_C(us.Parse(u"[%^%-%\\%%%]]").ToString(), TStringBuf("[%\\x2D\\x5C-\\x5E]"), ", actual: " << us.ToString());

        // Code points
        UNIT_ASSERT_EQUAL_C(us.Parse(u"[\\x13\x17]").ToString(), TStringBuf("[\\x13\\x17]"), ", actual: " << us.ToString());
        UNIT_ASSERT_EQUAL_C(us.Parse(u"[%x13\x17]").ToString(), TStringBuf("[\\x13\\x17]"), ", actual: " << us.ToString());
        UNIT_ASSERT_EQUAL_C(us.Parse(u"[\\uFF45]").ToString(true), TStringBuf("[\\uFF45]"), ", actual: " << us.ToString(true));
        UNIT_ASSERT_EQUAL_C(us.Parse(u"[%uFF45]").ToString(true), TStringBuf("[\\uFF45]"), ", actual: " << us.ToString(true));
        UNIT_ASSERT_EQUAL_C(us.Parse(u"[\\uFF45-\\uFF90]").ToString(true), TStringBuf("[\\uFF45-\\uFF90]"), ", actual: " << us.ToString(true));
        UNIT_ASSERT_EQUAL_C(us.Parse(u"[%uFF45-%uFF90]").ToString(true), TStringBuf("[\\uFF45-\\uFF90]"), ", actual: " << us.ToString(true));
        UNIT_ASSERT_EQUAL_C(us.Parse(u"[\\U0010FF00-\\U0010FFFF]").ToString(true), TStringBuf("[\\U0010FF00-\\U0010FFFF]"), ", actual: " << us.ToString(true));
        UNIT_ASSERT_EQUAL_C(us.Parse(u"[%U0010FF00-%U0010FFFF]").ToString(true), TStringBuf("[\\U0010FF00-\\U0010FFFF]"), ", actual: " << us.ToString(true));

        // Categories
        UNIT_ASSERT_EQUAL(us.Parse(u"[:L:]"), TUnicodeSet().AddCategory(TStringBuf("L")));
        UNIT_ASSERT_EQUAL(us.Parse(u"[:Ll:]"), TUnicodeSet().AddCategory(TStringBuf("Ll")));
        UNIT_ASSERT_EQUAL(us.Parse(u"[:Ll:]"), TUnicodeSet().AddCategory(TStringBuf("Ll_LOWER")));
        UNIT_ASSERT_EQUAL(us.Parse(u"[:Ll:]"), TUnicodeSet().Add(Ll_LOWER));
        UNIT_ASSERT_EQUAL(us.Parse(u"[:Ll_LOWER:]"), TUnicodeSet().AddCategory(TStringBuf("Ll")));
        UNIT_ASSERT_EQUAL(us.Parse(u"[:Ll_LOWER:]"), TUnicodeSet().AddCategory(TStringBuf("Ll_LOWER")));
        UNIT_ASSERT_EQUAL(us.Parse(u"[:Ll_LOWER:]"), TUnicodeSet().Add(Ll_LOWER));
        UNIT_ASSERT_EQUAL(us.Parse(u"[:Cs:]"), TUnicodeSet().AddCategory(TStringBuf("Cs")));
        UNIT_ASSERT_EQUAL(us.Parse(u"[:Cs:]"), TUnicodeSet().AddCategory(TStringBuf("Cs_LOW")).AddCategory(TStringBuf("Cs_HIGH")));
        UNIT_ASSERT_EQUAL(us.Parse(u"[:Cs:]"), TUnicodeSet().Add(Cs_LOW).Add(Cs_HIGH));
        UNIT_ASSERT_EQUAL(us.Parse(u"[[:Cs_LOW:][:Cs_HIGH:]]"), TUnicodeSet().AddCategory(TStringBuf("Cs")));
        UNIT_ASSERT_EQUAL(us.Parse(u"[[:Cs_LOW:][:Cs_HIGH:]]"), TUnicodeSet().AddCategory(TStringBuf("Cs_LOW")).AddCategory(TStringBuf("Cs_HIGH")));
        UNIT_ASSERT_EQUAL(us.Parse(u"[[:Cs_LOW:][:Cs_HIGH:]]"), TUnicodeSet().Add(Cs_LOW).Add(Cs_HIGH));
        UNIT_ASSERT_EQUAL(us.Parse(u"[^:Mn:]"), TUnicodeSet().AddCategory(TStringBuf("Mn")).Invert());

        // Category shortcuts
        UNIT_ASSERT_EQUAL(us.Parse(u"[\\w]"), TUnicodeSet().AddCategory(TStringBuf("L")));
        UNIT_ASSERT_EQUAL(us.Parse(u"[%w]"), TUnicodeSet().AddCategory(TStringBuf("L")));
        UNIT_ASSERT_EQUAL(us.Parse(u"[\\W]"), TUnicodeSet().AddCategory(TStringBuf("L")).Invert());
        UNIT_ASSERT_EQUAL(us.Parse(u"[%W]"), TUnicodeSet().AddCategory(TStringBuf("L")).Invert());
        UNIT_ASSERT_EQUAL(us.Parse(u"[\\s]"), TUnicodeSet().AddCategory(TStringBuf("Z")));
        UNIT_ASSERT_EQUAL(us.Parse(u"[%s]"), TUnicodeSet().AddCategory(TStringBuf("Z")));
        UNIT_ASSERT_EQUAL(us.Parse(u"[\\S]"), TUnicodeSet().AddCategory(TStringBuf("Z")).Invert());
        UNIT_ASSERT_EQUAL(us.Parse(u"[%S]"), TUnicodeSet().AddCategory(TStringBuf("Z")).Invert());
        UNIT_ASSERT_EQUAL(us.Parse(u"[\\d]"), TUnicodeSet().AddCategory(TStringBuf("Nd")));
        UNIT_ASSERT_EQUAL(us.Parse(u"[%d]"), TUnicodeSet().AddCategory(TStringBuf("Nd")));
        UNIT_ASSERT_EQUAL(us.Parse(u"[\\d]"), TUnicodeSet().Add(Nd_DIGIT));
        UNIT_ASSERT_EQUAL(us.Parse(u"[%d]"), TUnicodeSet().Add(Nd_DIGIT));
        UNIT_ASSERT_EQUAL(us.Parse(u"[\\D]"), TUnicodeSet().AddCategory(TStringBuf("Nd")).Invert());
        UNIT_ASSERT_EQUAL(us.Parse(u"[%D]"), TUnicodeSet().AddCategory(TStringBuf("Nd")).Invert());
        UNIT_ASSERT_EQUAL(us.Parse(u"[\\D]"), TUnicodeSet().Add(Nd_DIGIT).Invert());
        UNIT_ASSERT_EQUAL(us.Parse(u"[%D]"), TUnicodeSet().Add(Nd_DIGIT).Invert());

        // Inner sets
        UNIT_ASSERT_EQUAL(us.Parse(u"[a-z[:Z:]]"), TUnicodeSet().AddCategory(TStringBuf("Z")).Add('a', 'z'));
        UNIT_ASSERT_EQUAL(us.Parse(u"[^a-z[:Z:]]"), TUnicodeSet().AddCategory(TStringBuf("Z")).Add('a', 'z').Invert());
        UNIT_ASSERT_EQUAL(us.Parse(u"[a-z[^:Z:]]"), TUnicodeSet().AddCategory(TStringBuf("Z")).Invert().Add('a', 'z'));
        UNIT_ASSERT_EQUAL(us.Parse(u"[a-z[:Z:]0-9]"), TUnicodeSet().AddCategory(TStringBuf("Z")).Add('a', 'z').Add('0', '9'));
        UNIT_ASSERT_EQUAL(us.Parse(u"[[:Ll:][:Nd:]]"), TUnicodeSet().AddCategory(TStringBuf("Ll")).AddCategory(TStringBuf("Nd")));
        UNIT_ASSERT_EQUAL(us.Parse(u"[^[:Ll:][:Nd:]]"), TUnicodeSet().AddCategory(TStringBuf("Ll")).AddCategory(TStringBuf("Nd")).Invert());
        UNIT_ASSERT_EQUAL(us.Parse(u"[[^:Ll:][:Nd:]]"), TUnicodeSet().AddCategory(TStringBuf("Ll")).Invert().AddCategory(TStringBuf("Nd")));

        // Bad syntax
        UNIT_ASSERT_EXCEPTION(us.Parse(u""), yexception);
        UNIT_ASSERT_EXCEPTION(us.Parse(u"abc"), yexception);
        UNIT_ASSERT_EXCEPTION(us.Parse(u"[abc"), yexception);
        UNIT_ASSERT_EXCEPTION(us.Parse(u"abc]"), yexception);
        UNIT_ASSERT_EXCEPTION(us.Parse(u"[-a]"), yexception);
        UNIT_ASSERT_EXCEPTION(us.Parse(u"[a-]"), yexception);
        UNIT_ASSERT_EXCEPTION(us.Parse(u"[a-e-z]"), yexception);
        UNIT_ASSERT_EXCEPTION(us.Parse(u"[a-[:L:]]"), yexception);
        UNIT_ASSERT_EXCEPTION(us.Parse(u"[[:L:]-z]"), yexception);
        UNIT_ASSERT_EXCEPTION(us.Parse(u"[z-a]"), yexception);
        UNIT_ASSERT_EXCEPTION(us.Parse(u"[:A:]"), yexception);
        UNIT_ASSERT_EXCEPTION(us.Parse(u"[::]"), yexception);
        UNIT_ASSERT_EXCEPTION(us.Parse(u"[:123:]"), yexception);
        UNIT_ASSERT_EXCEPTION(us.Parse(u"[[:Z:]"), yexception);
        UNIT_ASSERT_EXCEPTION(us.Parse(u"[\\xWW]"), yexception);
        UNIT_ASSERT_EXCEPTION(us.Parse(u"[\\x1]"), yexception);
        UNIT_ASSERT_EXCEPTION(us.Parse(u"[\\u123G]"), yexception);
        UNIT_ASSERT_EXCEPTION(us.Parse(u"[\\u12]"), yexception);
        UNIT_ASSERT_EXCEPTION(us.Parse(u"[\\U1234567W]"), yexception);
        UNIT_ASSERT_EXCEPTION(us.Parse(u"[\\U12345]"), yexception);
        UNIT_ASSERT_EXCEPTION(us.Parse(u"[\\U00110000]"), yexception);
        UNIT_ASSERT_EXCEPTION(us.Parse(u"[\\U00110001]"), yexception);
        UNIT_ASSERT_EXCEPTION(us.Parse(u"\\"), yexception);
        UNIT_ASSERT_EXCEPTION(us.Parse(u"%"), yexception);
        UNIT_ASSERT_EXCEPTION(us.Parse(u"[\\]"), yexception);
        UNIT_ASSERT_EXCEPTION(us.Parse(u"[%]"), yexception);
    }

    Y_UNIT_TEST(Categories) {
        TVector<TUnicodeSet> uss;
        for (size_t cat = 0; cat < CCL_NUM; ++cat) {
            uss.push_back(TUnicodeSet(static_cast<WC_TYPE>(cat)));
        }
        for (wchar32 c = 0; c < TUnicodeSet::CODEPOINT_HIGH; ++c) {
            WC_TYPE cat = NUnicode::CharType(c);
            for (size_t us_cat = 0; us_cat < CCL_NUM; ++us_cat) {
                UNIT_ASSERT_EQUAL_C(uss[static_cast<size_t>(us_cat)].Has(c), us_cat == cat,
                                    ", for category " << static_cast<size_t>(cat) << ", for U+" << ::Hex(c) << " " << ::UTF32ToWide(&c, 1u));
            }
        }
    }

    Y_UNIT_TEST(CategoriesNamesBase) {
        UNIT_ASSERT_EQUAL(TUnicodeSet(Cn_UNASSIGNED), TUnicodeSet().AddCategory("Cn_UNASSIGNED"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Lu_UPPER), TUnicodeSet().AddCategory("Lu_UPPER"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Ll_LOWER), TUnicodeSet().AddCategory("Ll_LOWER"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Lt_TITLE), TUnicodeSet().AddCategory("Lt_TITLE"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Lm_EXTENDER), TUnicodeSet().AddCategory("Lm_EXTENDER"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Lm_LETTER), TUnicodeSet().AddCategory("Lm_LETTER"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Lo_OTHER), TUnicodeSet().AddCategory("Lo_OTHER"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Lo_IDEOGRAPH), TUnicodeSet().AddCategory("Lo_IDEOGRAPH"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Lo_KATAKANA), TUnicodeSet().AddCategory("Lo_KATAKANA"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Lo_HIRAGANA), TUnicodeSet().AddCategory("Lo_HIRAGANA"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Lo_LEADING), TUnicodeSet().AddCategory("Lo_LEADING"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Lo_VOWEL), TUnicodeSet().AddCategory("Lo_VOWEL"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Lo_TRAILING), TUnicodeSet().AddCategory("Lo_TRAILING"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Mn_NONSPACING), TUnicodeSet().AddCategory("Mn_NONSPACING"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Me_ENCLOSING), TUnicodeSet().AddCategory("Me_ENCLOSING"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Mc_SPACING), TUnicodeSet().AddCategory("Mc_SPACING"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Nd_DIGIT), TUnicodeSet().AddCategory("Nd_DIGIT"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Nl_LETTER), TUnicodeSet().AddCategory("Nl_LETTER"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Nl_IDEOGRAPH), TUnicodeSet().AddCategory("Nl_IDEOGRAPH"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(No_OTHER), TUnicodeSet().AddCategory("No_OTHER"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Zs_SPACE), TUnicodeSet().AddCategory("Zs_SPACE"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Zs_ZWSPACE), TUnicodeSet().AddCategory("Zs_ZWSPACE"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Zl_LINE), TUnicodeSet().AddCategory("Zl_LINE"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Zp_PARAGRAPH), TUnicodeSet().AddCategory("Zp_PARAGRAPH"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Cc_ASCII), TUnicodeSet().AddCategory("Cc_ASCII"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Cc_SPACE), TUnicodeSet().AddCategory("Cc_SPACE"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Cc_SEPARATOR), TUnicodeSet().AddCategory("Cc_SEPARATOR"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Cf_FORMAT), TUnicodeSet().AddCategory("Cf_FORMAT"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Cf_JOIN), TUnicodeSet().AddCategory("Cf_JOIN"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Cf_BIDI), TUnicodeSet().AddCategory("Cf_BIDI"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Cf_ZWNBSP), TUnicodeSet().AddCategory("Cf_ZWNBSP"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Cs_LOW), TUnicodeSet().AddCategory("Cs_LOW"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Cs_HIGH), TUnicodeSet().AddCategory("Cs_HIGH"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Pd_DASH), TUnicodeSet().AddCategory("Pd_DASH"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Pd_HYPHEN), TUnicodeSet().AddCategory("Pd_HYPHEN"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Ps_START), TUnicodeSet().AddCategory("Ps_START"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Ps_QUOTE), TUnicodeSet().AddCategory("Ps_QUOTE"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Pe_END), TUnicodeSet().AddCategory("Pe_END"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Pe_QUOTE), TUnicodeSet().AddCategory("Pe_QUOTE"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Pi_QUOTE), TUnicodeSet().AddCategory("Pi_QUOTE"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Pf_QUOTE), TUnicodeSet().AddCategory("Pf_QUOTE"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Pc_CONNECTOR), TUnicodeSet().AddCategory("Pc_CONNECTOR"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Po_OTHER), TUnicodeSet().AddCategory("Po_OTHER"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Po_QUOTE), TUnicodeSet().AddCategory("Po_QUOTE"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Po_TERMINAL), TUnicodeSet().AddCategory("Po_TERMINAL"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Po_EXTENDER), TUnicodeSet().AddCategory("Po_EXTENDER"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Po_HYPHEN), TUnicodeSet().AddCategory("Po_HYPHEN"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Sm_MATH), TUnicodeSet().AddCategory("Sm_MATH"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Sm_MINUS), TUnicodeSet().AddCategory("Sm_MINUS"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Sc_CURRENCY), TUnicodeSet().AddCategory("Sc_CURRENCY"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Sk_MODIFIER), TUnicodeSet().AddCategory("Sk_MODIFIER"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(So_OTHER), TUnicodeSet().AddCategory("So_OTHER"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Ps_SINGLE_QUOTE), TUnicodeSet().AddCategory("Ps_SINGLE_QUOTE"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Pe_SINGLE_QUOTE), TUnicodeSet()); // empty
        UNIT_ASSERT_EQUAL(TUnicodeSet(Pi_SINGLE_QUOTE), TUnicodeSet().AddCategory("Pi_SINGLE_QUOTE"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Pf_SINGLE_QUOTE), TUnicodeSet().AddCategory("Pf_SINGLE_QUOTE"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Po_SINGLE_QUOTE), TUnicodeSet().AddCategory("Po_SINGLE_QUOTE"));
    }

    Y_UNIT_TEST(CategoriesNamesAliases) {
        UNIT_ASSERT_EQUAL(TUnicodeSet(Co_PRIVATE), TUnicodeSet().AddCategory("Co_PRIVATE"));

        UNIT_ASSERT_EQUAL(TUnicodeSet(Cn_UNASSIGNED), TUnicodeSet().AddCategory("Cn"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Co_PRIVATE), TUnicodeSet().AddCategory("Co"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Lu_UPPER), TUnicodeSet().AddCategory("Lu"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Ll_LOWER), TUnicodeSet().AddCategory("Ll"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Lt_TITLE), TUnicodeSet().AddCategory("Lt"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Lm_EXTENDER).Add(Lm_LETTER), TUnicodeSet().AddCategory("Lm"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Lo_OTHER).Add(Lo_IDEOGRAPH).Add(Lo_KATAKANA).Add(Lo_HIRAGANA).Add(Lo_LEADING).Add(Lo_VOWEL).Add(Lo_TRAILING), TUnicodeSet().AddCategory("Lo"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Mn_NONSPACING), TUnicodeSet().AddCategory("Mn"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Me_ENCLOSING), TUnicodeSet().AddCategory("Me"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Mc_SPACING), TUnicodeSet().AddCategory("Mc"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Nd_DIGIT), TUnicodeSet().AddCategory("Nd"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Nl_LETTER).Add(Nl_IDEOGRAPH), TUnicodeSet().AddCategory("Nl"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(No_OTHER), TUnicodeSet().AddCategory("No"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Zs_SPACE).Add(Zs_ZWSPACE), TUnicodeSet().AddCategory("Zs"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Zl_LINE), TUnicodeSet().AddCategory("Zl"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Zp_PARAGRAPH), TUnicodeSet().AddCategory("Zp"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Cc_ASCII).Add(Cc_SPACE).Add(Cc_SEPARATOR), TUnicodeSet().AddCategory("Cc"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Cf_FORMAT).Add(Cf_JOIN).Add(Cf_BIDI).Add(Cf_ZWNBSP), TUnicodeSet().AddCategory("Cf"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Cs_LOW).Add(Cs_HIGH), TUnicodeSet().AddCategory("Cs"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Pd_DASH).Add(Pd_HYPHEN), TUnicodeSet().AddCategory("Pd"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Ps_START).Add(Ps_QUOTE).Add(Ps_SINGLE_QUOTE), TUnicodeSet().AddCategory("Ps"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Pe_END).Add(Pe_QUOTE).Add(Pe_SINGLE_QUOTE), TUnicodeSet().AddCategory("Pe"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Pi_QUOTE).Add(Pi_SINGLE_QUOTE), TUnicodeSet().AddCategory("Pi"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Pf_QUOTE).Add(Pf_SINGLE_QUOTE), TUnicodeSet().AddCategory("Pf"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Pc_CONNECTOR), TUnicodeSet().AddCategory("Pc"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Po_OTHER).Add(Po_QUOTE).Add(Po_TERMINAL).Add(Po_EXTENDER).Add(Po_HYPHEN).Add(Po_SINGLE_QUOTE), TUnicodeSet().AddCategory("Po"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Sm_MATH).Add(Sm_MINUS), TUnicodeSet().AddCategory("Sm"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Sc_CURRENCY), TUnicodeSet().AddCategory("Sc"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Sk_MODIFIER), TUnicodeSet().AddCategory("Sk"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(So_OTHER), TUnicodeSet().AddCategory("So"));

        UNIT_ASSERT_EQUAL(TUnicodeSet(Cn_UNASSIGNED).Add(Co_PRIVATE).Add(Cc_ASCII).Add(Cc_SPACE).Add(Cc_SEPARATOR).Add(Cf_FORMAT).Add(Cf_JOIN).Add(Cf_BIDI).Add(Cf_ZWNBSP).Add(Cs_LOW).Add(Cs_HIGH), TUnicodeSet().AddCategory("C"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Lu_UPPER).Add(Ll_LOWER).Add(Lt_TITLE).Add(Lm_EXTENDER).Add(Lm_LETTER).Add(Lo_OTHER).Add(Lo_IDEOGRAPH).Add(Lo_KATAKANA).Add(Lo_HIRAGANA).Add(Lo_LEADING).Add(Lo_VOWEL).Add(Lo_TRAILING), TUnicodeSet().AddCategory("L"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Mn_NONSPACING).Add(Me_ENCLOSING).Add(Mc_SPACING), TUnicodeSet().AddCategory("M"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Nd_DIGIT).Add(Nl_LETTER).Add(Nl_IDEOGRAPH).Add(No_OTHER), TUnicodeSet().AddCategory("N"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Zs_SPACE).Add(Zs_ZWSPACE).Add(Zl_LINE).Add(Zp_PARAGRAPH), TUnicodeSet().AddCategory("Z"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Pd_DASH).Add(Pd_HYPHEN).Add(Ps_START).Add(Ps_QUOTE).Add(Ps_SINGLE_QUOTE).Add(Pe_END).Add(Pe_QUOTE).Add(Pe_SINGLE_QUOTE).Add(Pi_QUOTE).Add(Pi_SINGLE_QUOTE).Add(Pf_QUOTE).Add(Pf_SINGLE_QUOTE).Add(Pc_CONNECTOR).Add(Po_OTHER).Add(Po_QUOTE).Add(Po_TERMINAL).Add(Po_EXTENDER).Add(Po_HYPHEN).Add(Po_SINGLE_QUOTE), TUnicodeSet().AddCategory("P"));
        UNIT_ASSERT_EQUAL(TUnicodeSet(Sm_MATH).Add(Sm_MINUS).Add(Sc_CURRENCY).Add(Sk_MODIFIER).Add(So_OTHER), TUnicodeSet().AddCategory("S"));
    }

    Y_UNIT_TEST(CategoriesCoverage) {
        TUnicodeSet us;
        for (size_t cat = 0; cat < CCL_NUM; ++cat) {
            us.Add(static_cast<WC_TYPE>(cat));
        }
        for (wchar32 c = 0; c < TUnicodeSet::CODEPOINT_HIGH; ++c) {
            UNIT_ASSERT_C(us.Has(c), ", for U+" << ::Hex(c) << " " << ::UTF32ToWide(&c, 1u));
        }
        UNIT_ASSERT_EQUAL(us, TUnicodeSet(0, TUnicodeSet::CODEPOINT_HIGH));
        UNIT_ASSERT(us.Invert().Empty());
        UNIT_ASSERT(!us.Has(TUnicodeSet::CODEPOINT_HIGH));
    }

    Y_UNIT_TEST(RealEqualityDoubleInvert) {
        ::TestRealEquality(TUnicodeSet(), TUnicodeSet().Invert().Invert());
        ::TestRealEquality(TUnicodeSet(Lu_UPPER), TUnicodeSet(Lu_UPPER).Invert().Invert());
        ::TestRealEquality(TUnicodeSet(u"abc"), TUnicodeSet(u"abc").Invert().Invert());
        ::TestRealEquality(TUnicodeSet(0x0410, 0x044F), TUnicodeSet(0x0410, 0x044F).Invert().Invert());
        ::TestRealEquality(TUnicodeSet().SetCategory("Lu_UPPER"), TUnicodeSet().SetCategory("Lu_UPPER").Invert().Invert());
        ::TestRealEquality(TUnicodeSet().SetCategory("Lu"), TUnicodeSet().SetCategory("Lu").Invert().Invert());
        ::TestRealEquality(TUnicodeSet().SetCategory("L"), TUnicodeSet().SetCategory("L").Invert().Invert());
        ::TestRealEquality(TUnicodeSet().Add(0x0410), TUnicodeSet().Add(0x0410).Invert().Invert());
        ::TestRealEquality(TUnicodeSet().Add(Lu_UPPER).Add(Ll_LOWER).AddCategory("Lt_TITLE").Add(0x0410), TUnicodeSet().Add(Lu_UPPER).Add(Ll_LOWER).AddCategory("Lt_TITLE").Add(0x0410).Invert().Invert());
    }

    Y_UNIT_TEST(RealEqualityCategories) {
        ::TestRealEquality(TUnicodeSet(Lu_UPPER), TUnicodeSet().SetCategory("Lu_UPPER"));
        ::TestRealEquality(TUnicodeSet().Add(Cs_LOW).Add(Cs_HIGH), TUnicodeSet().SetCategory("Cs"));
    }
}
