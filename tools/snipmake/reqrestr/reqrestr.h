#pragma once

#include <library/cpp/cgiparam/cgiparam.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NSnippets {
    void AddRestrToQtrees(TCgiParameters& cgi, const TString& restr);
    void AddRestrToQtrees(TCgiParameters& cgi, const TVector<TString>& urls);
}
