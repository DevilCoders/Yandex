#pragma once

#include "index_format.h"

#include <util/generic/string.h>

namespace NDoom {

EIndexFormat DetectIndexFormat(const TString& indexPath);

} // namespace NDoom
