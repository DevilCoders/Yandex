#pragma once

#include "base.h"

namespace NRemorphAPI {

namespace NImpl {

class TInfo: public TBase, public IInfo {
public:
    TInfo();

    unsigned int GetMajorVersion() const override;
    unsigned int GetMinorVersion() const override;
    unsigned int GetPatchVersion() const override;
};

} // NImpl

} // NRemorphAPI
