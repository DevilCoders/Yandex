#pragma once

#include <util/system/defaults.h>

// Контейнер, гарантирующий элементу типа T конкретный размер Size в файле
template<class T, size_t Size>
class TMapBlock {

private:
    char Buffer[Size]; // Обеспечивает необходимый размер

private:
    void StaticCheck() const; // Не позволяет скомпилироваться, если Size мал, чтобы уместить T

public:
    TMapBlock(const T& info);

    const T& Info() const;
};

template<class T, size_t Size>
inline void TMapBlock<T, Size>::StaticCheck() const
{
    static_assert(sizeof(T) <= sizeof(TMapBlock<T, Size>), "expect sizeof(T) <= sizeof(TMapBlock<T, Size>)");
    static_assert(sizeof(TMapBlock<T, Size>) == sizeof(char) * Size, "expect sizeof(TMapBlock<T, Size>) == sizeof(char) * Size");
    static_assert(sizeof(char) == 1, "expect sizeof(char) == 1");
}

template<class T, size_t Size>
inline TMapBlock<T, Size>::TMapBlock(const T& info)
{
    StaticCheck();
    new (this) T(info);
}

template<class T, size_t Size>
inline const T& TMapBlock<T, Size>::Info() const
{
    StaticCheck();
    return reinterpret_cast<const T&>(*this);
}
