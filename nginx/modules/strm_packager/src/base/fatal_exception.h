#pragma once

#include <exception>

namespace NStrm::NPackager {
    class TFatalExceptionContainer: std::exception {
    public:
        TFatalExceptionContainer(std::exception_ptr exception)
            : Exception_(exception)
        {
        }

        char const* what() const noexcept {
            return "TFatalExceptionContainer";
        }

        std::exception_ptr Exception() const {
            return Exception_;
        }

    private:
        std::exception_ptr Exception_;
    };
}
