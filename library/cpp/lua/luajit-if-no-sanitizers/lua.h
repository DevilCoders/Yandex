#pragma once

#ifdef NO_SANITIZERS_SO_USE_LUAJIT
#include <contrib/libs/luajit/lua.hpp>
#else
#include <contrib/libs/lua/lua.h>
#endif
