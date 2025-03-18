#pragma once

#include "sub_seeker.h"

namespace NOffroad {
    template <class Vectorizer, class Base>
    class TSubReader: public Base {
        using TSeeker = TSubSeeker<typename Base::THit, Vectorizer>;

    public:
        using TData = typename Base::THit;

        TSubReader() {
        }

        template <class... Args>
        TSubReader(const TArrayRef<const char>& subSource, Args&&... args)
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

        using Base::Seek;

        bool LowerBound(const TData& prefix, TData* first) {
            return Seeker_.LowerBound(prefix, first, this);
        }

    private:
        TSeeker Seeker_;
    };

}
