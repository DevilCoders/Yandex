#include "bad_user_agents.h"

#include <util/string/strip.h>

using namespace NAntiRobot;

void TBadUserAgents::Load(IInputStream& in, bool isCaseInsensitive) {
    using NRegExp::TFsm;

    TString line;

    Fsms.clear();
    Fsms.push_back(TFsm::False());

    size_t numPatternsInCurFsm = 0;
    const size_t MAX_PATTERNS_IN_FSM = 7;

    while (in.ReadLine(line)) {
        StripInPlace(line);
        if (!line) {
            continue;
        }

        if (line[0] == '#') {
            continue;
        }

        TFsm addFsm(line, TFsm::TOptions().SetCaseInsensitive(isCaseInsensitive).SetSurround(true));

        try {
            Fsms.back() = Fsms.back() | addFsm;
            ++numPatternsInCurFsm;
        } catch(...) {
            Fsms.push_back(addFsm);
            numPatternsInCurFsm = 1;
        }

        if (numPatternsInCurFsm >= MAX_PATTERNS_IN_FSM) {
            Fsms.push_back(TFsm::False());
            numPatternsInCurFsm = 0;
        }
    }
}
