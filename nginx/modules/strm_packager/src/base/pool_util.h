#pragma once

extern "C" {
#include <ngx_core.h>
}

#include <util/generic/yexception.h>

namespace NStrm::NPackager {
    template <typename T>
    class TNgxPoolUtil {
    public:
        static void DeleteInPool(void* data) {
            MakeAligned(data)->~T();
        }

        template <typename... Args>
        static T* NewInPool(ngx_pool_t* pool, Args&&... args) {
            Y_ENSURE(pool);
            ngx_pool_cleanup_t* const pca = ngx_pool_cleanup_add(pool, sizeof(T) + alignof(T) - 1);
            Y_ENSURE(pca && pca->data);

            T* result = MakeAligned(pca->data);

            new (result) T(std::forward<Args>(args)...);
            pca->handler = DeleteInPool;

            return result;
        }

        static T* Calloc(ngx_pool_t* pool, size_t count = 1) {
            Y_ENSURE(pool);
            Y_ENSURE(count > 0);
            void* result = ngx_pcalloc(pool, sizeof(T) * count + alignof(T) - 1);
            Y_ENSURE(result);
            return MakeAligned(result);
        }

        static T* Alloc(ngx_pool_t* pool, size_t count = 1) {
            Y_ENSURE(pool);
            Y_ENSURE(count > 0);
            void* result = ngx_palloc(pool, sizeof(T) * count + alignof(T) - 1);
            Y_ENSURE(result);
            return MakeAligned(result);
        }

        explicit TNgxPoolUtil(ngx_pool_t* pool)
            : Pool(pool)
        {
            Y_ENSURE(Pool);
        }

        template <typename... Args>
        T* New(Args&&... args) {
            return NewInPool(Pool, std::forward<Args>(args)...);
        }

        T* Calloc(size_t count = 1) {
            return Calloc(Pool, count);
        }

        T* Alloc(size_t count = 1) {
            return Alloc(Pool, count);
        }

    private:
        static T* MakeAligned(void* const ptr) {
            size_t ptrv = (size_t)ptr;
            while (ptrv % alignof(T) != 0) {
                ++ptrv;
            }
            return (T*)ptrv;
        }

    private:
        ngx_pool_t* Pool;
    };

    inline ngx_str_t NginxString(const TStringBuf& str, ngx_pool_t* pool, const bool makeLowercase = false) {
        ngx_str_t result{0, nullptr};

        if (str) {
            result.len = str.length();
            result.data = TNgxPoolUtil<u_char>::Calloc(pool, str.length() + 1);
            if (makeLowercase) {
                ngx_strlow(result.data, (ui8*)str.Data(), str.length());
            } else {
                std::memcpy(result.data, str.Data(), str.length());
            }
            result.data[result.len] = 0; // sometimes nginx require null-terminated string
        }

        return result;
    }

}
