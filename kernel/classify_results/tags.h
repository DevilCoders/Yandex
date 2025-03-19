#pragma once
#include <util/generic/string.h>
enum EResultTag {
    RT_NONE = 0,
    RT_PORN = 1,
    RT_EROTIC = 2,
    RT_COMMERCIAL = 4,
    RT_SHOCKING = 8,
};

struct TTagSnipTestData {
    EResultTag Tag;
    TString SnipFragments;
};
extern TTagSnipTestData TagSnipTests[];
