#include "urllen.h"

#include <util/generic/utility.h> // for ClampVal

ui8 CalcUrlLen (const TStringBuf url) {
    return ClampVal((ui32)url.length() / 5, (ui32)0, (ui32)255);
}
