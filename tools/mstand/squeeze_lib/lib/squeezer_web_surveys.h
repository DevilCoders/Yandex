#pragma once

#include "squeezer_common.h"


namespace NMstand {

class TWebSurveysActionsSqueezer : public TActionsSqueezer
{
public:
    NYT::TTableSchema GetSchema() const override;
    ui32 GetVersion() const override;
    bool CheckRequest(const NRA::TRequest* request) const override;
    void GetActions(TActionSqueezerArguments& args) const override;
};

}; // NMstand
