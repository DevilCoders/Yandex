#include <library/cpp/dbg_output/dump.h>
#include <library/cpp/nth_elements/nth_elements.h>

#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/stream/mem.h>

template <class T>
static inline T Read(IInputStream& in) {
    T t;

    in.LoadOrFail(&t, sizeof(t));

    return t;
}

template <class TValues, class TIndexes, class TComparator>
static inline void Test(const TValues& values, const TIndexes& indexes, TComparator comparator) {
    TValues valuesCopy = values;
    TVector<typename TValues::iterator> positions;
    Transform(indexes.begin(), indexes.end(), std::back_inserter(positions),
              [&valuesCopy](auto index) { return valuesCopy.begin() + index; });
    NthElements(valuesCopy.begin(), valuesCopy.end(),
                positions.begin(), positions.end(),
                comparator);

    auto valuesCopyForSort = values;
    Sort(valuesCopyForSort, comparator);

    for (auto index : indexes) {
        Y_ENSURE(valuesCopy[index] == valuesCopyForSort[index]);
    }
}

extern "C" int LLVMFuzzerTestOneInput(const ui8* data, size_t size) {
    TMemoryInput mi(data, size);

    if (mi.Avail() < 3 * sizeof(size_t)) {
        return 0;
    }

    size_t n = Min<size_t>(1000, Read<size_t>(mi));
    size_t k = Min<size_t>(1000, Read<size_t>(mi));
    size_t randSeed = Read<size_t>(mi) % 1000;

    if (n == 0 || n < k) {
        return 0;
    }

    if (mi.Avail() < (n + k) * sizeof(size_t)) {
        return 0;
    }

    TVector<size_t> values;
    GenerateN(std::back_inserter(values), n,
              [&mi] { return Read<size_t>(mi) % 1000; });

    TVector<size_t> indexes;
    GenerateN(std::back_inserter(indexes), k,
              [&mi, &values] { return Read<size_t>(mi) % values.size() % 1000; });

    try {
        Test(values, indexes, TLess<>());
        Test(values, indexes, TGreater<>());
    } catch (...) {
        Cout << CurrentExceptionMessage() << Endl;
        Cout << "RandSeed: " << randSeed << Endl;
        Cout << "Values: " << DbgDump(values) << Endl;
        Cout << "Indexes: " << DbgDump(indexes) << Endl;

        throw;
    }
    return 0;
}
