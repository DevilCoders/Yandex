#include <kernel/snippets/custom/canonize_unicode/canonize_unicode.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/charset/wide.h>

namespace NSnippets {

Y_UNIT_TEST_SUITE(TCanonizeUnicodeTest) {
    Y_UNIT_TEST(TestCanonizeUnicode) {
        TUtf16String emptyString;
        TUtf16String noMarks = u"this 丠𓅂 🀪 is nöt €möوi!";
        TUtf16String singleMark = u"naïve";
        TUtf16String manyMarks = u"p̵̨̛̖̂̃̈͋̓̆̀̐̚͝͝";
        TUtf16String realCase = u"HODANPEDIA: the System Shock Wiki! We hope you'll enjoy your stay and help ů̵̲s̴͘ i̷̪̟̝̭̼̫̬̞͂̌̈̏ͅͅͅm̶͖̫̝̝͒́͊̽̈́͂̓͐͗̾̽̚͠p̵̨̛̖̂̃̈͋̓̆̀̐̚͝͝r̸̫͖͚̣̦̼͋̽́́̀͐͆̇͘ơ̴̢͙͔͚̩̝̙̗̤̩̺͇͙̐̇̊̂v̸̡͎̥̥̼͚͖͍̠̬͇̘̦͈̆̃̂̓̒̒̈́͂̿͝e̴̝̰̲̓̅̃̇̓̓̈́̒̚͠ t̸̷̴̸̸̵̴̴̶̶̶̸̶̸̵̷̴̵̴̶̨̨̨̧̛̗̻̲̺͎̹͚̜͚͉̫͓̗̦͚͈͍̞̭̙͍͈̖̺̬̱̘͔̻̪͙̐̉͐̓͒͐̈́̀̄̀̇́͌̋̅̍͌͑͂̆̄̿̀̆̎̎͋͑̔̋͘̕̕h̶̶̴̸̷̶̵̸̸̴̴̴̸̴̸̢̛̫̟͉̲͉͈͕͉̰̠̰̖̹̥͇͈͍̱̪̝͉͎̙̀͂̂̓͒̌̇̓̅́͂̋̋͛̋͋̅̀͆̽͐̊̚̚͜͜͠ȩ̶̷̶̷̶̶̶̶̷̷̴̷̴̴̢̛͚̼̭͇͙͓̠̳̥̫̫̳̲͈̪͓͈̓͒͊̃̏͗̂̿̀̑́̅͒̀̃͐͌̽̽̓̾́̌̕͜͝͠-I";

        TUtf16String emptyStringCopy = emptyString;
        TUtf16String noMarksCopy = noMarks;
        UNIT_ASSERT_EQUAL(CanonizeUnicode(emptyString), false);
        UNIT_ASSERT_EQUAL(CanonizeUnicode(noMarks), false);
        UNIT_ASSERT_EQUAL(CanonizeUnicode(singleMark), true);
        UNIT_ASSERT_EQUAL(CanonizeUnicode(manyMarks), true);
        UNIT_ASSERT_EQUAL(CanonizeUnicode(realCase), true);

        UNIT_ASSERT_EQUAL(emptyString, emptyStringCopy);
        UNIT_ASSERT_EQUAL(noMarks, noMarksCopy);
        UNIT_ASSERT_EQUAL(singleMark, u"naïve"); // converted 'i' and mark into single char
        UNIT_ASSERT_EQUAL(manyMarks, u"p");
        UNIT_ASSERT_EQUAL(realCase, u"HODANPEDIA: the System Shock Wiki! We hope you'll enjoy your stay and help ůs imprơve thȩ-I");
    }
}

}
