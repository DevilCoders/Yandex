#pragma once


template <bool PCondition, typename T>
struct TTypeConstIf {
    typedef std::conditional_t<PCondition, const T, T> TResult;
};

