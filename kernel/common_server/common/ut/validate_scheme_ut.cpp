#include <kernel/common_server/common/scheme.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/logger/global/global.h>
#include <util/generic/guid.h>

Y_UNIT_TEST_SUITE(ValidateJsonByScheme) {

    void PrintJsonDescription(const NFrontend::TScheme& scheme, NJson::TJsonValue& json) {
        TStringStream so;
        NJsonWriter::TBuf buf;
        scheme.MakeJsonDescription(json, buf, so);
        INFO_LOG << so.Str() << Endl;
    }

    bool ValidateJson(const NFrontend::TScheme& scheme, const NJson::TJsonValue& json) {
        return scheme.ValidateJson(json);
    }

    template<class T, class TAction>
    void AddJsonKey(const NFrontend::TScheme& scheme, NJson::TJsonValue& json, const TString& key, const T& value, const TAction& validateFunction) {
        json.InsertValue(key, value);
        UNIT_ASSERT_C(validateFunction(scheme, json), json.GetStringRobust());

        PrintJsonDescription(scheme, json);
    }

    template<class T, class TAction>
    void AddFailJsonKey(const NFrontend::TScheme& scheme, NJson::TJsonValue& json, const TString& key, const T& value, const TAction& validateFunction) {
        json.InsertValue(key, value);
        UNIT_ASSERT_C(!validateFunction(scheme, json), json.GetStringRobust());
    }

    template <class TAction>
    void TestTemplate(const TAction& validateFunction) {
        NJson::TJsonValue json;
        NFrontend::TScheme scheme;
        UNIT_ASSERT_C(!validateFunction(scheme, json), json.GetStringRobust());

        json = NJson::JSON_MAP;
        UNIT_ASSERT_C(validateFunction(scheme, json), json.GetStringRobust());

        PrintJsonDescription(scheme, json);

        // TFSString
        AddFailJsonKey(scheme, json, "key_string", "value1", validateFunction);

        scheme.Add<TFSString>("key_string", "key for TFSString value");
        UNIT_ASSERT_C(validateFunction(scheme, json), json.GetStringRobust());

        // TFSNumeric
        scheme.Add<TFSNumeric>("key_numeric", "key for TFSNumeric value").SetMin(-5).SetMax(5).SetVisual(TFSNumeric::EVisualTypes::Money);
        UNIT_ASSERT_C(validateFunction(scheme, json), json.GetStringRobust());

        PrintJsonDescription(scheme, json);

        AddFailJsonKey(scheme, json, "key_numeric", "value1", validateFunction);
        AddFailJsonKey(scheme, json, "key_numeric", 10, validateFunction);
        AddFailJsonKey(scheme, json, "key_numeric", -3, validateFunction);
        AddJsonKey(scheme, json, "key_numeric", 3, validateFunction);

        // TFSBoolean
        scheme.Add<TFSBoolean>("key_bool", "key for TFSBoolean value");
        AddFailJsonKey(scheme, json, "key_bool", "string", validateFunction);
        AddJsonKey(scheme, json, "key_bool", true, validateFunction);

        // TFSVariants
        scheme.Add<TFSVariants>("key_variants", "key for TFSVariants value").SetVariants({ "a", "b", "c" });
        AddFailJsonKey(scheme, json, "key_variants", "string", validateFunction);
        AddJsonKey(scheme, json, "key_variants", "a", validateFunction);

        // TFSDuration
        scheme.Add<TFSDuration>("key_duration", "key for TFSDuration value").SetMin(TDuration::Seconds(25));
        AddFailJsonKey(scheme, json, "key_duration", "string", validateFunction);
        AddFailJsonKey(scheme, json, "key_duration", "5s", validateFunction);
        AddJsonKey(scheme, json, "key_duration", "10m", validateFunction);

        // TFSArray
        NJson::TJsonValue jsonContainers;
        NJson::TJsonValue& jsonArray = jsonContainers["key_array"];
        NFrontend::TScheme schemeContainers;
        schemeContainers.Add<TFSArray>("key_array", "key for TFSArray value").SetElement(scheme);
        UNIT_ASSERT_C(!validateFunction(schemeContainers, jsonContainers), jsonContainers.GetStringRobust());

        jsonArray.SetType(NJson::JSON_ARRAY);
        UNIT_ASSERT_C(validateFunction(schemeContainers, jsonContainers), jsonContainers.GetStringRobust());

        PrintJsonDescription(schemeContainers, jsonContainers);

        for (size_t i = 0; i < 5; ++i) {
            jsonArray.AppendValue(json);
        }
        UNIT_ASSERT_C(validateFunction(schemeContainers, jsonContainers), jsonContainers.GetStringRobust());

        PrintJsonDescription(schemeContainers, jsonContainers);

        NJson::TJsonValue& jsonArrayArray = jsonContainers["key_array_array"];
        jsonArrayArray.AppendValue(jsonArray);
        schemeContainers.Add<TFSArray>("key_array_array", "key for TFSArray value").SetElement<TFSArray>().SetElement(scheme);
        UNIT_ASSERT_C(validateFunction(schemeContainers, jsonContainers), jsonContainers.GetStringRobust());

        PrintJsonDescription(schemeContainers, jsonContainers);

        // TFSStructure
        NJson::TJsonValue& jsonStructure = jsonContainers["key_structure"];
        jsonStructure = json;
        schemeContainers.Add<TFSStructure>("key_structure", "key for TFSStructure value").SetStructure(scheme);
        UNIT_ASSERT_C(validateFunction(schemeContainers, jsonContainers), jsonContainers.GetStringRobust());

        PrintJsonDescription(schemeContainers, jsonContainers);
    }

    Y_UNIT_TEST(ValidateScheme) {
        TestTemplate(ValidateJson);
    }
}


