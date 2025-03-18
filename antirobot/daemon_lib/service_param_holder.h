#pragma once

#include "req_types.h"

#include <array>

namespace NAntiRobot {

template<class T>
class TServiceParamHolder {
public:
    const T& GetByService(size_t service) const {
        return Values[service];
    }

    T& GetByService(size_t service) {
        return Values[service];
    }

    std::array<T, EHostType::HOST_NUMTYPES>& GetArray() {
        return Values;
    }

private:
    std::array<T, EHostType::HOST_NUMTYPES> Values{};
};

}
