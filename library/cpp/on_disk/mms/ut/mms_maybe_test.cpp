#include <library/cpp/on_disk/mms/cast.h>
#include <library/cpp/on_disk/mms/copy.h>
#include <library/cpp/on_disk/mms/declare_fields.h>
#include <library/cpp/on_disk/mms/maybe.h>

#include <library/cpp/on_disk/mms/string.h>
#include <library/cpp/on_disk/mms/vector.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/buffer.h>
#include <util/stream/buffer.h>

template <class P>
struct TOptionalStruct {
    NMms::TMaybe<P, int> optionalInt;
    NMms::TMaybe<P, double> optionalDouble;
    NMms::TMaybe<P, NMms::TStringType<P>> optionalString;
    NMms::TMaybe<P, NMms::TMaybe<P, int>> optionalOptionalInt;
    NMms::TVectorType<P, NMms::TMaybe<P, int>> vectorOptionalInt;

    MMS_DECLARE_FIELDS(optionalInt, optionalDouble, optionalString, optionalOptionalInt, vectorOptionalInt);

    typedef TOptionalStruct<NMms::TMmapped> MmappedType;

    bool operator==(const TOptionalStruct<P>& rhs) const {
        return optionalInt == rhs.optionalInt && optionalDouble == rhs.optionalDouble && optionalString == rhs.optionalString && optionalOptionalInt == rhs.optionalOptionalInt && vectorOptionalInt == rhs.vectorOptionalInt;
    }

    bool operator!=(const TOptionalStruct<P>& rhs) const {
        return !(*this == rhs);
    }
};

Y_UNIT_TEST_SUITE(TMmsMaybeTest) {
    Y_UNIT_TEST(TestMaybe) {
        TBufferOutput out;
        NMms::TWriter w(out);

        TOptionalStruct<NMms::TStandalone> mmsOptional;
        mmsOptional.optionalInt = 35;
        mmsOptional.optionalString = NMms::TStringType<NMms::TStandalone>("abcdefg");
        mmsOptional.optionalOptionalInt = NMms::TMaybe<NMms::TStandalone, int>(13);
        mmsOptional.optionalOptionalInt = NMms::TMaybe<NMms::TStandalone, int>(13);
        mmsOptional.vectorOptionalInt.resize(100);
        for (size_t i = 13; i <= 57; ++i)
            mmsOptional.vectorOptionalInt[i] = i;

        size_t ofs = NMms::UnsafeWrite(w, mmsOptional);
        const TBuffer& buf = out.Buffer();

        TOptionalStruct<NMms::TMmapped> mappedOptional =
            NMms::UnsafeCast<TOptionalStruct<NMms::TMmapped>>(buf.Data(), buf.Size());

        UNIT_ASSERT(ofs % sizeof(void*) == 0);

        UNIT_ASSERT(static_cast<bool>(mappedOptional.optionalInt));
        UNIT_ASSERT_EQUAL(*mappedOptional.optionalInt, 35);
        UNIT_ASSERT_EQUAL(mappedOptional.optionalInt.GetOrElse(67), 35);

        UNIT_ASSERT(!mappedOptional.optionalDouble.Defined());
        UNIT_ASSERT(!mappedOptional.optionalDouble);
        UNIT_ASSERT(!static_cast<bool>(mappedOptional.optionalDouble));
        UNIT_ASSERT_EQUAL(mappedOptional.optionalDouble.GetOrElse(1.0), 1.0);

        UNIT_ASSERT(mappedOptional.optionalString.Defined());
        UNIT_ASSERT_EQUAL(mappedOptional.optionalString->length(), 7);
        UNIT_ASSERT_EQUAL(mappedOptional.optionalString.GetRef(), "abcdefg");
        UNIT_ASSERT_EQUAL(*mappedOptional.optionalString.Get(), "abcdefg");

        UNIT_ASSERT(static_cast<bool>(mappedOptional.optionalOptionalInt));
        UNIT_ASSERT(*mappedOptional.optionalOptionalInt);
        UNIT_ASSERT_EQUAL(*mappedOptional.optionalOptionalInt.GetRef(), 13);

        UNIT_ASSERT(mappedOptional.optionalInt != *mappedOptional.optionalOptionalInt);
        UNIT_ASSERT(!(mappedOptional.optionalInt == *mappedOptional.optionalOptionalInt));

        TOptionalStruct<NMms::TStandalone> copied;
        NMms::Copy(mappedOptional, copied);
        UNIT_ASSERT(copied == mmsOptional);
    }
}
