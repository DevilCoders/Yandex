#pragma once

#include <library/cpp/json/json_value.h>

#include <util/generic/ptr.h>

class TCollectServerInfo;
using TServerInfo = NJson::TJsonValue;

TServerInfo CollectServerInfo(TAutoPtr<TCollectServerInfo> collector);
