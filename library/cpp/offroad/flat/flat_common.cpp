#include "flat_common.h"

#include <util/generic/yexception.h>

namespace NOffroad {
    namespace NPrivate {
        void ThrowFlatSearcherKeyTooLongException() {
            ythrow yexception() << "Keys longer than 56 bits are not supported.";
        }

        void ThrowFlatSearcherDataTooLongException() {
            ythrow yexception() << "Values longer than 255 bits are not supported.";
        }

    }
}
