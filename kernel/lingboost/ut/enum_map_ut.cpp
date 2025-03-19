#include <kernel/lingboost/enum_map.h>
#include <kernel/lingboost/compact_map.h>
#include <kernel/lingboost/enum.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/array_ref.h>

using namespace NLingBoost;

struct TEmptyStruct {
    enum EType {
        EmptyMax
    };
    static const size_t Size = EmptyMax;
    static TArrayRef<const EType> GetValues() {
        return ::NLingBoost::NDetail::GetStaticRegion<EType>();
    }
};
using TEmpty = TEnumOps<TEmptyStruct>;
using EEmptyType = TEmpty::EType;

struct TSingleStruct {
    enum EType {
        Value = 0,
        SingleMax
    };
    static const size_t Size = SingleMax;
    static TArrayRef<const EType> GetValues() {
        return ::NLingBoost::NDetail::GetStaticRegion<EType, Value>();
    }
};
using TSingle = TEnumOps<TSingleStruct>;
using ESingleType = TSingle::EType;

struct TSeqStruct {
    enum EType {
        Value0 = 0,
        Value1 = 1,
        Value2 = 2,
        Value3 = 3,
        SeqMax
    };
    static const size_t Size = SeqMax;
    static TArrayRef<const EType> GetValues() {
        return ::NLingBoost::NDetail::GetStaticRegion<EType, Value0, Value1, Value2, Value3>();
    }
};
using TSeq = TEnumOps<TSeqStruct>;
using ESeqType = TSeq::EType;

struct TSeqWithGapStruct {
    enum EType {
        Value0 = 0,
        Value1 = 1,
        Value3 = 3,
        SeqWithGapMax
    };
    static const size_t Size = SeqWithGapMax;
    static TArrayRef<const EType> GetValues() {
        return ::NLingBoost::NDetail::GetStaticRegion<EType, Value0, Value1, Value3>();
    }
};
using TSeqWithGap = TEnumOps<TSeqWithGapStruct>;
using ESeqWithGapType = TSeqWithGap::EType;

struct TComplexStruct {
    enum EType {
        Value3 = 3,
        Value10 = 10,
        Value11 = 11,
        Value12 = 12,
        Value13 = 13,
        Value15 = 15,
        Value16 = 16,
        Value100 = 100,
        Value101 = 101,
        Value102 = 102,
        ComplexMax
    };
    static const size_t Size = ComplexMax;
    static TArrayRef<const EType> GetValues() {
        return ::NLingBoost::NDetail::GetStaticRegion<EType, Value3, Value10, Value11, Value12,
            Value13, Value15, Value16, Value100, Value101, Value102>();
    }
};
using TComplex = TEnumOps<TComplexStruct>;
using EComplexType = TComplex::EType;

struct TComplexPartStruct : public TComplex {
    static TArrayRef<const EType> GetValues() {
        return ::NLingBoost::NDetail::GetStaticRegion<EType, Value12, Value15, Value16>();
    }
};
using TComplexPart = TEnumOps<TComplexPartStruct>;
using EComplexPartType = TComplexPart::EType;

Y_UNIT_TEST_SUITE(EnumMapTest) {
    TMemoryPool Pool{1UL << 10};

    template <typename EnumStructType>
    struct TStaticEnumMapBuilder {
        template <typename ValueType, typename... Args>
        TStaticEnumMap<EnumStructType, ValueType> Create(Args&&... args) {
            return TStaticEnumMap<EnumStructType, ValueType>(std::forward<Args>(args)...);
        }
    };

    template <typename EnumStructType>
    struct TDynamicEnumMapBuilder {
        template <typename ValueType, typename... Args>
        TEnumMap<EnumStructType, ValueType> Create(Args&&... args) {
            return TEnumMap<EnumStructType, ValueType>(std::forward<Args>(args)...);
        }
    };

    template <typename EnumStructType>
    struct TPoolableEnumMapBuilder {
        template <typename ValueType, typename... Args>
        TPoolableEnumMap<EnumStructType, ValueType> Create(Args&&... args) {
            return TPoolableEnumMap<EnumStructType, ValueType>(std::forward<Args>(args)...);
        }
    };

    template <typename EnumStructType>
    struct TCompactEnumMapBuilder {
        template <typename ValueType, typename... Args>
        TPoolableCompactEnumMap<EnumStructType, ValueType> Create(Args&&... args) {
            return TPoolableCompactEnumMap<EnumStructType, ValueType>(std::forward<Args>(args)...);
        }
    };

    template <typename BuilderType, typename... Args>
    void CheckEnumMap(Args&&... args) {
        BuilderType builder;
        using TMapType = decltype(builder.template Create<int>(std::forward<Args>(args)...));
        using TEnum = typename TMapType::TEnum;
        using TEnumStruct = typename TMapType::TEnumStruct;

        Cdbg << "Check: init with 0's" << Endl;
        TMapType m = builder.template Create<int>(std::forward<Args>(args)...);
        for (auto entry : m) {
            Cdbg << (int)entry.Key() << " --> " << entry.Value() << Endl;
            UNIT_ASSERT_EQUAL(entry.Value(), 0);
        }

        Cdbg << "Check: assign (operator =) with 1's" << Endl;
        m = builder.template Create<int>(std::forward<Args>(args)..., 1);
        for (auto entry : m) {
            Cdbg << (int)entry.Key() << " --> " << entry.Value() << Endl;
            UNIT_ASSERT_EQUAL(entry.Value(), 1);
        }

        for (auto entry : m) {
            entry.Value() = 2;
        }

        size_t count = 0;
        for (auto entry : m) {
            UNIT_ASSERT_EQUAL(entry.Value(), 2);
            count += 1;
        }
        UNIT_ASSERT_EQUAL(count, m.Size());

        for (auto entry : m) {
            m[entry.Key()] = 3;
        }
        for (auto entry : m) {
            UNIT_ASSERT_EQUAL(entry.Value(), 3);
        }

        if (m.Size() > 0) {
            TEnum key = m.GetKeys()[m.Size() / 2];
            m[key] = 517;
            for (auto entry : m) {
                if (entry.Key() != key) {
                    entry.Value() = 715;
                }
            }
            UNIT_ASSERT_EQUAL(m[key], 517);
        }

        Cdbg << "Check: map views" << Endl;

        auto view = m.View();
        auto constView = const_cast<const TMapType&>(m).View();

        UNIT_ASSERT_EQUAL(m.Size(), view.Size());
        for (auto entry : view) {
            UNIT_ASSERT_EQUAL(entry.Value(), m[entry.Key()]);
        }
        if (m.Size() > 0) {
            view[*m.KeysBegin()] = 10;
            UNIT_ASSERT_EQUAL(m[*m.KeysBegin()], 10);
        }

        for (auto entry : constView) {
            UNIT_ASSERT_EQUAL(entry.Value(), m[entry.Key()]);
        }

        Cdbg << "Check: out of range keys" << Endl;

        if (m.Size() > 0) {
            for (auto key : m.GetKeys()) {
                UNIT_ASSERT(m.IsKeyInRange(key));
            }
            UNIT_ASSERT(m.IsKeyInRange(*m.KeysBegin()));
            UNIT_ASSERT(m.IsIndexInRange(0));
            UNIT_ASSERT(m.IsIndexInRange(m.Capacity() - 1));
        }
        UNIT_ASSERT(!m.IsIndexInRange(TEnumStruct::Size + 100));
        #ifndef _ubsan_enabled_
        UNIT_ASSERT(!m.IsKeyInRange(TEnum(TEnumStruct::Size + 100)));
        #endif
        UNIT_ASSERT(!m.IsIndexInRange(m.Capacity()));
    }

    template <typename EnumStructType>
    void CheckFullStaticEnumMap() {
        CheckEnumMap<TStaticEnumMapBuilder<EnumStructType>>();

        using TMapType = TStaticEnumMap<EnumStructType, int>;

        char bufX[sizeof(TMapType)];
        memset(bufX, 0xFF, sizeof(bufX));
        TMapType* mX = new (bufX) TMapType(1999);

        char bufY[sizeof(TMapType)];
        memset(bufY, 0x22, sizeof(bufY));
        TMapType *mY = new (bufY) TMapType(1999);

        for (auto entry : *mX) {
            UNIT_ASSERT_EQUAL(entry.Value(), 1999);
            UNIT_ASSERT_EQUAL((*mY)[entry.Key()], 1999);
        }

        mX->~TMapType();
        mY->~TMapType();
    }

    template <typename EnumStructType, typename... Args>
    void CheckDynamicEnumMap(Args&&... args) {
        CheckEnumMap<TDynamicEnumMapBuilder<EnumStructType>>(std::forward<Args>(args)...);
    }

    template <typename EnumStructType, typename... Args>
    void CheckPoolableEnumMap(TMemoryPool& pool, Args&&... args) {
        CheckEnumMap<TPoolableEnumMapBuilder<EnumStructType>>(pool, std::forward<Args>(args)...);
    }

    template <typename EnumStructType, typename... Args>
    void CheckCompactEnumMap(TMemoryPool& pool, Args&&... args) {
        TPoolableCompactEnumRemap<EnumStructType> remap(pool);
        CheckEnumMap<TCompactEnumMapBuilder<EnumStructType>>(pool, remap.View(), std::forward<Args>(args)...);
    }

    template <typename EnumStructType, typename... Args>
    void CheckCompactEnumMap(TMemoryPool& pool, TArrayRef<const typename EnumStructType::EType> domain, Args&&... args) {
        TPoolableCompactEnumRemap<EnumStructType> remap(pool, domain);
        CheckEnumMap<TCompactEnumMapBuilder<EnumStructType>>(pool, remap.View(), std::forward<Args>(args)...);
    }

    Y_UNIT_TEST(TestEmpty) {
        CheckFullStaticEnumMap<TEmpty>();
        CheckDynamicEnumMap<TEmpty>();
        CheckPoolableEnumMap<TEmpty>(Pool);
        CheckCompactEnumMap<TEmpty>(Pool);
    }

    Y_UNIT_TEST(TestDefaultCtor) {
        TStaticEnumMap<TSeq, TStaticEnumMap<TSeq, ui8>> map{};
        char* mapPtr = (char*) &map;

        Cdbg << "ADDR(map) = " << (void*) mapPtr << Endl;

        for (auto x : map) {
            for (auto y : x.Value()) {
                char* elemPtr = (char*) &y.Value();

                UNIT_ASSERT(elemPtr >= mapPtr);
                UNIT_ASSERT(elemPtr < mapPtr + (1UL << 10));

                Cdbg << "ADDR(" << (int) x.Key() << "," << (int) y.Key() << ") = " << (void*) elemPtr
                    << "(OFFSET = " << elemPtr - mapPtr << ")" << Endl;
            }
        }
    }

    Y_UNIT_TEST(TestSingle) {
        CheckFullStaticEnumMap<TSingle>();
        CheckDynamicEnumMap<TSingle>();
        CheckPoolableEnumMap<TSingle>(Pool);
        CheckCompactEnumMap<TSingle>(Pool);
    }

    Y_UNIT_TEST(TestSeq) {
        CheckFullStaticEnumMap<TSeq>();
        CheckDynamicEnumMap<TSeq>();
        CheckPoolableEnumMap<TSeq>(Pool);
        CheckCompactEnumMap<TSeq>(Pool);
    }

    Y_UNIT_TEST(TestSeqWithGap) {
        CheckFullStaticEnumMap<TSeqWithGap>();
        CheckDynamicEnumMap<TSeqWithGap>();
        CheckPoolableEnumMap<TSeqWithGap>(Pool);
        CheckCompactEnumMap<TSeqWithGap>(Pool);
    }

    Y_UNIT_TEST(TestComplex) {
        CheckFullStaticEnumMap<TComplex>();
        CheckDynamicEnumMap<TComplex>();
        CheckPoolableEnumMap<TComplex>(Pool);
        CheckCompactEnumMap<TComplex>(Pool);
    }

    Y_UNIT_TEST(TestComplexPart) {
        CheckPoolableEnumMap<TComplexPart>(Pool);
        CheckDynamicEnumMap<TComplex>(TComplexPart::GetValuesRegion());
        CheckPoolableEnumMap<TComplex>(Pool, TComplexPart::GetValuesRegion());
        CheckCompactEnumMap<TComplex>(Pool, TComplexPart::GetValuesRegion());
    }

    Y_UNIT_TEST(TestPoolableUninitializedMap) {
        NLingBoost::TPoolableEnumMap<TComplex, bool> map;
        const auto& constMap = map;
        UNIT_ASSERT_EQUAL(map.Size(), 0);
        UNIT_ASSERT_EQUAL(map.GetKeys().size(), 0);
        UNIT_ASSERT_EQUAL(map.begin(), map.end());
        UNIT_ASSERT_EQUAL(constMap.begin(), constMap.end());

        map.Fill(false);

        map = TPoolableEnumMap<TComplex, bool>(Pool, true);
        UNIT_ASSERT_UNEQUAL(map.Size(), 0);
        UNIT_ASSERT_UNEQUAL(map.GetKeys().size(), 0);
        UNIT_ASSERT_UNEQUAL(map.begin(), map.end());
        UNIT_ASSERT_UNEQUAL(constMap.begin(), constMap.end());

        for (auto entry : map) {
            UNIT_ASSERT_EQUAL(entry.Value(), true);
        }
    }

    struct TDestructCounter {
        size_t* Count = nullptr;

        TDestructCounter() = default;
        TDestructCounter(const TDestructCounter&) = default;
        explicit TDestructCounter(size_t& count)
            : Count(&count)
        {}
        TDestructCounter& operator = (const TDestructCounter& other) {
            if (Count) {
                ++*Count;
            }
            Count = other.Count;
            return *this;
        }
        ~TDestructCounter() {
            if (Count) {
                ++*Count;
            }
        }
    };

    Y_UNIT_TEST(TestDtors) {
        const size_t numKeys = TComplex::GetValuesRegion().size();
        size_t count = 0;
        {
            NLingBoost::TStaticEnumMap<TComplex, TDestructCounter> map;
            map.Fill(TDestructCounter(count));
        }
        Cdbg << "STATIC DTOR CALLED " << count << " TIMES" << Endl;
        UNIT_ASSERT_EQUAL(count, numKeys + 1); // once for temporary

        count = 0;
        {
            NLingBoost::TDynamicEnumMap<TComplex, TDestructCounter> map;
            map.Fill(TDestructCounter(count));
        }
        Cdbg << "DYNAMIC DTOR CALLED " << count << " TIMES" << Endl;
        UNIT_ASSERT_EQUAL(count, numKeys + 1);

        count = 0;
        {
            NLingBoost::TDynamicEnumMap<TComplex, TDestructCounter> mapX;
            mapX.Fill(TDestructCounter(count));

            NLingBoost::TDynamicEnumMap<TComplex, TDestructCounter> mapY;
            mapY.Fill(TDestructCounter(count));

            mapX = mapY;
        }
        Cdbg << "DYNAMIC DTOR CALLED " << count << " TIMES (ASSIGN)" << Endl;
        UNIT_ASSERT_EQUAL(count, 3 * numKeys + 2);

        count = 0;
        {
            NLingBoost::TDynamicEnumMap<TComplex, TDestructCounter> mapX;
            mapX.Fill(TDestructCounter(count));

            NLingBoost::TDynamicEnumMap<TComplex, NLingBoost::TDynamicEnumMap<TComplex, TDestructCounter>> mapY;
            mapY.Fill(mapX);
        }
        Cdbg << "DYNAMIC DTOR CALLED " << count << " TIMES (RECURSIVE)" << Endl;
        UNIT_ASSERT_EQUAL(count, (numKeys * numKeys) + numKeys + 1);

        count = 0;
        {
            NLingBoost::TPoolableEnumMap<TComplex, TDestructCounter> mapX(Pool);
            mapX.Fill(TDestructCounter(count));

            NLingBoost::TPoolableEnumMap<TComplex, TDestructCounter> mapY(Pool);
            mapY.Fill(TDestructCounter(count));

            mapX = mapY;
        }
        Cdbg << "POOLABLE DTOR CALLED " << count << " TIMES" << Endl;
        UNIT_ASSERT_EQUAL(count, 2); // shallow, 2 for temporary
    }
}
