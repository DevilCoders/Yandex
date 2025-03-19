#pragma once

#include "services.h"

#include <util/generic/strbuf.h>

bool IsNonCommonQuery(TStringBuf cgiParamsStr, EMainServiceSpec serv, bool allowRstr, bool isSiteSearch = false, bool isMailRu = false);

bool IsNonCommonSearchUrl(TStringBuf searchUrl, EMainServiceSpec serv, bool allowRstr, bool isSiteSearch = false, bool isMailRu = false);
