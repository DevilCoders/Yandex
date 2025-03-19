#pragma once

#include "enums.h"

#include <library/cpp/containers/dictionary/dictionary.h>

namespace NSe
{
    TVector< std::pair<const char*, ESearchEngine> > GetSearchEngineNames();
    TVector< std::pair<const char*, ESearchType> >   GetSearchTypeNames();
    TVector< std::pair<const char*, ESearchFlags> >  GetSearchFlagNames();
    TVector< std::pair<const char*, EPlatform> >     GetPlatformNames();
};
