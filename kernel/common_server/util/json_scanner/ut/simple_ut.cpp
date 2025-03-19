#include <fintech/risk/backend/src/factors_constructor/constructors/json_scanner/scanner.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/json/json_reader.h>

using namespace NCS;
Y_UNIT_TEST_SUITE(Json) {

    const TString jsonSimpleExample = R"(
{
  "a": "b",
  "c": "d",
  "e": [
    1,
    2,
    3
  ],
  "d": {
    "l": [
      {
        "c": 1
      },
      {
        "d": 2
      },
      {
        "e": 3
      }
    ],
    "f": [
      {
        "a": 4
      },
      {
        "b": 5
      },
      {
        "c": 6
      }
    ]
  }
}
)";

    class TJsonScannerByPathTest: public TJsonScannerByPath {
    private:
        using TBase = TJsonScannerByPath;
        IOutputStream& Stream;
    protected:
        virtual bool DoExecute(const NJson::TJsonValue& value) const override {
            Stream << value << ":";
            for (auto&& i : GetContextInfo()) {
                Stream << i.first << "=" << i.second->GetStringRobust() << ";";
            }
            return true;
        }
    public:
        TJsonScannerByPathTest(const NJson::TJsonValue& jsonDoc, IOutputStream& os)
            : TBase(jsonDoc)
            , Stream(os)
        {

        }
    };

    Y_UNIT_TEST(Simple) {
        NJson::TJsonValue jsonValue = NJson::ReadJsonFastTree(jsonSimpleExample);
        {
            TStringStream ss;
            TJsonScannerByPathTest parser(jsonValue, ss);
            CHECK_WITH_LOG(parser.Scan("[d][*]"));
            CHECK_WITH_LOG(ss.Str() == R"([{"a":4},{"b":5},{"c":6}]:[{"c":1},{"d":2},{"e":3}]:)");
        }
        {
            TStringStream ss;
            TJsonScannerByPathTest parser(jsonValue, ss);
            CHECK_WITH_LOG(parser.Scan("[d][*|w:__key][*|h:__key][*]"));
            CHECK_WITH_LOG(ss.Str() == R"(4:h=0;w=f;5:h=1;w=f;6:h=2;w=f;1:h=0;w=l;2:h=1;w=l;3:h=2;w=l;)") << ss.Str();
        }
        {
            TStringStream ss;
            TJsonScannerByPathTest parser(jsonValue, ss);
            CHECK_WITH_LOG(parser.Scan("[d][l|w:__key][*|h:__key]"));
            CHECK_WITH_LOG(ss.Str() == R"({"c":1}:h=0;w=l;{"d":2}:h=1;w=l;{"e":3}:h=2;w=l;)") << ss.Str();
        }
    };

    Y_UNIT_TEST(NoData) {
        NJson::TJsonValue jsonValue = NJson::ReadJsonFastTree(jsonSimpleExample);
        {
            TStringStream ss;
            TJsonScannerByPathTest parser(jsonValue, ss);
            CHECK_WITH_LOG(parser.Scan("[b][*]"));
        }
    };

    const TString jsonSelectiveExample = R"({
"d" : [{"a" : 4, "party" : "a"}, {"b" : 5, "party" : "b"}, {"c" : 4}]
})";

    Y_UNIT_TEST(Selective) {
        NJson::TJsonValue jsonValue = NJson::ReadJsonFastTree(jsonSelectiveExample);
        {
            TStringStream ss;
            TJsonScannerByPathTest parser(jsonValue, ss);
            CHECK_WITH_LOG(parser.Scan("[d][*?party=test]")) << ss.Str() << Endl;
            CHECK_WITH_LOG(!ss.Str()) << ss.Str() << Endl;

        }
        {
            TStringStream ss;
            TJsonScannerByPathTest parser(jsonValue, ss);
            CHECK_WITH_LOG(parser.Scan("[d][*?party=b|value:b]"));
            CHECK_WITH_LOG(ss.Str() == R"({"party":"b","b":5}:value=5;)") << ss.Str();
        }
    };
}
