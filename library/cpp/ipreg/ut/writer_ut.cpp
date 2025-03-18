#include <library/cpp/testing/unittest/registar.h>

#include <library/cpp/ipreg/writer.h>

using namespace NIPREG;

Y_UNIT_TEST_SUITE(WriterTest) {
    Y_UNIT_TEST(PlainWriterOne) {
        TStringStream ss;

        TWriter writer(ss, "  ", EAddressFormat::IPV6);

        writer.Write(TAddress::ParseAny("::1"), TAddress::ParseAny("::2"), "a");
        writer.Finalize();

        UNIT_ASSERT_STRINGS_EQUAL(
            ss.Str(),
            "0000:0000:0000:0000:0000:0000:0000:0001  0000:0000:0000:0000:0000:0000:0000:0002  a\n"
        );
    }

    Y_UNIT_TEST(PlainWriterMany) {
        TStringStream ss;

        TWriter writer(ss);

        writer.Write(TAddress::ParseAny("::1"), TAddress::ParseAny("::2"), "a");
        writer.Write(TAddress::ParseAny("::4"), TAddress::ParseAny("::5"), "b");
        writer.Write(TAddress::ParseAny("::5"), TAddress::ParseAny("::6"), "c");
        writer.Finalize();

        UNIT_ASSERT_STRINGS_EQUAL(
            ss.Str(),
            "0000:0000:0000:0000:0000:0000:0000:0001-0000:0000:0000:0000:0000:0000:0000:0002\ta\n"
            "0000:0000:0000:0000:0000:0000:0000:0004-0000:0000:0000:0000:0000:0000:0000:0005\tb\n"
            "0000:0000:0000:0000:0000:0000:0000:0005-0000:0000:0000:0000:0000:0000:0000:0006\tc\n"
        );
    }

    Y_UNIT_TEST(MergingWriterOne) {
        TStringStream ss;

        TMergingWriter writer(ss);

        writer.Write(TAddress::ParseAny("::1"), TAddress::ParseAny("::2"), "a");
        writer.Finalize();

        UNIT_ASSERT_STRINGS_EQUAL(
            ss.Str(),
            "0000:0000:0000:0000:0000:0000:0000:0001-0000:0000:0000:0000:0000:0000:0000:0002\ta\n"
        );
    }

    Y_UNIT_TEST(MergingWriterMany) {
        TStringStream ss;

        TMergingWriter writer(ss, "  ", EAddressFormat::IPV6);

        writer.Write(TAddress::ParseAny("::1"), TAddress::ParseAny("::2"), "a");
        writer.Write(TAddress::ParseAny("::3"), TAddress::ParseAny("::4"), "a");
        writer.Write(TAddress::ParseAny("::6"), TAddress::ParseAny("::7"), "a");
        writer.Write(TAddress::ParseAny("::8"), TAddress::ParseAny("::9"), "a");
        writer.Write(TAddress::ParseAny("::a"), TAddress::ParseAny("::b"), "b");
        writer.Write(TAddress::ParseAny("::c"), TAddress::ParseAny("::d"), "b");
        writer.Write(TAddress::ParseAny("::10"), TAddress::ParseAny("::11"), "c");
        writer.Finalize();

        UNIT_ASSERT_STRINGS_EQUAL(
            ss.Str(),
            "0000:0000:0000:0000:0000:0000:0000:0001  0000:0000:0000:0000:0000:0000:0000:0004  a\n"
            "0000:0000:0000:0000:0000:0000:0000:0006  0000:0000:0000:0000:0000:0000:0000:0009  a\n"
            "0000:0000:0000:0000:0000:0000:0000:000a  0000:0000:0000:0000:0000:0000:0000:000d  b\n"
            "0000:0000:0000:0000:0000:0000:0000:0010  0000:0000:0000:0000:0000:0000:0000:0011  c\n"
        );
    }
}
