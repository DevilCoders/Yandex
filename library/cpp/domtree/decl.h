#pragma once

#include <util/generic/ptr.h>

namespace NDomTree {
    enum class EDomNodeType {
        NT_Element = 0,
        NT_Text = 1
    };

    class IDomTree;

    using TDomTreePtr = TSimpleSharedPtr<const IDomTree>;
}
