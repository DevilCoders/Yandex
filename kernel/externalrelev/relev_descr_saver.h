#pragma once

#include <util/generic/fwd.h>

class IRelevDescrSaver {
protected:
    virtual ~IRelevDescrSaver() {}
public:
    virtual void AddDescr(const TString& msg) = 0;
};
