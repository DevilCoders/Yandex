#pragma once

#include "cache_sync.h"
#include "forwarding_stats.h"
#include "time_stats.h"

#include <antirobot/lib/error.h>
#include <antirobot/lib/ip_map.h>

#include <library/cpp/threading/future/future.h>

#include <util/generic/vector.h>
#include <util/generic/yexception.h>

#include <functional>

class IOutputStream;


namespace NAntiRobot {


class TAddr;
class THttpRequest;
struct TRequestContext;
struct TCaptchaInputInfo;

void ForwardRequestAsync(const TRequestContext& rc, const TString& location);
void ForwardCaptchaInputAsync(const TCaptchaInputInfo& captchaInputInfo, const TString& location);


} // namespace NAntiRobot
