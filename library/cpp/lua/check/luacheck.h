#pragma once

#include <util/generic/string.h>

struct lua_State;

bool CheckLuaCode(lua_State* L, const TString& code, TString& message);
