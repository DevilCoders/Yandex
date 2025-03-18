#pragma once

#include "tags.h"

struct TTagStrings {
    HT_TAG Tag;
    unsigned OpenLen;
    const char* Open;
    unsigned CloseLen;
    const char* Close;
};

extern TTagStrings TagStrings[];
