#pragma once

namespace NDoom {


struct TIdentityCombiner {
    static constexpr bool IsIdentity = true;

    template <class T>
    static void Combine(const T& prev, const T& next, T* res) {
        Y_UNUSED(prev);
        *res = next;
    }
};


} // namespace NDoom
