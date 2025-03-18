#pragma once

#include <util/system/defaults.h>

namespace NAntiRobot {
    template<class T, size_t MaxItems>
    class TCyclicQueue {
    public:
        TCyclicQueue()
            : Begin(0)
            , End(0)
        {
        }

        size_t GetSize() const {
            if (Begin == 0)
                return End;

            return MaxItems;
        }

        void Clear() {
            Begin = 0;
            End = 0;
        }

        void Push(const T& x) {
            size_t newEnd = End + 1;
            if (newEnd > MaxItems)
                newEnd = 0;

            if (newEnd == Begin) {
                ++Begin;
                if (Begin > MaxItems)
                    Begin = 0;
            }

            Items[End] = x;
            End = newEnd;
        }

        const T& GetItem(size_t index) const {
            return Items[(Begin + index) % (MaxItems + 1)];
        }

        const T& GetLast() const {
            return Items[End > 0 ? End - 1 : MaxItems];
        }
    private:
        T Items[MaxItems + 1] = {};
        size_t Begin;
        size_t End;
    };
}
