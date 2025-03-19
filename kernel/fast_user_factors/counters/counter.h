#pragma once

#include <util/generic/ptr.h>

class IInputStream;
class IOutputStream;

namespace NFastUserFactors {

    class ICounter {
    public:
        virtual ~ICounter() = default;

        virtual void Add(const time_t ts, const float value) noexcept = 0;
        virtual float Accumulate() const noexcept = 0;
        virtual void Move(const time_t ts) noexcept = 0;
        virtual ui64 LastTs() const noexcept = 0;
        virtual void Clear() noexcept = 0;

        virtual void Save(IOutputStream& out) const = 0;
        virtual void Load(IInputStream& in) = 0;
    };

    using TCounterPtr = TSimpleSharedPtr<ICounter>;

}
