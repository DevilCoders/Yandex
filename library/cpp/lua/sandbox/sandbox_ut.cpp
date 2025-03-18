#include "sandbox.h"
#include <library/cpp/testing/unittest/registar.h>

class TLuaSandboxTest: public TTestBase {
    UNIT_TEST_SUITE(TLuaSandboxTest);
    UNIT_TEST(TestSimple)
    UNIT_TEST(TestAccess)
    UNIT_TEST(TestUpdate)
    UNIT_TEST(TestNewEnv)
    UNIT_TEST(TestNewEnvUpdate)
    UNIT_TEST(TestCommonEnv)
    UNIT_TEST(TestDifferentEnvs)
    UNIT_TEST_SUITE_END();

public:
    void TestSimple() {
        NLua::TSandbox sandbox;
        NLua::TSandbox::TScript f(sandbox, ScriptFactorial);
        f.Push();
        f.State.push_string("6");
        f.State.call(1, 1);
        // user script uses whitelisted functions tonumber and tostring
        UNIT_ASSERT_VALUES_EQUAL(f.State.pop_string(), "720");
    }

    void TestAccess() {
        NLua::TSandbox sandbox;
        NLua::TSandbox::TScript f(sandbox, ScriptPrint);
        f.Push();
        // sandboxed script isn't able to call print function
        UNIT_ASSERT_EXCEPTION(f.State.call(0, 0), TLuaStateHolder::TError);
    }

    void TestUpdate() {
        NLua::TSandbox sandbox;
        NLua::TSandbox::TScript f(sandbox, ScriptCallUser);
        sandbox.State.register_function("user", NLua::FunctionHandler<UserFunction>);

        // user function registered, but not yet available for the sandboxed script
        f.Push();
        UNIT_ASSERT_EXCEPTION(f.State.call(0, 1), TLuaStateHolder::TError);

        // add user function to the white list
        sandbox.UpdateEnv("user", "user");
        f.Push();
        UNIT_ASSERT_NO_EXCEPTION(f.State.call(0, 1));
        UNIT_ASSERT_VALUES_EQUAL(f.State.pop_number<int>(), 123);

        // remove user function
        sandbox.UpdateEnv("user");
        f.Push();
        UNIT_ASSERT_EXCEPTION(f.State.call(0, 1), TLuaStateHolder::TError);
    }

    void TestNewEnv() {
        NLua::TSandbox sandbox;
        NLua::TSandbox::TScript f(sandbox, ScriptFactorial, true);

        // empty env
        f.Push();
        f.State.push_string("6");
        UNIT_ASSERT_EXCEPTION(f.State.call(1, 1), TLuaStateHolder::TError);

        // env with standart functions
        f.InitEnv();
        f.Push();
        f.State.push_string("5");
        f.State.call(1, 1);
        UNIT_ASSERT_VALUES_EQUAL(f.State.pop_string(), "120");
    }

    void TestNewEnvUpdate() {
        NLua::TSandbox sandbox;
        NLua::TSandbox::TScript f(sandbox, ScriptCallUser, true);
        sandbox.State.register_function("user", NLua::FunctionHandler<UserFunction>);

        // user function registered, but not yet available for the sandboxed script
        f.Push();
        UNIT_ASSERT_EXCEPTION(f.State.call(0, 1), TLuaStateHolder::TError);

        // add user function to the white list
        f.UpdateEnv("user", "user");
        f.Push();
        UNIT_ASSERT_NO_EXCEPTION(f.State.call(0, 1));
        UNIT_ASSERT_VALUES_EQUAL(f.State.pop_number<int>(), 123);

        // remove user function
        f.UpdateEnv("user");
        f.Push();
        UNIT_ASSERT_EXCEPTION(f.State.call(0, 1), TLuaStateHolder::TError);
    }

    void TestCommonEnv() {
        NLua::TSandbox sandbox;
        NLua::TSandbox::TScript setX(sandbox, ScriptSetX);
        NLua::TSandbox::TScript getX(sandbox, ScriptGetX);

        const int x = 2013;

        setX.Push();
        setX.State.push_number(x);
        setX.State.call(1, 0);

        getX.Push();
        getX.State.call(0, 1);
        UNIT_ASSERT_VALUES_EQUAL(getX.State.pop_number<int>(), x);
    }

    void TestDifferentEnvs() {
        NLua::TSandbox sandbox;
        NLua::TSandbox::TScript setX(sandbox, ScriptSetX, true);
        NLua::TSandbox::TScript getX(sandbox, ScriptGetX, true);

        const int x = 2013;

        setX.Push();
        setX.State.push_number(x);
        setX.State.call(1, 0);

        getX.Push();
        getX.State.call(0, 1);
        UNIT_ASSERT_VALUES_EQUAL(getX.State.pop_nil(), true);
    }

private:
    static int UserFunction(TLuaStateHolder& state) {
        state.push_number(123);
        return 1;
    }

    const static TStringBuf ScriptSetX;
    const static TStringBuf ScriptGetX;
    const static TStringBuf ScriptPrint;
    const static TStringBuf ScriptCallUser;
    const static TStringBuf ScriptFactorial;
};

const TStringBuf TLuaSandboxTest::ScriptSetX("x = ...");
const TStringBuf TLuaSandboxTest::ScriptGetX("return x");
const TStringBuf TLuaSandboxTest::ScriptPrint("print(\"test\")");
const TStringBuf TLuaSandboxTest::ScriptCallUser("return user()");
const TStringBuf TLuaSandboxTest::ScriptFactorial("\
n = ...\n\
f = 1\n\
for i = 2, tonumber(n) do\n\
  f = f*i\n\
end\n\
return tostring(f)\n\
");

UNIT_TEST_SUITE_REGISTRATION(TLuaSandboxTest);
