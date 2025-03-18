#include "wrapper.h"
#include <util/generic/buffer.h>
#include <util/stream/buffer.h>
#include <util/stream/mem.h>
#include <util/datetime/cputimer.h>

#include <library/cpp/testing/unittest/registar.h>

namespace {
    struct TUserData {
        int X;

        TUserData()
            : X(0)
        {
        }

        TUserData(int x)
            : X(x)
        {
        }

        int MethodGet(TLuaStateHolder& state) const {
            state.push_number(X);
            return 1;
        }

        int MethodMult(TLuaStateHolder& state) const {
            state.require(2);
            if (state.is_number(2)) {
                const int a = state.to_number<int>(2);
                state.push_number(a * X);
            } else {
                state.push_nil();
            }
            return 1;
        }

        int MethodSet(TLuaStateHolder& state) {
            X = state.to_number_strict<int>(2);
            return 0;
        }

        static const luaL_Reg LUA_FUNCTIONS[];
        static const char LUA_METATABLE_NAME[];
    };

    const luaL_Reg TUserData::LUA_FUNCTIONS[] = {
        {"get", NLua::MethodConstHandler<TUserData, &TUserData::MethodGet>},
        {"mult", NLua::MethodConstHandler<TUserData, &TUserData::MethodMult>},
        {"set", NLua::MethodHandler<TUserData, &TUserData::MethodSet>},
        {nullptr, nullptr}};

    const char TUserData::LUA_METATABLE_NAME[] = "UnitTests.simple";

    struct TDestructable {
        TDestructable(int* count)
            : Counter(count)
        {
        }

        struct TCounter {
            TCounter(int* count)
                : Count(count)
            {
            }

            ~TCounter() {
                ++*Count;
            }

            int* Count;

        } Counter;

        static const luaL_Reg LUA_FUNCTIONS[];
        static const char LUA_METATABLE_NAME[];
    };

    const luaL_Reg TDestructable::LUA_FUNCTIONS[] = {
        {"__gc", NLua::Destructor<TDestructable>},
        {nullptr, nullptr}};

    const char TDestructable::LUA_METATABLE_NAME[] = "UnitTests.destructable";

}

class TLuaStateHolderTest: public TTestBase {
    UNIT_TEST_SUITE(TLuaStateHolderTest);
    UNIT_TEST(TestCall)
    UNIT_TEST(TestString)
    UNIT_TEST(TestNumber)
    UNIT_TEST(TestBool)
    UNIT_TEST(TestNil)
    UNIT_TEST(TestVoid)
    UNIT_TEST(TestTable)
    UNIT_TEST(TestFunction)
    UNIT_TEST(TestUserdata)
    UNIT_TEST(TestUserdataMutable)
    UNIT_TEST(TestDump)
    UNIT_TEST(TestTimeLimit)
    UNIT_TEST(TestMemoryLimit)
    UNIT_TEST(TestDestructor)
    UNIT_TEST_SUITE_END();

public:
    void TestCall() {
        TLuaStateHolder state;
        InitLua(state, SimpleFunctions);

        LuaGCD(state, 40, 6);
        LuaGCD(state, 9, 99);
        LuaAvg(state, 1, 2);
        LuaAvg(state, 5, 10);
        LuaStrlen(state, "test string");
    }

    void TestString() {
        TLuaStateHolder state;
        state.BootStrap();

        TestString(state, "abcdefg", "qwerty");
        TestString(state, "hello world", "fallback string");
    }

    void TestString(TLuaStateHolder& state, const TStringBuf& testValue, const TStringBuf& fallbackValue) {
        state.push_nil();
        state.push_string(testValue);

        // is_string

        UNIT_ASSERT(state.is_string(-1));
        UNIT_ASSERT(!state.is_string(-2));

        UNIT_ASSERT_NO_EXCEPTION(state.is_string_strict(-1));
        UNIT_ASSERT_EXCEPTION(state.is_string_strict(-2), TLuaStateHolder::TError);

        // to_string

        UNIT_ASSERT_VALUES_EQUAL(state.to_string(-1), testValue);
        UNIT_ASSERT_VALUES_UNEQUAL(state.to_string(-2), testValue);

        UNIT_ASSERT_VALUES_EQUAL(state.to_string(-1, fallbackValue), testValue);
        UNIT_ASSERT_VALUES_EQUAL(state.to_string(-2, fallbackValue), fallbackValue);

        UNIT_ASSERT_VALUES_EQUAL(state.to_string_strict(-1), testValue);
        UNIT_ASSERT_EXCEPTION(state.to_string_strict(-2), TLuaStateHolder::TError);

        // pop_string

        UNIT_ASSERT_VALUES_EQUAL(state.pop_string(), testValue);
        UNIT_ASSERT_VALUES_UNEQUAL(state.pop_string(), testValue);

        state.push_nil();
        state.push_string(testValue);

        UNIT_ASSERT_VALUES_EQUAL(state.pop_string(fallbackValue), testValue);
        UNIT_ASSERT_VALUES_EQUAL(state.pop_string(fallbackValue), fallbackValue);

        state.push_nil();
        state.push_string(testValue);

        UNIT_ASSERT_VALUES_EQUAL(state.pop_string_strict(), testValue);
        UNIT_ASSERT_EXCEPTION(state.pop_string_strict(), TLuaStateHolder::TError);
        state.pop();
    }

    void TestNumber() {
        TLuaStateHolder state;
        state.BootStrap();

        TestNumber<double>(state, 12.3, 32.1);
        TestNumber<int>(state, 123, -321);
        TestNumber<size_t>(state, 456, 789);
    }

    template <typename T>
    void TestNumber(TLuaStateHolder& state, const T testValue, const T fallbackValue) {
        state.push_nil();
        state.push_string(ToString(testValue));
        state.push_number(testValue);

        // is_number

        UNIT_ASSERT(state.is_number(-1));
        UNIT_ASSERT(state.is_number(-2));
        UNIT_ASSERT(!state.is_number(-3));

        UNIT_ASSERT_NO_EXCEPTION(state.is_number_strict(-1));
        UNIT_ASSERT_NO_EXCEPTION(state.is_number_strict(-2));
        UNIT_ASSERT_EXCEPTION(state.is_number_strict(-3), TLuaStateHolder::TError);

        // to_number

        UNIT_ASSERT_VALUES_EQUAL(state.to_number<T>(-1), testValue);
        UNIT_ASSERT_VALUES_EQUAL(state.to_number<T>(-2), testValue);
        UNIT_ASSERT_VALUES_UNEQUAL(state.to_number<T>(-3), testValue);

        UNIT_ASSERT_VALUES_EQUAL(state.to_number<T>(-1, fallbackValue), testValue);
        UNIT_ASSERT_VALUES_EQUAL(state.to_number<T>(-2, fallbackValue), testValue);
        UNIT_ASSERT_VALUES_EQUAL(state.to_number<T>(-3, fallbackValue), fallbackValue);

        UNIT_ASSERT_VALUES_EQUAL(state.to_number_strict<T>(-1), testValue);
        UNIT_ASSERT_VALUES_EQUAL(state.to_number_strict<T>(-2), testValue);
        UNIT_ASSERT_EXCEPTION(state.to_number_strict<T>(-3), TLuaStateHolder::TError);

        // pop_number

        UNIT_ASSERT_VALUES_EQUAL(state.pop_number<T>(), testValue);
        UNIT_ASSERT_VALUES_EQUAL(state.pop_number<T>(), testValue);
        UNIT_ASSERT_VALUES_UNEQUAL(state.pop_number<T>(), testValue);

        state.push_nil();
        state.push_string(ToString(testValue));
        state.push_number(testValue);

        UNIT_ASSERT_VALUES_EQUAL(state.pop_number<T>(fallbackValue), testValue);
        UNIT_ASSERT_VALUES_EQUAL(state.pop_number<T>(fallbackValue), testValue);
        UNIT_ASSERT_VALUES_EQUAL(state.pop_number<T>(fallbackValue), fallbackValue);

        state.push_nil();
        state.push_string(ToString(testValue));
        state.push_number(testValue);

        UNIT_ASSERT_VALUES_EQUAL(state.pop_number_strict<T>(), testValue);
        UNIT_ASSERT_VALUES_EQUAL(state.pop_number_strict<T>(), testValue);
        UNIT_ASSERT_EXCEPTION(state.pop_number_strict<T>(), TLuaStateHolder::TError);
        state.pop();
    }

    void TestBool() {
        TLuaStateHolder state;
        state.BootStrap();

        state.push_nil();
        state.push_bool(false);
        state.push_bool(true);

        // is_bool

        UNIT_ASSERT(state.is_bool(-1));
        UNIT_ASSERT(state.is_bool(-2));
        UNIT_ASSERT(!state.is_bool(-3));

        UNIT_ASSERT_NO_EXCEPTION(state.is_bool_strict(-1));
        UNIT_ASSERT_NO_EXCEPTION(state.is_bool_strict(-2));
        UNIT_ASSERT_EXCEPTION(state.is_bool_strict(-3), TLuaStateHolder::TError);

        // to_bool

        UNIT_ASSERT_VALUES_EQUAL(state.to_bool(-1), true);
        UNIT_ASSERT_VALUES_EQUAL(state.to_bool(-2), false);
        UNIT_ASSERT_VALUES_EQUAL(state.to_bool(-3), false);

        UNIT_ASSERT_VALUES_EQUAL(state.to_bool(-1, false), true);
        UNIT_ASSERT_VALUES_EQUAL(state.to_bool(-2, true), false);
        UNIT_ASSERT_VALUES_EQUAL(state.to_bool(-3, true), true);

        UNIT_ASSERT_VALUES_EQUAL(state.to_bool_strict(-1), true);
        UNIT_ASSERT_VALUES_EQUAL(state.to_bool_strict(-2), false);
        UNIT_ASSERT_EXCEPTION(state.to_bool_strict(-3), TLuaStateHolder::TError);

        // pop_bool

        UNIT_ASSERT_VALUES_EQUAL(state.pop_bool(), true);
        UNIT_ASSERT_VALUES_EQUAL(state.pop_bool(), false);
        UNIT_ASSERT_VALUES_EQUAL(state.pop_bool(), false);

        state.push_nil();
        state.push_bool(false);
        state.push_bool(true);

        UNIT_ASSERT_VALUES_EQUAL(state.pop_bool(false), true);
        UNIT_ASSERT_VALUES_EQUAL(state.pop_bool(true), false);
        UNIT_ASSERT_VALUES_EQUAL(state.pop_bool(true), true);

        state.push_nil();
        state.push_bool(false);
        state.push_bool(true);

        UNIT_ASSERT_VALUES_EQUAL(state.pop_bool_strict(), true);
        UNIT_ASSERT_VALUES_EQUAL(state.pop_bool_strict(), false);
        UNIT_ASSERT_EXCEPTION(state.pop_bool_strict(), TLuaStateHolder::TError);
    }

    void TestNil() {
        TLuaStateHolder state;
        state.BootStrap();

        state.push_bool(true);
        state.push_nil();

        // is_nil

        UNIT_ASSERT(state.is_nil(-1));
        UNIT_ASSERT(!state.is_nil(-2));

        UNIT_ASSERT_NO_EXCEPTION(state.is_nil_strict(-1));
        UNIT_ASSERT_EXCEPTION(state.is_nil_strict(-2), TLuaStateHolder::TError);

        // pop_nil

        UNIT_ASSERT_VALUES_EQUAL(state.pop_nil(), true);
        UNIT_ASSERT_VALUES_EQUAL(state.pop_nil(), false);

        state.push_bool(true);
        state.push_nil();

        UNIT_ASSERT_NO_EXCEPTION(state.pop_nil_strict());
        UNIT_ASSERT_EXCEPTION(state.pop_nil_strict(), TLuaStateHolder::TError);
    }

    void TestVoid() {
        TLuaStateHolder state;
        state.BootStrap();

        int test1 = 0;
        int test2 = 0;
        TestVoid(state, &test1, &test2);
        TestVoid(state, &test2, &test1);
    }

    void TestVoid(TLuaStateHolder& state, void* testValue, void* fallbackValue) {
        state.push_nil();
        state.push_void(testValue);

        // is_void

        UNIT_ASSERT(state.is_void(-1));
        UNIT_ASSERT(!state.is_void(-2));

        UNIT_ASSERT_NO_EXCEPTION(state.is_void_strict(-1));
        UNIT_ASSERT_EXCEPTION(state.is_void_strict(-2), TLuaStateHolder::TError);

        // to_void

        UNIT_ASSERT_VALUES_EQUAL(state.to_void(-1), testValue);
        UNIT_ASSERT_VALUES_UNEQUAL(state.to_void(-2), testValue);

        UNIT_ASSERT_VALUES_EQUAL(state.to_void(-1, fallbackValue), testValue);
        UNIT_ASSERT_VALUES_EQUAL(state.to_void(-2, fallbackValue), fallbackValue);

        UNIT_ASSERT_VALUES_EQUAL(state.to_void_strict(-1), testValue);
        UNIT_ASSERT_EXCEPTION(state.to_void_strict(-2), TLuaStateHolder::TError);

        // pop_void

        UNIT_ASSERT_VALUES_EQUAL(state.pop_void(), testValue);
        UNIT_ASSERT_VALUES_UNEQUAL(state.pop_void(), testValue);

        state.push_nil();
        state.push_void(testValue);

        UNIT_ASSERT_VALUES_EQUAL(state.pop_void(fallbackValue), testValue);
        UNIT_ASSERT_VALUES_EQUAL(state.pop_void(fallbackValue), fallbackValue);

        state.push_nil();
        state.push_void(testValue);

        UNIT_ASSERT_VALUES_EQUAL(state.pop_void_strict(), testValue);
        UNIT_ASSERT_EXCEPTION(state.pop_void_strict(), TLuaStateHolder::TError);
        state.pop();
    }

    void TestTable() {
        TLuaStateHolder state;
        state.BootStrap();

        state.push_nil();
        state.create_table();

        UNIT_ASSERT(state.is_table(-1));
        UNIT_ASSERT(!state.is_table(-2));

        UNIT_ASSERT_NO_EXCEPTION(state.is_table_strict(-1));
        UNIT_ASSERT_EXCEPTION(state.is_table_strict(-2), TLuaStateHolder::TError);
    }

    void TestFunction() {
        TLuaStateHolder state;
        InitLua(state, UserfunctionFunctions);

        state.register_function("user", NLua::FunctionHandler<UserFunction>);
        state.push_global("test");
        state.call(0, 1);
        UNIT_ASSERT_VALUES_EQUAL(state.pop_number<int>(), 123);
    }

    void TestUserdata() {
        TLuaStateHolder state;
        InitLua(state, UserdataFunctions);

        LuaUserSum(state, 5, 10);
        LuaUserSum(state, 16, 87);
        LuaUserMax(state, 54, 63);
        LuaUserMax(state, 83, 1);
        LuaUserMult(state, 6, 9);
        LuaUserMult(state, 23, 67);
        LuaUserMult(state);
    }

    void TestUserdataMutable() {
        TLuaStateHolder state;
        InitLua(state, UserdataFunctions);

        LuaUserSet(state, 50);
        LuaUserSet(state, 100);
        LuaUserSet(state, "123", true);
        LuaUserSet(state, "456", true);
        LuaUserSet(state, "qwerty", false);
        LuaUserSet(state, "string argument", false);
    }

    void TestDump() {
        TBuffer compiled;
        NLua::Compile(SimpleFunctions, compiled);
        TBufferInput input(compiled);

        TLuaStateHolder state;
        InitLua(state, input);

        LuaGCD(state, 123, 321);
        LuaAvg(state, 123, 321);
        LuaStrlen(state, "test string");
    }

    void TestTimeLimit() {
        TLuaStateHolder state;
        InitLua(state, BadFunctions);

        const TDuration limit = TDuration::MilliSeconds(10);
        const TDuration testTime = TDuration::MilliSeconds(1000);

        {
            TSimpleTimer Timer;
            UNIT_ASSERT_EXCEPTION(LuaTooLong(state, true, limit), TLuaStateHolder::TError);
            const TDuration duration = Timer.Get();
            UNIT_ASSERT(duration >= limit);
            UNIT_ASSERT(duration <= testTime);
        }

        {
            TSimpleTimer Timer;
            UNIT_ASSERT_NO_EXCEPTION(LuaTooLong(state, false, limit));
            const TDuration duration = Timer.Get();
            UNIT_ASSERT(duration <= testTime);
        }

        UNIT_ASSERT_EXCEPTION(LuaTooCount(state, 10), TLuaStateHolder::TError);
        UNIT_ASSERT_NO_EXCEPTION(LuaTooCount(state, 20));
    }

    void TestMemoryLimit() {
        TLuaStateHolder state(1024 * 1024);
        InitLua(state, BadFunctions);

        UNIT_ASSERT_EXCEPTION(LuaTooBig(state, 10000), TLuaStateHolder::TError);
        UNIT_ASSERT_NO_EXCEPTION(LuaTooBig(state, 100));
    }

    void TestDestructor() {
        int count1 = 0;
        int count2 = 0;

        {
            TLuaStateHolder state;
            InitLua(state, SetFunctions);

            state.push_global("set");
            state.push_userdata<TDestructable>(&count1);
            state.call(1, 0);
            UNIT_ASSERT_VALUES_EQUAL(count1, 0);

            state.push_global("set");
            state.push_userdata<TDestructable>(&count2);
            state.call(1, 0);
            UNIT_ASSERT_VALUES_EQUAL(count2, 0);
        }

        UNIT_ASSERT_VALUES_EQUAL(count1, 1);
        UNIT_ASSERT_VALUES_EQUAL(count2, 1);
    }

private:
    static void InitLua(TLuaStateHolder& state, const TStringBuf& script) {
        TMemoryInput input(script.data(), script.size());
        InitLua(state, input);
    }

    static void InitLua(TLuaStateHolder& state, IInputStream& input) {
        state.BootStrap();
        state.Load(&input, "main");
        state.call(0, 0);
    }

    static void LuaAvg(TLuaStateHolder& state, double a, double b) {
        state.push_global("avg");
        state.push_number(a);
        state.push_number(b);
        state.call(2, 1);
        const double res = state.pop_number<double>();

        UNIT_ASSERT_VALUES_EQUAL(res, (a + b) / 2);
    }

    static void LuaStrlen(TLuaStateHolder& state, const TString& s) {
        state.push_global("str_len");
        state.push_string(s.data());
        state.call(1, 1);
        const int res = state.pop_number<int>();

        UNIT_ASSERT_VALUES_EQUAL(size_t(res), s.size());
    }

    static void LuaGCD(TLuaStateHolder& state, int a, int b) {
        state.push_global("gcd");
        state.push_number(a);
        state.push_number(b);
        state.call(2, 1);
        const int res = state.pop_number<int>();

        UNIT_ASSERT_VALUES_EQUAL(res, GCD(a, b));
    }

    static void LuaTooLong(TLuaStateHolder& state, bool inf, TDuration time) {
        state.push_global("too_long");
        state.push_bool(inf);
        state.call(1, 1, time, 100);
        const bool res = state.pop_bool();

        UNIT_ASSERT_VALUES_EQUAL(res, inf);
    }

    static void LuaTooCount(TLuaStateHolder& state, int limit) {
        state.push_global("too_count");
        state.push_number(1);
        state.push_number(2);
        state.push_number(3);
        state.call(3, 1, limit);
        const int res = state.pop_number<int>(0);

        UNIT_ASSERT_VALUES_EQUAL(res, 1 + 2 + 3);
    }

    static void LuaTooBig(TLuaStateHolder& state, int n) {
        state.push_global("too_big");
        state.push_number(n);
        state.call(1, 1);

        const int res = state.pop_number<int>();

        UNIT_ASSERT_VALUES_EQUAL(res, n);
    }

    static void LuaUserSum(TLuaStateHolder& state, int a, int b) {
        state.push_global("sum");
        // copy objects to stack
        state.push_userdata(TUserData(a));
        state.push_userdata(TUserData(b));
        state.call(2, 1);
        const int res = state.pop_number<int>();

        UNIT_ASSERT_VALUES_EQUAL(res, a + b);
    }

    static void LuaUserMax(TLuaStateHolder& state, int a, int b) {
        state.push_global("max");
        // construct objects on stack
        state.push_userdata<TUserData>(a);
        state.push_userdata<TUserData>(b);
        state.call(2, 1);
        const TUserData res = state.pop_userdata_strict<TUserData>();

        UNIT_ASSERT_VALUES_EQUAL(res.X, Max(a, b));
    }

    static void LuaUserMult(TLuaStateHolder& state, int x, int a) {
        state.push_global("mult");
        // copy objects to stack
        state.push_userdata<TUserData>(x);
        state.push_number(a);
        state.call(2, 1);

        const int res = state.pop_number<int>();

        UNIT_ASSERT_VALUES_EQUAL(res, x * a);
    }

    static void LuaUserMult(TLuaStateHolder& state) {
        state.push_global("mult");
        // copy objects to stack
        state.push_userdata<TUserData>();
        state.push_nil();
        state.call(2, 1);

        UNIT_ASSERT_NO_EXCEPTION(state.pop_nil_strict());
    }

    static void LuaUserSet(TLuaStateHolder& state, int x) {
        state.push_global("set");
        state.push_userdata<TUserData>();
        state.push_number(x);
        state.call(2, 1);
        const TUserData res = state.pop_userdata_strict<TUserData>();
        UNIT_ASSERT_VALUES_EQUAL(res.X, x);
    }

    static void LuaUserSet(TLuaStateHolder& state, const TStringBuf& x, bool number) {
        state.push_global("set");
        state.push_userdata<TUserData>();
        state.push_string(x);
        if (number) {
            UNIT_ASSERT_NO_EXCEPTION(state.call(2, 1));
            const TUserData res = state.pop_userdata_strict<TUserData>();
            UNIT_ASSERT_VALUES_EQUAL(res.X, FromString<int>(x));
        } else {
            UNIT_ASSERT_EXCEPTION(state.call(2, 1), TLuaStateHolder::TError);
        }
    }

    static int GCD(int a, int b) {
        while (b != 0) {
            a = a % b;
            DoSwap(a, b);
        }
        return a;
    }

    static int UserFunction(TLuaStateHolder& state) {
        state.push_number(123);
        return 1;
    }

    static const TStringBuf SimpleFunctions;
    static const TStringBuf UserfunctionFunctions;
    static const TStringBuf UserdataFunctions;
    static const TStringBuf BadFunctions;
    static const TStringBuf SetFunctions;
};

const TStringBuf TLuaStateHolderTest::SimpleFunctions("\
function avg(a,b)\n\
 return (a+b)/2\n\
end\n\
\n\
function gcd(a,b)\n\
 while b~=0 do\n\
  a, b = b, a%b\n\
 end\n\
 return a\n\
end\n\
\n\
function str_len(s)\n\
 return string.len(s)\n\
end\n\
");

const TStringBuf TLuaStateHolderTest::UserfunctionFunctions("\
function test()\n\
 return user()\n\
end\n\
");

const TStringBuf TLuaStateHolderTest::UserdataFunctions("\
function sum(a,b)\n\
 return a:get() + b:get()\n\
end\n\
\n\
function max(a,b)\n\
 if a:get() > b:get() then\n\
  return a\n\
 else\n\
  return b\n\
 end\n\
end\n\
\n\
function mult(x,a)\n\
 return x:mult(a)\n\
end\n\
\n\
function set(x,a)\n\
 x:set(a)\n\
 return x\n\
end\n\
");

const TStringBuf TLuaStateHolderTest::BadFunctions("\
function too_long(x)\n\
 while x do end\n\
 return x\n\
end\n\
\n\
function too_big(n)\n\
 a={}\n\
 for i=1,n do\n\
  b = {}\n\
  for j=1,n do\n\
   b[j] = tostring(j)\n\
  end\n\
  a[tostring(i)] = b\n\
 end\n\
 return n\n\
end\n\
\n\
function too_count(...)\n\
 a, b, c = ...\n\
 if a>0 and b>0 and c>0 then\n\
  return a+b+c\n\
 end\n\
end\n\
");

const TStringBuf TLuaStateHolderTest::SetFunctions("\
function set(val)\n\
 local x = val\n\
 y = val\n\
 z = x\n\
end\n\
");

UNIT_TEST_SUITE_REGISTRATION(TLuaStateHolderTest);
