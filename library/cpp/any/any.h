#pragma once

#include <util/generic/yexception.h>

namespace NAny {
    namespace NImpl {
        struct TBadAnyCast : yexception {
        };

        struct TEmptyAny {
        };

        struct TBasePolicy {
            virtual void Delete(void** x) = 0;
            virtual void Copy(void const* src, void** dest) = 0;
            virtual void* GetValue(void** src) = 0;

            virtual const void* GetValue(const void* const* src) const = 0;
        };

        template <typename T>
        struct TSmallPolicy : TBasePolicy {
            void Delete(void**) override {
            }

            void Copy(void const* src, void** dest) override {
                *(T*)dest = *(const T*)src;
            }
            void* GetValue(void** src) override {
                return reinterpret_cast<void*>(src);
            }
            const void* GetValue(const void* const* src) const override {
                return reinterpret_cast<const void*>(src);
            }
        };

        template <typename T>
        struct TBigPolicy : TBasePolicy {
            void Delete(void** x) override {
                if (*x)
                    delete (*reinterpret_cast<T**>(x));
                *x = nullptr;
            }
            void Copy(void const* src, void** dest) override {
                *dest = new T(*reinterpret_cast<T const*>(src));
            }
            void* GetValue(void** src) override {
                return *src;
            }
            const void* GetValue(const void* const* src) const override {
                return *src;
            }
        };

        template <typename T>
        struct TChoosePolicy {
            using TType = TBigPolicy<T>;
        };

        template <typename T>
        struct TChoosePolicy<T*> {
            using TType = TSmallPolicy<T*>;
        };

        struct TAny;

        /// Choosing the policy for an TAny type is illegal, but should never happen.
        /// This is designed to throw a compiler error.
        template <>
        struct TChoosePolicy<TAny> {
            using TType = void;
        };

/// Specializations for small types.
#define ANY_SMALL_POLICY(TYPE) \
    template <>                \
    struct                     \
        TChoosePolicy<TYPE> { using TType = TSmallPolicy<TYPE>; };

        ANY_SMALL_POLICY(bool);
        ANY_SMALL_POLICY(char);
        ANY_SMALL_POLICY(signed char);
        ANY_SMALL_POLICY(unsigned char);
        ANY_SMALL_POLICY(signed short);
        ANY_SMALL_POLICY(unsigned short);
        ANY_SMALL_POLICY(signed int);
        ANY_SMALL_POLICY(unsigned int);
        ANY_SMALL_POLICY(signed long);
        ANY_SMALL_POLICY(unsigned long);
        ANY_SMALL_POLICY(signed long long);
        ANY_SMALL_POLICY(unsigned long long);
        ANY_SMALL_POLICY(float);
        ANY_SMALL_POLICY(double);

#undef ANY_SMALL_POLICY

        /// This function will return a different policy for each type.
        template <typename T>
        TBasePolicy* GetPolicy() {
            static typename TChoosePolicy<T>::TType policy;
            return &policy;
        }
    }

    struct TAny {
    private:
        NImpl::TBasePolicy* Policy;
        void* Object;

    public:
        TAny()
            : Policy(NImpl::GetPolicy<NImpl::TEmptyAny>())
            , Object(nullptr)
        {
        }

        TAny(const TAny& x)
            : Policy(NImpl::GetPolicy<NImpl::TEmptyAny>())
            , Object(nullptr)
        {
            Assign(x);
        }

        template <typename T>
        TAny(const T& x)
            : Policy(NImpl::GetPolicy<NImpl::TEmptyAny>())
            , Object(nullptr)
        {
            Assign(x);
        }

        ~TAny() {
            Policy->Delete(&Object);
        }

        TAny& operator=(const TAny& x) {
            return Assign(x);
        }

        template <typename T>
        TAny& operator=(const T& x) {
            return Assign(x);
        }

        template <typename T>
        T& Cast() {
            if (Policy != NImpl::GetPolicy<T>())
                ythrow NImpl::TBadAnyCast();
            T* r = reinterpret_cast<T*>(Policy->GetValue(&Object));
            return *r;
        }

        template <typename T>
        const T& Cast() const {
            if (Policy != NImpl::GetPolicy<T>())
                ythrow NImpl::TBadAnyCast();
            const T* r = reinterpret_cast<const T*>(Policy->GetValue(&Object));
            return *r;
        }

        bool Compatible(const TAny& x) const {
            return Policy == x.Policy;
        }

        template <typename T>
        bool Compatible() const {
            return Policy == NImpl::GetPolicy<T>();
        }

        template <typename T>
        bool Compatible(const T&) const {
            return Policy == NImpl::GetPolicy<T>();
        }

        bool Empty() const {
            return Policy == NImpl::GetPolicy<NImpl::TEmptyAny>();
        }

        void Reset() {
            Policy->Delete(&Object);
            Object = nullptr;
            Policy = NImpl::GetPolicy<NImpl::TEmptyAny>();
        }

        TAny& Assign(const TAny& x) {
            if (this == &x)
                return *this;

            Reset();
            Policy = x.Policy;
            Policy->Copy(Policy->GetValue(&x.Object), &Object);
            return *this;
        }

        template <typename T>
        TAny& Assign(const T& x) {
            Reset();
            Policy = NImpl::GetPolicy<T>();
            Policy->Copy(&x, &Object);
            return *this;
        }

        TAny& Swap(TAny& x) {
            DoSwap(Policy, x.Policy);
            DoSwap(Object, x.Object);
            return *this;
        }
    };
}
