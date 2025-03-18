#include "eval.h"

#include <library/cpp/json/json_value.h>

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(TLuaEvalTest){
Y_UNIT_TEST(EvalExpression) {
    TLuaEval eval;

    eval.SetVariable("x", "12");
    eval.SetVariable("y", "34");
    eval.SetVariable("z", 56);

    NJson::TJsonValue value;

    value.InsertValue("test", 42);
    value.InsertValue("zz", 44);

    value.InsertValue("arr", NJson::TJsonValue());
    value["arr"].AppendValue(1);
    value["arr"].AppendValue(2);
    value["arr"].AppendValue(3);
    value["arr"].AppendValue(NJson::TJsonValue(NJson::JSON_NULL));
    value["arr"].AppendValue("some_string");

    eval.SetVariable("json", value);

    UNIT_ASSERT_VALUES_EQUAL(eval.EvalExpression("x + y"), "46");
    UNIT_ASSERT_VALUES_EQUAL(eval.EvalExpression("(x + y) > 0"), "true");
    UNIT_ASSERT_VALUES_EQUAL(eval.EvalExpression("a and a or 'b'"), "b");
    UNIT_ASSERT_VALUES_EQUAL(eval.EvalExpression("x and x or 'b'"), "12");
    UNIT_ASSERT_VALUES_EQUAL(eval.EvalExpression("x .. 'b'"), "12b");
    UNIT_ASSERT_VALUES_EQUAL(eval.EvalExpression("z"), "56");
    UNIT_ASSERT_VALUES_EQUAL(eval.EvalExpression("json.test"), "42");
    UNIT_ASSERT_VALUES_EQUAL(eval.EvalExpression("json['zz']"), "44");
    UNIT_ASSERT_VALUES_EQUAL(eval.EvalExpression("json.arr[2]"), "2");
    UNIT_ASSERT_VALUES_EQUAL(eval.EvalExpression("tostring(json.arr[4])"), "nil");
    UNIT_ASSERT_VALUES_EQUAL(eval.EvalExpression("json.arr[5]"), "some_string");
    UNIT_ASSERT_EXCEPTION(eval.EvalExpression("x:100"), yexception);

    const auto compiled = eval.Compile("x + y");
    UNIT_ASSERT_VALUES_EQUAL(eval.EvalCompiled(compiled), "46");
    eval.SetVariable("y", "3");
    UNIT_ASSERT_VALUES_EQUAL(eval.EvalCompiled(compiled), "15");
}

Y_UNIT_TEST(DeepJsonTest) {
    NJson::TJsonValue json = "hi";

    for (int i = 0; i < 40; i++) {
        NJson::TJsonValue new_json;
        new_json.InsertValue("hi", json);
        json = new_json;
    }

    TLuaEval eval;
    eval.SetVariable("deep_json", json);

    UNIT_ASSERT_NO_EXCEPTION(eval.EvalExpression("2 + 2"));
}

Y_UNIT_TEST(BigArrayTest) {
    NJson::TJsonValue json;

    for (int i = 0; i < 400; i++) {
        json.AppendValue(42);
    }

    TLuaEval eval;
    eval.SetVariable("deep_json", json);

    UNIT_ASSERT_NO_EXCEPTION(eval.EvalExpression("2 + 2"));
}

Y_UNIT_TEST(StackSize) {
    TLuaEval eval;
    eval.SetVariable("x", 100);
    auto compiled = eval.Compile("x > 0");

    for (int i = 0; i < 1000000; ++i) {
        UNIT_ASSERT_VALUES_EQUAL(eval.EvalCompiled(compiled), "true");
        UNIT_ASSERT_VALUES_EQUAL(eval.EvalCompiledCondition(compiled), true);
    }
}

Y_UNIT_TEST(NonCondition) {
    TLuaEval eval;
    auto compiled = eval.Compile("100");
    UNIT_ASSERT_EXCEPTION(eval.EvalCompiledCondition(compiled), yexception);
}

struct TUserDataLocal {
    TMap<TString, ui32>* Values;

    ui32 GetValue(const TStringBuf sb) const {
        if (!Values) {
            return 0;
        }
        auto it = Values->find(TString(sb));
        if (it == Values->end()) {
            return 0;
        }
        return it->second;
    }

    void SetValue(const TStringBuf sb, const TStringBuf sbValue) {
        (*Values)[TString(sb)] = FromString<ui32>(sbValue);
    }

    int Get(TLuaStateHolder& state) const {
        state.require(2);
        if (state.is_string(2)) {
            auto v = GetValue(state.to_string(2, "undefined"));
            state.push_number(v);
        } else {
            state.push_nil();
        }
        return 1;
    }

    int Set(TLuaStateHolder& state) {
        state.require(3);
        SetValue(state.to_string_strict(2), state.to_string_strict(3));
        return 0;
    }

    static const char LUA_METATABLE_NAME[];
    static const luaL_Reg LUA_FUNCTIONS[];
};

const luaL_Reg TUserDataLocal::LUA_FUNCTIONS[] = {
    {"get", NLua::MethodConstHandler<TUserDataLocal, &TUserDataLocal::Get>},
    {"set", NLua::MethodHandler<TUserDataLocal, &TUserDataLocal::Set>},
    {nullptr, nullptr} };
const char TUserDataLocal::LUA_METATABLE_NAME[] = "user_data_local";

Y_UNIT_TEST(Function) {
    TLuaEval eval;
    TMap<TString, ui32> values;
    {
        TUserDataLocal uData;
        uData.Values = &values;
        eval.SetUserdata("factors", std::move(uData));
    }
    {
        const auto compilation = eval.CompileFunction("return factors:get('abcde') + 1");
        values["abcde"] = FromString<ui32>(eval.EvalCompiled(compilation));
        values["abcde"] = FromString<ui32>(eval.EvalCompiled(compilation));
        values["abcde"] = FromString<ui32>(eval.EvalCompiled(compilation));
        UNIT_ASSERT_EQUAL(values["abcde"], 3);
    }
    {
        const auto compilation = eval.CompileFunction("factors:set('abcde', factors:get('abcde') + 1);");
        eval.EvalCompiledRaw(compilation);
        eval.EvalCompiledRaw(compilation);
        eval.EvalCompiledRaw(compilation);
        UNIT_ASSERT_EQUAL(values["abcde"], 6);
    }
}

}
