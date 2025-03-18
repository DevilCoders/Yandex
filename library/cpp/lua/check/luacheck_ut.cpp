#include "luacheck.h"

#include <library/cpp/lua/luajit-if-no-sanitizers/lua.h>
#include <library/cpp/testing/unittest/registar.h>

class TLuacheckTest: public TTestBase {
    UNIT_TEST_SUITE(TLuacheckTest);
    UNIT_TEST(TestCheckLuaCode)
    UNIT_TEST_SUITE_END();

public:
    void TestCheckLuaCode() {
        lua_State* L = luaL_newstate();
        luaL_openlibs(L);

        {
            TString messages;
            bool ok = CheckLuaCode(L, "print(42)", messages);
            UNIT_ASSERT(ok);
        }

        {
            TString messages;
            bool ok = CheckLuaCode(L, "x=1", messages);
            UNIT_ASSERT(!ok);
        }

        {
            TString messages;
            bool ok = CheckLuaCode(L, "print(", messages);
            UNIT_ASSERT(!ok);
        }

        lua_close(L);
    }
};

UNIT_TEST_SUITE_REGISTRATION(TLuacheckTest);
