#include "string_cast.h"
#include "yexception.h"

namespace NMatrixnet {

template<>
std::string ToString<double>(const double& value) {
    return std::to_string(value);
}

template<>
double FromString<double>(const std::string& s) {
    std::istringstream istrm(s);
    double ret;
    istrm >> ret;

    if (istrm.fail() || !istrm.eof())
        ythrow TFromStringException() << "cannot parse float(" << s << ")";

    return ret;
}

} // namespace NMatrixnet
