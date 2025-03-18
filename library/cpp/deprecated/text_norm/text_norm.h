#pragma once

#include <util/generic/string.h>

/** \file text_norm.h
 *
 * Omnidex text normalization function
 *
 * See examples at ut/omni_ut.cpp:OmnidexNormalizeTextSuite
 *
 */

namespace NOmni {

    TUtf16String NormalizeText(const TUtf16String& text);

} // NOmni
