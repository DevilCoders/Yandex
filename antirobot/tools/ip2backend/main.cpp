#include "ip2backend.h"
#include "options.h"

#include <util/generic/strbuf.h>
#include <util/generic/yexception.h>

#include <cstdio>

/*
 * Описание см. здесь: http://wiki.yandex-team.ru/JandeksPoisk/Sepe/antirobotonline/theory/Tools
 */

using namespace NAntiRobot;

int main(int argc, char* argv[]) {
    try {
        NLastGetopt::TOpts opts = CreateOptions();
        NLastGetopt::TOptsParseResult res(&opts, argc, argv);

        const TString ipAddr = res.GetFreeArgs()[0];
        const THostAddr antirobotService = GetAntirobotService(res);
        const auto request = MakeRequest(ipAddr, antirobotService, GetProcessorCount(res));

        if (JustPrintRequest(res)) {
            puts(ToString(request).c_str());
        } else {
            puts(Ip2Backend(antirobotService, request, GetRequestCount(res)).c_str());
        }

        return 0;
    } catch (...) {
        fprintf(stderr, "Failed to retrieve address\nReason: %s\n", CurrentExceptionMessage().c_str());
        return 2;
    }
}
