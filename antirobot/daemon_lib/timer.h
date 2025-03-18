#pragma once

#include <util/generic/typetraits.h>

namespace NAntiRobot {
    template <class T>
    class TTimer {
        public:
            inline TTimer()
                : P_(0)
            {
            }

            inline T Next(T c) noexcept {
                T ret = 0;

                if (c > P_) {
                    if (P_) {
                        ret = c - P_;
                    }

                    P_ = c;
                }

                return ret;
            }

        private:
            T P_;
    };
}

template <class T>
struct TPodTraits< NAntiRobot::TTimer<T> > {
    enum {
        IsPod = TTypeTraits<T>::IsPod
    };
};
