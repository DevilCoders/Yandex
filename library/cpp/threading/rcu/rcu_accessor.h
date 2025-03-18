#pragma once

#include <util/generic/ptr.h>
#include <util/system/rwlock.h>

namespace NThreading {
    namespace NRcuPrivate {
        template <class T>
        class TRcuBase {
            using TPtr = TAtomicSharedPtr<const T>;

        public:
            TRcuBase(TPtr value)
                : Value(std::move(value))
            {
            }

            TPtr GetValue() const {
                TReadGuard rg(Mutex);
                return Value;
            }

            void SetValue(TPtr ptr) {
                TWriteGuard wg(Mutex);
                Value.Swap(ptr);
            }

            class TValueReference {
            public:
                TValueReference(TPtr pointer)
                    : Pointer(std::move(pointer))
                {
                }

                const T& operator*() const {
                    return *Pointer;
                }

                const T* operator->() const {
                    return Pointer.Get();
                }

            protected:
                TPtr Pointer;
            };

            class TPointerReference: public TValueReference {
            public:
                TPointerReference(TPtr ptr)
                    : TValueReference(std::move(ptr))
                {
                }

                explicit operator bool() const {
                    return static_cast<bool>(this->Pointer);
                }
            };

        private:
            TRWMutex Mutex;
            TPtr Value;
        };

    }

    /**
 * Implement Read-Copy-Update idiom when there are many threads reading the value and at most
 * one at time that updates it.
 */
    template <class T>
    class TRcuAccessor: private NRcuPrivate::TRcuBase<T> {
        using TBase = NRcuPrivate::TRcuBase<T>;

    public:
        using TReference = typename TBase::TValueReference;

        TRcuAccessor()
            : TBase(new T)
        {
        }

        explicit TRcuAccessor(T init)
            : TBase(new T(std::move(init)))
        {
        }

        TReference Get() const {
            return TReference(this->GetValue());
        }

        void Set(T value) {
            this->SetValue(new T(std::move(value)));
        }
    };

    /*
 * Возможны случаи, когда нам может понадобиться TRcuAcccessor<T*>. Это может пригодиться
 * для полиморфных типов, а также для случаев, когда допускается отсутствие значения.
 *
 * Если мы воспользуемся базовой реализацией шаблона TRcuAccessor для указателей,
 * у нас получится, что мы будем хранить shared pointer на T* (TAtomicSharedPtr<T*>).
 * Соответственно для обращения к объекту через TValueReference, нам придётся писать так
 *
 * struct Widget {
 *     void foo();
 * };
 *
 * TRcuAccessor<Widget*> rcu;
 * rcu.Get()->->foo();
 *
 * Кроме того, указатель может быть равен nullptr, а TValueReference не позволяет это проверить.
 *
 * Поэтому для указателей нужна отдельная специализация. Она будет хранить объект в виде
 * TAtomicSharedPtr<T> и возвращать TPointerReference из метода Get(). В отличие от
 * TValueReference класс TPointerReference имеет явный operator bool, который позволяет проверить
 * указатель на равенство nullptr.
 */
    template <class T>
    class TRcuAccessor<T*>: private NRcuPrivate::TRcuBase<T> {
        using TBase = NRcuPrivate::TRcuBase<T>;

    public:
        using TReference = typename TBase::TPointerReference;

        explicit TRcuAccessor(THolder<const T> init = nullptr)
            : TBase(init.Release())
        {
        }

        TReference Get() const {
            return TReference(this->GetValue());
        }

        void Set(THolder<const T> value) {
            this->SetValue(value.Release());
        }
    };

}
