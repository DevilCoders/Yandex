#pragma once

#include <util/generic/yexception.h>

namespace NAntiRobot {
    class TForwardingFailure: public yexception {
    };

    class TTimeoutException: public yexception {
    public:
        TTimeoutException() {
            (*this) << "Request timeout";
        }
    };

    class TNehQueueOverflowException: public yexception {
    };

    class THttpCodeException: public yexception {
    public:
        THttpCodeException(ui32 code);

        ui32 GetCode() const {
            return Code;
        }

    private:
        ui32 Code;
    };

    class TCaptchaGenerationError : public yexception {
    };

}
