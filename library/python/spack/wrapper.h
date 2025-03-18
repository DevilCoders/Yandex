#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/string.h>

TString ConvertJsonSensorsToSpackV1(TStringBuf jsonData);
TString ConvertSpackV1SensorsToJson(TStringBuf spackData);
