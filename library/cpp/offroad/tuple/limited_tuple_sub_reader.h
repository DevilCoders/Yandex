#pragma once

#include <util/memory/blob.h>

#include "limited_tuple_sub_seeker.h"

namespace NOffroad {
    template <class Vectorizer, class Base>
    class TLimitedTupleSubReader: public Base {
        using TSeeker = TLimitedTupleSubSeeker<typename Base::THit, Vectorizer>;

    public:
        using THit = typename Base::THit;
        using TPosition = TTupleSubOffset;

        TLimitedTupleSubReader() = default;

        template <class... Args>
        TLimitedTupleSubReader(const TArrayRef<const char>& subSource, Args&&... args)
            : Base(std::forward<Args>(args)...)
            , Seeker_(subSource)
        {
        }

        void Reset() {
            Base::Reset();
            Seeker_.Reset();
        }

        template <class... Args>
        void Reset(const TArrayRef<const char>& subSource, Args&&... args) {
            Base::Reset(std::forward<Args>(args)...);
            Seeker_.Reset(subSource);
        }

        template <class... Args>
        void Reset(const TBlob& subSource, Args&&... args) {
            Base::Reset(std::forward<Args>(args)...);
            Seeker_.Reset(subSource);
        }

        template <class... Args>
        Y_FORCE_INLINE bool Seek(const TTupleSubOffset& position, Args&&... args) {
            return Base::Seek(position.Offset(), std::forward<Args>(args)...);
        }

        bool LowerBound(const THit& prefix, THit* first) {
            return Seeker_.LowerBound(prefix, StartLimit_, EndLimit_, first, this);
        }

        void SetLimits(const TTupleSubOffset& startLimit, const TTupleSubOffset& endLimit) {
            Base::SetLimits(startLimit.Offset(), endLimit.Offset());
            StartLimit_ = startLimit;
            EndLimit_ = endLimit;
        }

    private:
        TSeeker Seeker_;
        TTupleSubOffset StartLimit_;
        TTupleSubOffset EndLimit_;
    };

}
