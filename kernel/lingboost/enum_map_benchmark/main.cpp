#include <kernel/lingboost/enum.h>
#include <kernel/lingboost/enum_map.h>

#include <library/cpp/testing/benchmark/bench.h>

#include <util/random/random.h>
#include <util/generic/vector.h>

using namespace NLingBoost;

template <typename M, typename I>
inline void DoRandomAccess(TVector<M>&& v, I&& i) {
    const ui64 n = i.Iterations();
    ui64 y = 0;
    auto size = v[0].Size();
    auto keys = v[0].GetKeys();

    for (ui64 i = 0; i < n; ++i) {
        auto x = keys[RandomNumber<ui64>(size)];

        for (ui64 j : xrange(v.size())) {
            auto& m = v[(13 * j) % v.size()];
            Y_DO_NOT_OPTIMIZE_AWAY(y += m[x]);
            Y_DO_NOT_OPTIMIZE_AWAY(m[x] *= y);
        }
    }
}

struct TEnum10Struct {
    enum EType {
        V0, V1, V2, V3, V4, V5, V6, V7, V8, V9, VMax
    };
    static const size_t Size = VMax;
    static TArrayRef<const EType> GetValues() {
        return ::NLingBoost::NDetail::GetStaticRegion<EType, V0, V1, V2, V3, V4, V5, V6, V7, V8, V9, VMax>();
    }
};
using TEnum10 = TEnumOps<TEnum10Struct>;
using TEnum10Map = TStaticEnumMap<TEnum10, ui64>;
using TPoolableEnum10Map = TPoolableEnumMap<TEnum10, ui64>;
using TPoolableCompactEnum10Remap = TPoolableCompactEnumRemap<TEnum10>;
using TPoolableCompactEnum10Map = TPoolableCompactEnumMap<TEnum10, ui64>;

struct TSparseEnum10Struct {
    enum EType {
        V0 = 0, V1 = 100, V2 = 200, V3 = 300,
        V4 = 400, V5 = 500, V6 = 600, V7 = 700,
        V8 = 800, V9 = 900, VMax
    };
    static const size_t Size = VMax;
    static TArrayRef<const EType> GetValues() {
        return ::NLingBoost::NDetail::GetStaticRegion<EType, V0, V1, V2, V3, V4, V5, V6, V7, V8, V9, VMax>();
    }
};
using TSparseEnum10 = TEnumOps<TSparseEnum10Struct>;
using TSparseEnum10Map = TStaticEnumMap<TSparseEnum10, ui64>;
using TPoolableSparseEnum10Map = TPoolableEnumMap<TSparseEnum10, ui64>;
using TPoolableCompactSparseEnum10Remap = TPoolableCompactEnumRemap<TSparseEnum10>;
using TPoolableCompactSparseEnum10Map = TPoolableCompactEnumMap<TSparseEnum10, ui64>;


struct TEnumMapAccess_10_Data {
    TVector<TEnum10Map> V;

    TEnumMapAccess_10_Data() {
        V.reserve(100000);
        for (ui64 i = 0; i < V.capacity(); ++i) {
            V.emplace_back(42);
        }
    }
} EnumMapAccess_10_Data;

Y_CPU_BENCHMARK(EnumMapAccess_10, iface) {
    DoRandomAccess<TEnum10Map>(std::move(EnumMapAccess_10_Data.V), iface);
}

struct TEnumMapAccess_Sparse10_Data {
    TVector<TSparseEnum10Map> V;

    TEnumMapAccess_Sparse10_Data() {
        V.reserve(100000);
        for (ui64 i = 0; i < V.capacity(); ++i) {
            V.emplace_back(42);
        }
    }
} EnumMapAccess_Sparse10_Data;

Y_CPU_BENCHMARK(EnumMapAccess_Sparse10, iface) {
    DoRandomAccess<TSparseEnum10Map>(std::move(EnumMapAccess_Sparse10_Data.V), iface);
}

struct TPoolableEnumMapAccess_10_Data {
    TMemoryPool Pool{1UL << 20};
    TVector<TPoolableEnum10Map> V;

    TPoolableEnumMapAccess_10_Data() {
        V.reserve(100000);
        for (ui64 i = 0; i < V.capacity(); ++i) {
            V.emplace_back(Pool, 42);
        }
    }
} PoolableEnumMapAccess_10_Data;

Y_CPU_BENCHMARK(PoolableEnumMapAccess_10, iface) {
    DoRandomAccess<TPoolableEnum10Map>(std::move(PoolableEnumMapAccess_10_Data.V), iface);
}

struct TPoolableEnumMapAccess_Sparse10_Data {
    TMemoryPool Pool{1UL << 20};
    TVector<TPoolableSparseEnum10Map> V;

    TPoolableEnumMapAccess_Sparse10_Data() {
        V.reserve(100000);
        for (ui64 i = 0; i < V.capacity(); ++i) {
            V.emplace_back(Pool, 42);
        }
    }
} PoolableEnumMapAccess_Sparse10_Data;

Y_CPU_BENCHMARK(PoolableEnumMapAccess_Sparse10, iface) {
    DoRandomAccess<TPoolableSparseEnum10Map>(std::move(PoolableEnumMapAccess_Sparse10_Data.V), iface);
}

struct TPoolableCompactEnumMapAccess_10_Data {
    TMemoryPool Pool{1UL << 20};
    TPoolableCompactEnum10Remap R;
    TVector<TPoolableCompactEnum10Map> V;

    TPoolableCompactEnumMapAccess_10_Data()
        : R(Pool)
    {
        V.reserve(100000);
        for (ui64 i = 0; i < V.capacity(); ++i) {
            V.emplace_back(Pool, R.View(), 42);
        }
    }
} PoolableCompactEnumMapAccess_10_Data;

Y_CPU_BENCHMARK(PoolableCompactEnumMapAccess_10, iface) {
    DoRandomAccess<TPoolableCompactEnum10Map>(std::move(PoolableCompactEnumMapAccess_10_Data.V), iface);
}

struct TPoolableCompactEnumMapAccess_Sparse10_Data {
    TMemoryPool Pool{1UL << 20};
    TPoolableCompactSparseEnum10Remap R;
    TVector<TPoolableCompactSparseEnum10Map> V;

    TPoolableCompactEnumMapAccess_Sparse10_Data()
        : R(Pool)
    {
        V.reserve(100000);
        for (ui64 i = 0; i < V.capacity(); ++i) {
            V.emplace_back(Pool, R.View(), 42);
        }
    }
} PoolableCompactEnumMapAccess_Sparse10_Data;

Y_CPU_BENCHMARK(PoolableCompactEnumMapAccess_Sparse10, iface) {
    DoRandomAccess<TPoolableCompactSparseEnum10Map>(std::move(PoolableCompactEnumMapAccess_Sparse10_Data.V), iface);
}

