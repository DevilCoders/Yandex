#pragma once

#include <util/memory/blob.h>
#include <util/generic/vector.h>
#include <util/generic/string.h>

class TDocArchive;

int FindDocCommon(TBlob b, TBlob doctext, TVector<int>& breaks, TDocArchive& da);
