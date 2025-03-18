#include <antirobot/lib/ja3_parser/parser.h>
#include <library/cpp/testing/unittest/registar.h>

namespace NAntiRobot {
    const float DEFAULT_EPS = 1e-8;
    Y_UNIT_TEST_SUITE(TestJa3Parser) {
        Y_UNIT_TEST(TestStringToJa3Valid) {
            {   //                    0    1    2    3     4     5     6     7     8     9     10   11  12 13 14
                TString text = "771,4865-4866-4867-49195-49199-49196-49200-52393-52392-49171-49172-156-157-47-53,0-23-65281-10-11-35-16-5-13-18-51-45-43-21,29-23-24,0";
                //                                                                                               1 2    3   4  5  6  7  8 9  10 11 12 13 14
                Ja3Parser::TJa3 ja3 = Ja3Parser::StringToJa3(text);
                UNIT_ASSERT_EQUAL(ja3.TlsVersion, 771);
                UNIT_ASSERT_EQUAL(ja3.CiphersLen, 15);
                UNIT_ASSERT_EQUAL(ja3.ExtensionsLen, 14);
                UNIT_ASSERT_EQUAL(ja3.C159, 0);
                UNIT_ASSERT_EQUAL(ja3.C57_61, 0);
                UNIT_ASSERT_DOUBLES_EQUAL(ja3.C53, 1.0 / (1 + 14), DEFAULT_EPS);
                UNIT_ASSERT_EQUAL(ja3.C60_49187, 0);
                UNIT_ASSERT_EQUAL(ja3.C53_49187, 0);
                UNIT_ASSERT_EQUAL(ja3.C52393_103, 0);
                UNIT_ASSERT_EQUAL(ja3.C49162, 0);
                UNIT_ASSERT_EQUAL(ja3.C50, 0);
                UNIT_ASSERT_EQUAL(ja3.C51, 0);
                UNIT_ASSERT_EQUAL(ja3.C255, 0);
                UNIT_ASSERT_DOUBLES_EQUAL(ja3.C52392, 1.0 / (1 + 8), DEFAULT_EPS);
                UNIT_ASSERT_EQUAL(ja3.C10, 0);
                UNIT_ASSERT_EQUAL(ja3.C157_49200, 0);
                UNIT_ASSERT_DOUBLES_EQUAL(ja3.C49200, 1.0 / (1 + 6), DEFAULT_EPS);
                UNIT_ASSERT_EQUAL(ja3.C49171_103, 0);
                UNIT_ASSERT_EQUAL(ja3.C49191_52394, 0);
                UNIT_ASSERT_EQUAL(ja3.C49192_52394, 0);
                UNIT_ASSERT_EQUAL(ja3.C65_52394, 0);
                UNIT_ASSERT_DOUBLES_EQUAL(ja3.C157, 1.0 / (1 + 12), DEFAULT_EPS);
                UNIT_ASSERT_EQUAL(ja3.C52393_49200, 0);
                UNIT_ASSERT_EQUAL(ja3.C49159, 0);
                UNIT_ASSERT_DOUBLES_EQUAL(ja3.C4865, 1.0 / (1 + 0), DEFAULT_EPS);
                UNIT_ASSERT_EQUAL(ja3.C158_61, 0);
                UNIT_ASSERT_EQUAL(ja3.C49196_47, 0);
                UNIT_ASSERT_EQUAL(ja3.C103, 0);
                UNIT_ASSERT_EQUAL(ja3.C103_49196, 0);
                UNIT_ASSERT_EQUAL(ja3.C52393_49188, 0);
                UNIT_ASSERT_EQUAL(ja3.C60_65, 0);
                UNIT_ASSERT_EQUAL(ja3.C49195_69, 0);
                UNIT_ASSERT_EQUAL(ja3.C154, 0);
                UNIT_ASSERT_EQUAL(ja3.C49187_49188, 0);
                UNIT_ASSERT_EQUAL(ja3.C49199_60, 0);
                UNIT_ASSERT_EQUAL(ja3.C49195_51, 0);
                UNIT_ASSERT_EQUAL(ja3.C49235, 0);
                UNIT_ASSERT_DOUBLES_EQUAL(ja3.C47, 1.0 / (1 + 13), DEFAULT_EPS);
                UNIT_ASSERT_EQUAL(ja3.C49169, 0);
                UNIT_ASSERT_EQUAL(ja3.C49249, 0);
                UNIT_ASSERT_EQUAL(ja3.C49171_60, 0);
                UNIT_ASSERT_EQUAL(ja3.C49188_49196, 0);
                UNIT_ASSERT_EQUAL(ja3.C61, 0);
                UNIT_ASSERT_EQUAL(ja3.C156_255, 0);
                UNIT_ASSERT_EQUAL(ja3.C47_57, 0);
                UNIT_ASSERT_EQUAL(ja3.C186, 0);
                UNIT_ASSERT_EQUAL(ja3.C49245, 0);
                UNIT_ASSERT_EQUAL(ja3.C156_52394, 0);
                UNIT_ASSERT_EQUAL(ja3.C20, 0);
                UNIT_ASSERT_EQUAL(ja3.C49188_49195, 0);
                UNIT_ASSERT_EQUAL(ja3.C52394_52392, 0);
                UNIT_ASSERT_EQUAL(ja3.C53_49162, 0);
                UNIT_ASSERT_EQUAL(ja3.C49191, 0);
                UNIT_ASSERT_EQUAL(ja3.C49245_49249, 0);
                UNIT_ASSERT_DOUBLES_EQUAL(ja3.C49171, 1.0 / (1 + 9), DEFAULT_EPS);
                UNIT_ASSERT_EQUAL(ja3.C53_52393, 0);
                UNIT_ASSERT_EQUAL(ja3.C51_49199, 0);
                UNIT_ASSERT_EQUAL(ja3.C49234, 0);
                UNIT_ASSERT_EQUAL(ja3.C49315, 0);
                UNIT_ASSERT_EQUAL(ja3.C158, 0);
                UNIT_ASSERT_EQUAL(ja3.C49187_49161, 0);
                UNIT_ASSERT_EQUAL(ja3.C107, 0);
                UNIT_ASSERT_EQUAL(ja3.C52394, 0);
                UNIT_ASSERT_EQUAL(ja3.C49162_61, 0);
                UNIT_ASSERT_EQUAL(ja3.C153, 0);
                UNIT_ASSERT_EQUAL(ja3.C49170, 0);
                UNIT_ASSERT_DOUBLES_EQUAL(ja3.C156, 1.0 / (1 + 11), DEFAULT_EPS);
                UNIT_ASSERT_EQUAL(ja3.C52393_60, 0);
                UNIT_ASSERT_EQUAL(ja3.C49195_49192, 0);
                UNIT_ASSERT_EQUAL(ja3.C7, 0);
                UNIT_ASSERT_EQUAL(ja3.C49187_103, 0);
                UNIT_ASSERT_EQUAL(ja3.C61_49172, 0);
                UNIT_ASSERT_EQUAL(ja3.C159_49188, 0);
                UNIT_ASSERT_EQUAL(ja3.C49171_49187, 0);
                UNIT_ASSERT_EQUAL(ja3.C49196_49188, 0);
                UNIT_ASSERT_EQUAL(ja3.C158_49161, 0);
                UNIT_ASSERT_EQUAL(ja3.C49193, 0);
            }
        }
        Y_UNIT_TEST(TestStringToJa3Invalid) {
            {
                TString text = "invalid_version771,4865-4866-4867-49195-49199-49196-49200-52393-52392-49171-49172-156-157-47-53,0-23-65281-10-11-35-16-5-13-18-51-45-43-21,29-23-24,0";
                UNIT_CHECK_GENERATED_EXCEPTION(Ja3Parser::StringToJa3(text), std::exception);
            }
        }
    }
} // namespace NAntiRobot
