#pragma once

#include <kernel/dater/dater.h>
#include <library/cpp/deprecated/dater_old/scanner/dater.h>

namespace ND2 {

void ConvertUrlDates(const TDates& urldates, NDater::TDatePositions& res);
void ConvertUrlDates(const TDates& urldates, NDater::TDateCoords& res);
void ConvertTitleDates(const TDates& titledates, NDater::TDatePositions& res);
void ConvertTextDates(const TDaterDocumentContext& ctx, const TDates& textdates, NDater::TDatePositions& res);

}
