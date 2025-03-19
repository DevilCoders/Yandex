#include "handler.h"

void TCommonRequestHandler::DoAuthProcess(TJsonReport::TGuard& g, IAuthInfo::TPtr authInfo) {
    ProcessHttpRequest(g, authInfo);
}
