#pragma once

#if !defined(MATRIXNET_WITHOUT_ARCADIA)
#error This header file is intended for use in the MATRIXNET_WITHOUT_ARCADIA mode only.
#endif

#include <exception>
#include <sstream>
#include <cassert>

namespace NMatrixnet {

class yexception: public std::exception {
public:
    using TBase = std::exception;
    yexception() {}
    yexception(yexception&& src);

    const char* what() const noexcept override;

    template<typename T>
    yexception&& operator <<(const T& t) {
        Stream_ << t;
        return std::move(*this);
    }

private:
    std::stringstream Stream_;
    mutable std::string Message_;
};

class TFileError: public yexception {};
class TLoadEOF: public yexception {};
class TFromStringException: public yexception {};

#if defined(MATRIXNET_WITHOUT_ARCADIA_NO_EXCEPTIONS)
class TTerminateOnException {
public:
    TTerminateOnException() {}
    void operator ,(yexception&& e);
};

// We chose comma because it has the minimal priority among C++ operators.
#define ythrow NMatrixnet::TTerminateOnException(),

#else // !defined(MATRIXNET_WITHOUT_ARCADIA_NO_EXCEPTIONS)

#define ythrow throw

#endif // !defined(MATRIXNET_WITHOUT_ARCADIA_NO_EXCEPTIONS)

#define Y_ENSURE(CONDITION, DESCRIPTION)           \
    do {                                           \
        if (!(CONDITION)) {                        \
            ythrow yexception() << DESCRIPTION;    \
        }                                          \
    } while (0)

} // namespace NMatrixnet

#define Y_ASSERT(a) assert(a)
