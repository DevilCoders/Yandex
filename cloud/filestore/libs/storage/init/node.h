#pragma once

#include "public.h"

#include <cloud/storage/core/libs/diagnostics/logging.h>
#include <cloud/storage/core/libs/kikimr/public.h>

#include <library/cpp/actors/core/defs.h>

#include <util/generic/maybe.h>
#include <util/generic/string.h>

#include <utility>

namespace NCloud::NFileStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

struct TRegisterDynamicNodeOptions
{
    TString Domain;
    TString SchemeShardDir;
    TString NodeType;

    TString NodeBrokerAddress;
    ui32 NodeBrokerPort = 0;

    ui32 InterconnectPort = 0;

    TString DataCenter;
    TString Rack = 0;
    ui64 Body = 0;

    bool LoadCmsConfigs = false;
};

////////////////////////////////////////////////////////////////////////////////

using TRegisterDynamicNodeResult = std::tuple<ui32, NActors::TScopeId>;

////////////////////////////////////////////////////////////////////////////////

TRegisterDynamicNodeResult RegisterDynamicNode(
    NKikimrConfig::TAppConfigPtr appConfig,
    const TRegisterDynamicNodeOptions& options,
    TLog& Log);

}   // namespace NCloud::NFileStore::NStorage
