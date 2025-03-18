#include "luacheck.h"

#include <library/cpp/lua/luajit-if-no-sanitizers/lua.h>
#include <library/cpp/resource/registry.h>

const int LUA_NO_ERROR = 0;

class TLuaStackGuard {
public:
    TLuaStackGuard(lua_State* l)
        : L(l)
        , Top(lua_gettop(L))
    {
    }

    ~TLuaStackGuard() {
        lua_settop(L, Top);
    }

private:
    lua_State* L;
    int Top;
};

static bool IsLuacheckLoaded(lua_State* L) {
    // check existance of package.preload.luacheck
    lua_getglobal(L, "package");
    luaL_checktype(L, -1, LUA_TTABLE);
    lua_getfield(L, -1, "preload");
    luaL_checktype(L, -1, LUA_TTABLE);
    lua_getfield(L, -1, "luacheck");
    return lua_type(L, -1) == LUA_TTABLE;
}

static void LoadFile(lua_State* L, const char* module) {
    // package.preload[module] = load(module)
    lua_getglobal(L, "package");
    luaL_checktype(L, -1, LUA_TTABLE);
    lua_getfield(L, -1, "preload");
    luaL_checktype(L, -1, LUA_TTABLE);
    int preload_index = lua_gettop(L);
    TString code = NResource::Find(module);
    if (luaL_loadstring(L, code.c_str()) != LUA_NO_ERROR) {
        luaL_error(L, "Failed to load file %s", module);
    }
    lua_setfield(L, preload_index, module);
}

static void LoadLuacheck(lua_State* L) {
    static const char* MODULES[] = {
        "luacheck",
        "luacheck.analyze",
        "luacheck.argparse",
        "luacheck.cache",
        "luacheck.check",
        "luacheck.config",
        "luacheck.core_utils",
        "luacheck.expand_rockspec",
        "luacheck.filter",
        "luacheck.format",
        "luacheck.fs",
        "luacheck.globbing",
        "luacheck.inline_options",
        "luacheck.lexer",
        "luacheck.linearize",
        "luacheck.main",
        "luacheck.multithreading",
        "luacheck.options",
        "luacheck.parser",
        "luacheck.reachability",
        "luacheck.stds",
        "luacheck.utils",
        "luacheck.version",
        "luacheck.cpp.wrapper",
        NULL,
    };
    for (const char** it = MODULES; *it != NULL; ++it) {
        LoadFile(L, *it);
    }
}

static void LoadChecker(lua_State* L) {
    lua_getglobal(L, "require");
    lua_pushliteral(L, "luacheck.cpp.wrapper");
    lua_call(L, 1, 1);
}

struct TCheck {
    const TString& Code;
    TString& Message;
    bool Result;
};

static void CallChecker(lua_State* L, TCheck& check) {
    lua_pushstring(L, check.Code.data());
    lua_call(L, 1, 2);
    check.Result = lua_toboolean(L, -2);
    check.Message = lua_tostring(L, -1);
}

static int lua_CheckLuaCode(lua_State* L) {
    if (!IsLuacheckLoaded(L)) {
        LoadLuacheck(L);
    }
    LoadChecker(L);
    luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
    TCheck* check = reinterpret_cast<TCheck*>(lua_touserdata(L, 1));
    CallChecker(L, *check);
    return 0;
}

bool CheckLuaCode(lua_State* L, const TString& code, TString& message) {
    TLuaStackGuard guard(L);
    TCheck check = {code, message, false};
    if (lua_cpcall(L, lua_CheckLuaCode, &check) != LUA_NO_ERROR) {
        // error calling luacheck
        if (lua_type(L, -1) == LUA_TSTRING) {
            message = lua_tostring(L, -1);
        } else {
            message = "Luacheck failed and type of error is ";
            message += lua_typename(L, -1);
        }
        return false;
    }
    return check.Result;
}
