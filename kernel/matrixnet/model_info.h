#pragma once

#if defined(MATRIXNET_WITHOUT_ARCADIA)
#include "without_arcadia/util/map.h"
#include "without_arcadia/util/stroka.h"
#else // !defined(MATRIXNET_WITHOUT_ARCADIA)
#include <util/generic/map.h>
#include <util/generic/string.h>
#endif // !defined(MATRIXNET_WITHOUT_ARCADIA)

namespace NMatrixnet {

typedef TMap<TString, TString> TModelInfo;

}
