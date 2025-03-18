#include <util/generic/yexception.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/system/tempfile.h>
#include <util/stream/file.h>
#include <tools/json_to_jsonlines/json_to_jsonlines.h>

using namespace NJsonToJsonLines;

void WriteInput(const TString& fileName, const TString& jsonData) {
    TFileOutput dst(fileName);
    dst.Write(jsonData);
}


TString ReadInput(const TString& fileName) {
    TFileInput src(fileName);
    return src.ReadAll();
}

void TestConversionSmoke(const TString& alias, const TString& jsonData, bool doLines) {
    TString tmpName = alias + "temp-test-input-file.json";
    TTempFile tmpIn(tmpName);
    TTempFile tmpOut(tmpName + ".output");

    {
        WriteInput(tmpIn.Name(), jsonData);

        TJsonToJsonLinesStreamParser parser(tmpIn.Name(), tmpOut.Name(), doLines);
        parser.RunConversion();
    }
}

void TestConversionComplete(const TString& alias, const TString& jsonData, const TString& expectedData, bool doLines, bool reversed = false) {
    TString tmpName = alias + "temp-test-input-file.json";
    TTempFile tmpIn(tmpName);
    TTempFile tmpOut(tmpName + ".output");

    {
        WriteInput(tmpIn.Name(), jsonData);

        TJsonToJsonLinesStreamParser parser(tmpIn.Name(), tmpOut.Name(), doLines, reversed);
        parser.RunConversion();

        TString outputData = ReadInput(tmpOut.Name());
        for (size_t i = 0; i < expectedData.size(); ++i) {
            if (i < outputData.size()) {
                UNIT_ASSERT_C(expectedData[i] == outputData[i],
                              "Bad index " << i << ": expected = " << expectedData[i] << ", actual = " << outputData[i]);
            } else {
                UNIT_ASSERT_C(false, "Bad index " << i << ": expected = " << expectedData[i] << ", actual output is shorter");
            }
        }
        UNIT_ASSERT(expectedData == outputData);
    }
}


Y_UNIT_TEST_SUITE(TJsonToJsonLinesStreamParserTest) {

    Y_UNIT_TEST(JsonToJsonLinesSimple1) {
        TestConversionSmoke("simple1", "[{\"key\":0}]", true);
    }

    Y_UNIT_TEST(JsonToJsonLinesSimple2) {
        TestConversionSmoke("simple2", "[{\"key\":0},{\"key\":1}]", true);
    }

    Y_UNIT_TEST(JsonToJsonLinesSimple3) {
        TestConversionSmoke("simple2-no-lines", "[{\"key\":0},{\"key\":1}]", false);
    }

    Y_UNIT_TEST(JsonToJsonLinesList1) {
        TString src = "[{\"key\":0},{\"key\":1}]";
        TString dst = "{\"key\":0}\n{\"key\":1}\n";
        TestConversionComplete("list", src, dst, true);
    }

    Y_UNIT_TEST(JsonToJsonLinesEscape1) {
        TString src = "[{\"key\":0},{\"key\":1},{\"escaped\\n\":2}]";
        TString dst = "{\"key\":0}\n{\"key\":1}\n{\"escaped\\n\":2}\n";
        TestConversionComplete("escape", src, dst, true);
    }

    Y_UNIT_TEST(JsonToJsonLinesCommas1) {
        TString src = "[{},{\"key\":{}},{\"key\":[]}]";
        TString dst = "{}\n{\"key\":{}}\n{\"key\":[]}\n";
        TestConversionComplete("commas", src, dst, true);
    }

    Y_UNIT_TEST(JsonToJsonLinesCommasAndSpaces) {
        TString src = "[{\"key\":[[],[1  ],[2, 3, 4]]}]";
        TString dst = "{\"key\":[[],[1],[2,3,4]]}\n";
        TestConversionComplete("commas-and-spaces", src, dst, true);
    }

    Y_UNIT_TEST(JsonToJsonLinesConstants) {
        TString src = "[{\"key\":[[null,{}],{   },[true, false,  null ],[true, false, [], 1,true]]}]";
        TString dst = "{\"key\":[[null,{}],{},[true,false,null],[true,false,[],1,true]]}\n";
        TestConversionComplete("constants", src, dst, true);
    }

    Y_UNIT_TEST(UnterminatedJson) {
        TString src = "[{\"key\":[]}";
        try {
            TestConversionSmoke("unterminated", src, true);
            Y_ASSERT(false);
        } catch (const yexception& exc) {
        }
    }

    Y_UNIT_TEST(BrokenListWithTrailingComma) {
        TString src = "[[{\"string\":[1,2,3]}]]]";
        try {
            TestConversionSmoke("broken", src, true);
            Y_UNREACHABLE();
        } catch (const yexception& exc) {}
    }

    Y_UNIT_TEST(JsonLinesToJsonReverse) {
        TString src = "{\"key\":0}\n{\"key\":1}\n{\"key\":2}\n";
        TString dst = "[\n"
                      "{\"key\":0}\n"
                      ",{\"key\":1}\n"
                      ",{\"key\":2}\n"
                      "]";
        TestConversionComplete("reversed", src, dst, false, true);
    }


}
