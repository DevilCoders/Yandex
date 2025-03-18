#pragma once

#include "misc_points.h"

#include <util/stream/output.h>

class TMasterOutputStream
   : public TMaster2ArgsPoint<const void*, size_t>,
      public IOutputStream {
private:
    typedef TMaster2ArgsPoint<const void*, size_t> TBase;

protected:
    void DoWrite(const void* buf, size_t len) override {
        Func(buf, len);
    }

public:
    TMasterOutputStream() {
    }
    TMasterOutputStream(TSimpleModule* module, const TString& names)
        : TBase(module, names)
    {
    }
};
