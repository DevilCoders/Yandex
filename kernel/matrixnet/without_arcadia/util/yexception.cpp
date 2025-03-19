#include "yexception.h"

#if defined(MATRIXNET_WITHOUT_ARCADIA_NO_EXCEPTIONS)
#include <iostream>
#endif // defined(MATRIXNET_WITHOUT_ARCADIA_NO_EXCEPTIONS)

namespace NMatrixnet {

yexception::yexception(yexception&& src)
    : TBase(src), Stream_(std::move(src.Stream_)) {
}

const char* yexception::what() const noexcept {
    Message_ = Stream_.str();
    return Message_.c_str();
}

#if defined(MATRIXNET_WITHOUT_ARCADIA_NO_EXCEPTIONS)
void TTerminateOnException::operator ,(yexception&& e) {
    std::cerr << e.what() << std::endl;
    std::exit(EXIT_FAILURE);
}
#endif // defined(MATRIXNET_WITHOUT_ARCADIA_NO_EXCEPTIONS)

} // namespace NMatrixnet
