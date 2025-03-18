#include "token_markup.h"

namespace NTokenClassification {
    TInterval::TInterval()
        : Begin(nullptr)
        , End(nullptr)
    {
    }

    TInterval::TInterval(const wchar16* begin, const wchar16* end)
        : Begin(begin)
        , End(end)
    {
        Y_ASSERT(Begin <= End);
    }

    size_t TInterval::Length() const {
        Y_ASSERT(Initialized());
        return End - Begin;
    }

    bool TInterval::Empty() const {
        return Begin == End;
    }

    bool TInterval::Initialized() const {
        return Begin != nullptr && End != nullptr;
    }

}
