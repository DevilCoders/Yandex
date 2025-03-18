#pragma once

#include <boost/optional/optional.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace testing::internal {
    template<typename T>
    class UniversalPrinter<boost::optional<T>> {
    public:
        static void Print(const boost::optional<T>& value, ::std::ostream* os) {
            *os << '(';
            if (!value) {
                *os << "nullopt";
            } else {
                UniversalPrint(*value, os);
            }
            *os << ')';
        }
    };
}
