#include "ffb.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/stream/output.h>
#include <util/string/split.h>
#include <util/generic/vector.h>
#include <util/datetime/cputimer.h>

class LineSplitterTest: public TTestBase {
    UNIT_TEST_SUITE(LineSplitterTest);
    UNIT_TEST(TestOneCharMode);
    UNIT_TEST(TestManyCharsMode);
    UNIT_TEST(TestNonEmptyMode);
    UNIT_TEST(TestIsSpaceMode);
    UNIT_TEST(TestStringMode);
    UNIT_TEST_SUITE_END();

public:
    void TestOneCharMode() {
        TLineSplitter split(str_spn("a"));
        char str[] = "ababa aabbbba";
        const char* res[] = {"", "b", "b", " ", "", "bbbb", ""};
        ui32 n = 7;
        TVector<char*> v;
        split(str, v);

        UNIT_ASSERT_EQUAL(v.size(), n);
        auto i = v.begin(), e = v.end();
        for (ui32 j = 0; i != e && j < n; ++i, ++j)
            UNIT_ASSERT(!strcmp(*i, res[j]));
    }

    void TestManyCharsMode() {
        TLineSplitter split(str_spn("a "));
        char str[] = "ababa aabbbba";
        const char* res[] = {"", "b", "b", "", "", "", "bbbb", ""};
        ui32 n = 8;
        TVector<char*> v;
        split(str, v);

        UNIT_ASSERT_EQUAL(v.size(), n);
        TVector<char*>::iterator i = v.begin(), e = v.end();
        for (ui32 j = 0; i != e && j < n; ++i, ++j)
            UNIT_ASSERT(!strcmp(*i, res[j]));
    }

    void TestNonEmptyMode() {
        TLineSplitter split(str_spn("a "), true);
        char str[] = "ababa aabbbba";
        const char* res[] = {"b", "b", "bbbb"};
        ui32 n = 3;
        TVector<char*> v;
        split(str, v);

        UNIT_ASSERT_EQUAL(v.size(), n);
        TVector<char*>::iterator i = v.begin(), e = v.end();
        for (ui32 j = 0; i != e && j < n; ++i, ++j)
            UNIT_ASSERT(!strcmp(*i, res[j]));
    }

    void TestIsSpaceMode() {
        TLineSplitter split(str_spn("\t\n\v\f\r "), true);
        char str[] = "\t \n ab aba aa\tbb\nb\vb\r a";
        const char* res[] = {"ab", "aba", "aa", "bb", "b", "b", "a"};
        ui32 n = 7;
        TVector<char*> v;
        split(str, v);

        UNIT_ASSERT_EQUAL(v.size(), n);
        TVector<char*>::iterator i = v.begin(), e = v.end();
        for (ui32 j = 0; i != e && j < n; ++i, ++j)
            UNIT_ASSERT(!strcmp(*i, res[j]));
    }

    void TestStringMode() {
        TLineSplitter split("bb", false);
        char str[] = "bbabbaba aabbbbabb";
        const char* res[] = {"", "a", "aba aa", "", "a", ""};
        ui32 n = 6;
        TVector<char*> v;
        split(str, v);

        UNIT_ASSERT_EQUAL(v.size(), n);
        TVector<char*>::iterator i = v.begin(), e = v.end();
        for (ui32 j = 0; i != e && j < n; ++i, ++j)
            UNIT_ASSERT(!strcmp(*i, res[j]));
    }
};

UNIT_TEST_SUITE_REGISTRATION(LineSplitterTest);

class FileSplitterTest: public TTestBase {
    UNIT_TEST_SUITE(FileSplitterTest);
    UNIT_TEST(TestTwoFieldsLF);
    UNIT_TEST(TestTwoFieldsCRLF);
    UNIT_TEST(TestAnyFieldsLF);
    UNIT_TEST(TestAwkFieldsLF);
    UNIT_TEST_SUITE_END();

public:
    void TestTwoFieldsLF() {
        const char* fname = "test_two_fields_lf.tmp";
        char str[] = "opqiuwhep\tiufhp\nqio\teuhlq\n\twre\n\t\nquwehofi\t\n";
        const char* res[] = {"opqiuwhep", "iufhp", "qio", "euhlq", "", "wre", "", "", "quwehofi", ""};
        int n = 10;

        TFILEPtr tmp_file(fname, "w");
        tmp_file.fsput(str, strlen(str));
        tmp_file.close();

        TSFReader rd(fname, "\t", 2);
        int k = 0;
        while (rd.NextLine()) {
            int nf = (int)rd;
            UNIT_ASSERT(k + nf <= n);
            for (int i = 0; i < nf; i++, k++)
                UNIT_ASSERT(!strcmp(rd[i], res[k]));
        }
        unlink(fname);
    }

    void TestTwoFieldsCRLF() {
        const char* fname = "test_two_fields_cr_lf.tmp";
        char str[] = "opqiuwhep\tiufhp\r\nqio\teuhlq\r\n\twre\r\n\r\t\r\nquwehofi\t\r\n";
        const char* res[] = {"opqiuwhep", "iufhp", "qio", "euhlq", "", "wre", "\r", "", "quwehofi", ""};
        int n = 10;

        TFILEPtr tmp_file(fname, "w");
        tmp_file.fsput(str, strlen(str));
        tmp_file.close();

        TSFReader rd(fname, "\t", 2);
        int k = 0;
        while (rd.NextLine()) {
            int nf = (int)rd;
            UNIT_ASSERT(k + nf <= n);
            for (int i = 0; i < nf; i++, k++)
                UNIT_ASSERT(!strcmp(rd[i], res[k]));
        }
        unlink(fname);
    }

    void TestAnyFieldsLF() {
        const char* fname = "test_any_fields_lf.tmp";
        char str[] = "opqiuwhep\tiufhp\nqioeuhlq\n\twre\n\n\t\t\nquwehofi\t\n";
        const char* res[] = {"opqiuwhep", "iufhp", "qioeuhlq", "", "wre", "", "", "", "quwehofi", ""};
        int n = 10;

        TFILEPtr tmp_file(fname, "w");
        tmp_file.fsput(str, strlen(str));
        tmp_file.close();

        TSFReader rd(fname, "\t");
        int k = 0;
        while (rd.NextLine()) {
            int nf = (int)rd;
            UNIT_ASSERT(k + nf <= n);
            for (int i = 0; i < nf; i++, k++)
                UNIT_ASSERT(!strcmp(rd[i], res[k]));
        }
        unlink(fname);
    }
    void TestAwkFieldsLF() {
        const char* fname = "test_awk_fields_lf.tmp";
        char str[] = "opqi\r uwhep\tiuf        hp\nqioeuhlq\n\twre\n\n\t        \t\nquwehofi\t\n";
        const char* res[] = {"opqi", "uwhep", "iuf", "hp", "qioeuhlq", "wre", "quwehofi"};
        int n = 7;

        TFILEPtr tmp_file(fname, "w");
        tmp_file.fsput(str, strlen(str));
        tmp_file.close();

        TSFReader rd(fname, ' '); // Awk-imitation mode (all whitespace characters are separators, skipping all empty tokens)
        int k = 0;
        while (rd.NextLine()) {
            int nf = (int)rd;
            UNIT_ASSERT(k + nf <= n);
            for (int i = 0; i < nf; i++, k++)
                UNIT_ASSERT(!strcmp(rd[i], res[k]));
        }
        unlink(fname);
    }
};

UNIT_TEST_SUITE_REGISTRATION(FileSplitterTest);
