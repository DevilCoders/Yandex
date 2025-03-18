#pragma once

#include "misc_points.h"

#include <util/stream/input.h>

class TMasterInputStream
   : public TMasterAnswer2Point<void*, size_t, bool>,
      public IInputStream {
private:
    typedef TMasterAnswer2Point<void*, size_t, bool> TBase;

protected:
    size_t DoRead(void* buf, size_t len) override {
        return Answer(buf, len) ? len : 0;
    }

public:
    TMasterInputStream() {
    }
    TMasterInputStream(TSimpleModule* module, const TString& names)
        : TBase(module, names)
    {
    }
};
