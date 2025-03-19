#include "position.h"

namespace {
    class TInvalidPosition
        : public NReqBundleIterator::TPosition
    {
    public:
        TInvalidPosition() {
            SetInvalid();
        }
    };
} // namespace

namespace NReqBundleIterator {
    const TPosition TPosition::Invalid = TInvalidPosition{};
} // NReqBundleIterator
