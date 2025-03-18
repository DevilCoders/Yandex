#pragma once

#include "request.h"

#include <util/memory/tempbuf.h>

namespace NCommunism {

void Solve(TRequestsHandle Requests, TInstant &WaitingDeadline, NGlobal::TPoller &Poller);

}
