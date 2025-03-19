#pragma once

#include "base.h"

#include <util/generic/string.h>
#include <util/memory/blob.h>

namespace NRemorphAPI {

namespace NImpl {

class TBlob: public TBase, public IBlob {
private:
    ::TBlob Blob;

public:
    TBlob(const ::TBlob& blob);
    TBlob(const TString& str);

    const char* GetData() const override;
    unsigned long GetSize() const override;
};

} // NImpl

} // NRemorphAPI
