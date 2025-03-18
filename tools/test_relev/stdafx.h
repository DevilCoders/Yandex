#pragma once

//#include <fstream>
//#include <tchar.h>
//#include <windows.h>

#ifndef _WIN32
#   include <unistd.h>
#endif

#undef ASSERT
#ifdef _DEBUG
#  define ASSERT( a ) if ( !(a) ) __debugbreak();
#else
#  define ASSERT( a ) ((void)0)
#endif

#include <util/generic/vector.h>
#include <util/generic/hash.h>
#include <util/stream/file.h>
#include <string>
using std::string;
