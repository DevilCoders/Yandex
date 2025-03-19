#pragma once

#include <util/generic/array_ref.h>
#include <util/generic/vector.h>

namespace NMatrixnet {
    inline const float* GetOrCopyFeatures(const TArrayRef<const float>& factors, const size_t desiredLength, TVector<float>& holder) {
        if (Y_LIKELY(factors.size() >= desiredLength)) {
            return factors.data();
        }

        Y_ASSERT(holder.size() + desiredLength <= holder.capacity() && "Insertion causes reallocation and thus invalidates all previuos pointers");
        size_t offset = holder.size();
        holder.insert(holder.end(), desiredLength, 0);
        memcpy(&holder[offset], factors.data(), factors.size() * sizeof(float));

        return &holder[offset];
    }

    inline const float* CreateZeroes(const size_t desiredLength, TVector<float>& holder) {
        size_t offset = holder.size();
        holder.insert(holder.end(), desiredLength, 0);
        return &holder[offset];
    }
}
