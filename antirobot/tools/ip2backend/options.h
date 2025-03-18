#pragma once

#include "ip2backend.h"

#include <antirobot/lib/host_addr.h>

#include <library/cpp/getopt/last_getopt.h>

namespace NAntiRobot {

NLastGetopt::TOpts CreateOptions();
bool JustPrintRequest(const NLastGetopt::TOptsParseResult& res);
THostAddr GetAntirobotService(const NLastGetopt::TOptsParseResult& res);
int GetRequestCount(const NLastGetopt::TOptsParseResult& res);
int GetProcessorCount(const NLastGetopt::TOptsParseResult& res);

}
