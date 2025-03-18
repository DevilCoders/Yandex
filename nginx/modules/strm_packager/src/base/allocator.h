#pragma once

extern "C" {
#include <ngx_core.h>
}

#include <util/generic/yexception.h>

namespace NStrm::NPackager {
    // allocator that use nginx pool
    template <typename T>
    class TNgxAllocator {
    public:
        using size_type = std::size_t;
        using value_type = T;

        template <typename U>
        struct rebind {
            using other = TNgxAllocator<U>;
        };

        explicit TNgxAllocator(ngx_pool_t* pool)
            : Pool(pool)
        {
            Y_ENSURE(Pool);
        }

        template <typename U>
        operator TNgxAllocator<U>() {
            return TNgxAllocator<U>(*this);
        }

        template <typename U>
        TNgxAllocator(TNgxAllocator<U>& another)
            : Pool(another.Pool)
        {
        }

        ~TNgxAllocator() = default;

        value_type* allocate(size_type n) {
            value_type* const result = (value_type*)ngx_palloc(Pool, sizeof(value_type) * n);
            Y_ENSURE(result);
            return result;
        }

        void deallocate(value_type* pointer, size_type n) {
            (void)n;
            ngx_pfree(Pool, pointer);
        }

        template <typename U>
        bool operator==(const TNgxAllocator<U>& another) const {
            return Pool == another.Pool;
        }

        template <typename U>
        bool operator!=(const TNgxAllocator<U>& another) const {
            return Pool != another.Pool;
        }

    private:
        ngx_pool_t* Pool;

        template <typename U>
        friend class TNgxAllocator;
    };

    template <typename T>
    using TNgxVector = TVector<T, TNgxAllocator<T>>;
}
