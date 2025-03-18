#include "options.h"

#include <antirobot/daemon_lib/antirobot_service.h>

#include <util/generic/strbuf.h>
#include <util/generic/yexception.h>

namespace NAntiRobot {

namespace {
const char PRINT_REQ_OPTION = 'p';
const char* ANTIROBOT_SERVICE_OPTION = "antirobot";
const char* REQUEST_COUNT_OPTION = "reqs";
const char* PROCESSOR_COUNT_OPTION = "processors";
}

bool JustPrintRequest(const NLastGetopt::TOptsParseResult& res) {
    return res.Has(PRINT_REQ_OPTION);
}

NLastGetopt::TOpts CreateOptions() {
    NLastGetopt::TOpts opts = NLastGetopt::TOpts::Default();
    opts.AddCharOption(PRINT_REQ_OPTION, NLastGetopt::NO_ARGUMENT, "Print request to stdout");
    opts.AddLongOption(ANTIROBOT_SERVICE_OPTION, "Address of antirobot service").DefaultValue(NAntiRobot::ANTIROBOT_SERVICE_HOST);
    opts.AddLongOption(PROCESSOR_COUNT_OPTION, "Number of processing backends to output").DefaultValue("1");
    /* Значение по умолчанию 30 в следующем параметре взято не с потолка.
     * Пусть у нас есть K вертикалей, в каждую вертикаль запрос попадает с вероятностью 1/K.
     * Пусть P(i, j) - вероятность того, что после отправки i запросов, в каждую из j вертикалей
     * попадёт хотя бы один запрос. P(i, j) = P(i - 1, j - 1) * (j / K) + P(i - 1, j) * ((K - j) / K)
     *
     * У нас 4 вертикали (msk, sas, ams, man), так что K = 4. Осталось найти такое N, что P(N, K)
     * будет достаточно большим. P(30, 4) > 0.999, так что по принципу о практической невозможности
     * маловероятных событий, 30 запросов будет всегда достаточно, чтобы хотя бы раз попасть во
     * все вертикали.
     */
    opts.AddLongOption(REQUEST_COUNT_OPTION, "Number of requests to send to antirobot service").DefaultValue("30");

    opts.SetFreeArgsNum(1);
    opts.SetFreeArgTitle(0, "<IPv4 or IPv6>", "IP for the backend is looked for");
    return opts;
}

THostAddr GetAntirobotService(const NLastGetopt::TOptsParseResult& res) {
    return CreateHostAddr(res.Get(ANTIROBOT_SERVICE_OPTION));
}

int GetRequestCount(const NLastGetopt::TOptsParseResult& res) {
    return res.Get<int>(REQUEST_COUNT_OPTION);
}

int GetProcessorCount(const NLastGetopt::TOptsParseResult& res) {
    return res.Get<int>(PROCESSOR_COUNT_OPTION);
}

}
