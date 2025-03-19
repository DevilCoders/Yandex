#pragma once

#include <util/generic/string.h>
#include <util/system/defaults.h>

class IId2String {
public:
    virtual TString GetString(ui32 id) = 0;
    virtual ~IId2String() {};
};

class TId2Empty : public IId2String {
public:
    TString GetString(ui32 /*id*/) override {
        return TString();
    }
};
