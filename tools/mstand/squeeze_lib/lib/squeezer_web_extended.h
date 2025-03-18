#pragma once

#include "squeezer_common.h"


namespace NMstand {

class TWebDesktopExtendedActionsSqueezer : public TActionsSqueezer
{
public:
    NYT::TTableSchema GetSchema() const override;
    ui32 GetVersion() const override;
    bool CheckRequest(const NRA::TRequest* request) const override;
    void GetActions(TActionSqueezerArguments& args) const override;
};

class TWebTouchExtendedActionsSqueezer : public TWebDesktopExtendedActionsSqueezer
{
public:
    bool CheckRequest(const NRA::TRequest* request) const override;
};

}; // NMstand
