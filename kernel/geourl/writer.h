#pragma once

#include <util/generic/fwd.h>

class IOutputStream;

void WriteGeoUrl(const TString& folder, IOutputStream* out);
