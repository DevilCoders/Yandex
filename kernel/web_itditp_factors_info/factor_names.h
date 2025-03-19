#pragma once

#include <kernel/factors_info/factors_info.h>

// This header file is auto-generated from factors_gen.in
#include <kernel/web_itditp_factors_info/factors_gen.h>


class IFactorsInfo;

namespace NWebItdItp {

const IFactorsInfo* GetWebItdItpFactorsInfo();
const size_t N_FACTOR_COUNT = NWebItdItp::FI_FACTOR_COUNT;

} // namespace NWebItdItp
