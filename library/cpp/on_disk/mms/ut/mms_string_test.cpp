#include "tools.h"

#include <library/cpp/on_disk/mms/string.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/string.h>
#include <util/string/builder.h>

Y_UNIT_TEST_SUITE(TMmsStringTest) {
    Y_UNIT_TEST(StringTest) {
        size_t maxSize = 1000;
        TMmsObjects objs;
        for (size_t i = 0; i <= maxSize; ++i) {
            TString s(i, 'a');
            UNIT_ASSERT_EQUAL(s, objs.MakeMmappedString(s));
            TString s0(i, '\0');
            UNIT_ASSERT_EQUAL(s0, objs.MakeMmappedString(s0));
        }
        UNIT_ASSERT_EQUAL(NMms::TStringType<NMms::TMmapped>(), TString());
        UNIT_ASSERT_EQUAL(NMms::TStringType<NMms::TMmapped>("abc", 3), TString("abc"));
        UNIT_ASSERT_EXCEPTION(NMms::TStringType<NMms::TMmapped>("abc", 1), yexception);

        UNIT_ASSERT_EQUAL(static_cast<TStringBuf>(NMms::TStringType<NMms::TMmapped>("abc", 3)), "abc");
        UNIT_ASSERT_EQUAL(static_cast<TStringBuf>(NMms::TStringType<NMms::TMmapped>("a\00c", 3)), TString("a\00c", 3));
    }

    Y_UNIT_TEST(AlignStringTest) {
        size_t maxSize = 100;
        for (size_t i = 0; i <= maxSize; ++i) {
            TString s(i, 'a');
            TBufferOutput out;
            NMms::TWriter w(out);
            NMms::TStringType<NMms::TStandalone> mmsString(s);

            size_t ofs = NMms::UnsafeWrite(w, mmsString);
            const TBuffer& buf = out.Buffer();
            NMms::TStringType<NMms::TMmapped> mappedString =
                *reinterpret_cast<const NMms::TStringType<NMms::TMmapped>*>(buf.Data() + ofs);

            UNIT_ASSERT(ofs % sizeof(void*) == 0);
            UNIT_ASSERT(IsAligned(mappedString));
            UNIT_ASSERT_EQUAL(w.pos(), AlignedSize(sizeof(char) * (i + 1)) + 2 * sizeof(size_t) //reference size
            );
        }
    }

    Y_UNIT_TEST(RefCounted) {
        TString str = "abacaba";
        UNIT_ASSERT_EQUAL(str.IsDetached(), true);

        NMms::TStringType<NMms::TStandalone> mmsStr = str;
        UNIT_ASSERT_EQUAL(str.IsDetached(), false);
        UNIT_ASSERT_EQUAL(mmsStr.IsDetached(), false);
    }

    Y_UNIT_TEST(StringBuilder) {
        TMmsObjects objs;
        const TString str = "abacaba";
        const auto strStandalone = NMms::TStringType<NMms::TStandalone>(str);
        const auto strMmapped = objs.MakeMmappedString(strStandalone);

        UNIT_ASSERT_EQUAL(str, TStringBuilder() << strStandalone);
        UNIT_ASSERT_EQUAL(str, TStringBuilder() << strMmapped);
    }
}
