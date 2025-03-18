#pragma once
#include <antirobot/lib/http_request.h>

#include <util/generic/string.h>

/**
 * @return info about @arg request as it parsed by Antirobot
 * @param antirobot host
 * @param send request as fullreq (if ask directly to antirobot instance)
 * @param request to inspect
 */
TString GetReqInfo(const TString& hostPort, bool asFullReq, const TString& request);
