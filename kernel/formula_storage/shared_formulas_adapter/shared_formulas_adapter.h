#pragma once

#include <util/generic/fwd.h>
#include <util/generic/ptr.h>

namespace NMatrixnet {
    class IRelevCalcer;
};

class ISharedFormulasAdapter {
    using TBaseCalcer = NMatrixnet::IRelevCalcer;

public:
    using TBaseCalcerPtr = TAtomicSharedPtr<TBaseCalcer>;
    virtual TBaseCalcerPtr GetSharedFormula(const TStringBuf name) const = 0;

protected:
    virtual ~ISharedFormulasAdapter() = default;
};
