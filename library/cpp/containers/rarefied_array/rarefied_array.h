#pragma once

#include <util/system/yassert.h>
#include <util/system/defaults.h>
#include <util/stream/output.h>
#include <util/generic/ymath.h>
#include <util/generic/vector.h>
#include <utility>

/** TRarefiedArray: контейнер со следующими свойствами:
    - снаружи выглядит почти как hash_map<int, T> (есть operator[], begin(), end(), find(), возвращающий что-то, что можно трактовать, как указатель на пару,
      size()). Отличие от hash_map в том, что ключ всегда int, и его максимальное значение ограничено сверху
    - изнутри это два массива - sparse и dense
    - потребление памяти 4*(максимальное значение ключа) + sizeof(T)*(число элементов) + O(1)
    - вставка значения, итерация, find() и clear() за O(1)
    на тесте работает в несколько раз быстрее hash_map'а (вдвое при полностью рандомной вставке, в десятки раз вставке в возрастающем порядке)
    вообще говоря, жрет больше памяти (хотя это, очевидно, depends)
*/
template <class T>
class TRarefiedArray {
    size_t Size;
    ui32* Sparse;
    TVector<std::pair<size_t, T>> Dense;
    void InitImpl(size_t size) {
        Size = size;
        Sparse = new ui32[size];
#if defined(WITH_VALGRIND) || defined(_msan_enabled_)
        // На самом деле, этот контейнер не требует инициализации массива Sparse (см. также метод clear)
        // путь выполнения operator[] и find() действительно зависит от неинициализированных значений,
        // но при этом результат остается доказуемо корректным.
        // Инициализация здесь вставлена только для того, чтобы valgrind "не ругался", обнаружив сей факт.
        memset(Sparse, 0, size * sizeof(ui32));
#endif
        Dense.reserve((size_t)(sqrt((double)size)) + 1);
    }

public:
    TRarefiedArray()
        : Size(0)
        , Sparse(nullptr)
    {
    }
    void Init(size_t size) {
        Y_ASSERT(Size == 0 && !Sparse && "a rarefied array cannot be resized");
        InitImpl(size);
    }
    TRarefiedArray(size_t size) {
        InitImpl(size);
    }
    ~TRarefiedArray() {
        delete[] Sparse;
    }
    size_t capacity() const {
        return Size;
    }
    T& operator[](size_t index) {
        if (index >= Size)
            Cerr << "TRarefiedArray index " << index << " out of bounds (" << Size << ")" << Endl;
        Y_ASSERT(index < Size);
        ui32 denseIdx = Sparse[index];
        if (denseIdx < Dense.size() && Dense[denseIdx].first == index)
            return Dense[denseIdx].second;
        Sparse[index] = Dense.ysize();
        Dense.emplace_back();
        Dense.back().first = index;
        return Dense.back().second;
    }
    void clear() {
        Dense.clear();
    }
    using iterator = typename TVector<std::pair<size_t, T>>::iterator;
    using const_iterator = typename TVector<std::pair<size_t, T>>::const_iterator;
    iterator begin() {
        return Dense.begin();
    }
    const_iterator begin() const {
        return Dense.begin();
    }
    iterator end() {
        return Dense.end();
    }
    const_iterator end() const {
        return Dense.end();
    }
    size_t size() const {
        return Dense.size();
    }
    iterator find(size_t index) {
        if (index >= Size)
            Cerr << "TRarefiedArray index " << index << " out of bounds (" << Size << ")" << Endl;
        Y_ASSERT(index < Size);
        ui32 denseIdx = Sparse[index];
        if (denseIdx < Dense.size() && Dense[denseIdx].first == index)
            return &Dense[denseIdx];
        return end();
    }
    const_iterator find(size_t index) const {
        return (const_cast<TRarefiedArray*>(this))->find(index);
    }
    bool empty() const {
        return Dense.empty();
    }
    void swap(TRarefiedArray& rhs) {
        std::swap(Size, rhs.Size);
        std::swap(Sparse, rhs.Sparse);
        Dense.swap(rhs.Dense);
    }
};
