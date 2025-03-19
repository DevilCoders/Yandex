#pragma once

#include <util/system/types.h>

namespace NTextMarks {

inline constexpr char HILITE_OPEN[] = "\007[";
inline constexpr char HILITE_CLOSE[] = "\007]";

inline constexpr wchar16 HILITE_OPEN_WIDE[] = {wchar16('\007'), wchar16('['), wchar16(0)};
inline constexpr wchar16 HILITE_CLOSE_WIDE[] = {wchar16('\007'), wchar16(']'), wchar16(0)};

}
