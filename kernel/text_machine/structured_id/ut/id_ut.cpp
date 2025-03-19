#include "id_ut.h"

#include <kernel/text_machine/structured_id/full_id.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NStructuredId;

struct TXYIdBuilder : public TIdBuilder<EXYPartType> {
    using TXYList = TListType<
        TPair<EXYPartType::XPart, TPartMixin<EXType>>,
        TPair<EXYPartType::YPart, TPartMixin<EYType>>
    >;
};

using TYBitSet = TEnumBitSet<EYType, 0, 2>;

struct TXYSetIdBuilder : public TIdBuilder<EXYSetPartType> {
    using TXYSetList = TListType<
        TPair<EXYSetPartType::XPart, TPartMixin<EXType>>,
        TPair<EXYSetPartType::YSetPart, TEnumBitSetMixin<TYBitSet>>
    >;
};

namespace NStructuredId {
    template <>
    struct TGetIdTraitsList<EXYPartType> {
        using TResult = TXYIdBuilder::TXYList;
    };

    template <>
    struct TGetIdTraitsList<EXYSetPartType> {
        using TResult = TXYSetIdBuilder::TXYSetList;
    };
}

using TXYId = TFullId<EXYPartType>;
using TXYSetId = TFullId<EXYSetPartType>;

struct TIsSubX
    : public NStructuredId::NDetail::TIsEqual
{
    using NStructuredId::NDetail::TIsEqual::IsSubValue;

    inline bool IsSubValue(EXType x, EXType y) const {
        return (x == y) || (x == EXType::XFoo && y == EXType::XBar);
    }
};

class TIdTest : public TTestBase {
private:
    UNIT_TEST_SUITE(TIdTest);
        UNIT_TEST(TestId);
        UNIT_TEST(TestStringId);
        UNIT_TEST(TestSubId);
        UNIT_TEST(TestIdHash);
        UNIT_TEST(TestIdWithEnumBitSetPart);
        UNIT_TEST(TestIdWithEnumBitSetPartHash);
    UNIT_TEST_SUITE_END();

public:
    void TestId() {
        TXYId id0;
        TXYId id1(EXType::XFoo, EYType::YBar);
        TXYId id2(EYType::YBar, EXType::XFoo);
        TXYId id3(EXType::XBar, EYType::YFoo);
        TXYId id4(EXType::XBar);
        TXYId id5(EYType::YBar);

        UNIT_ASSERT_EQUAL(id0.FullName(), "");
        UNIT_ASSERT_EQUAL(id1.FullName(), "XFoo_YBar");
        UNIT_ASSERT_EQUAL(id2.FullName(), "XFoo_YBar");
        UNIT_ASSERT_EQUAL(id3.FullName(), "XBar_YFoo");
        UNIT_ASSERT_EQUAL(id4.FullName(), "XBar");
        UNIT_ASSERT_EQUAL(id5.FullName(), "YBar");

        UNIT_ASSERT(id3.IsValid<EXYPartType::XPart>());
        UNIT_ASSERT(id3.IsValid<EXYPartType::YPart>());
        UNIT_ASSERT_EQUAL(id3.Get<EXYPartType::XPart>(), EXType::XBar);
        UNIT_ASSERT_EQUAL(id3.Get<EXYPartType::YPart>(), EYType::YFoo);
        id3.UnSet<EXYPartType::YPart>();
        UNIT_ASSERT(id3.IsValid<EXYPartType::XPart>());
        UNIT_ASSERT(!id3.IsValid<EXYPartType::YPart>());
        UNIT_ASSERT_EQUAL(id3.Get<EXYPartType::XPart>(), EXType::XBar);
        UNIT_ASSERT_EQUAL(id3.FullName(), "XBar");
        id3.Set(EYType::YBar);
        UNIT_ASSERT(id3.IsValid<EXYPartType::XPart>());
        UNIT_ASSERT(id3.IsValid<EXYPartType::YPart>());
        UNIT_ASSERT_EQUAL(id3.Get<EXYPartType::XPart>(), EXType::XBar);
        UNIT_ASSERT_EQUAL(id3.Get<EXYPartType::YPart>(), EYType::YBar);
        UNIT_ASSERT_EQUAL(id3.FullName(), "XBar_YBar");

        UNIT_ASSERT(TXYId(EXType::XFoo, EYType::YBar) < TXYId(EXType::XBar, EYType::YFoo));
    }

    void TestSubId() {
        UNIT_ASSERT(IsSubId(TXYId(), TXYId()));
        UNIT_ASSERT(IsSubId(TXYId(EXType::XFoo, EYType::YBar), TXYId()));
        UNIT_ASSERT(IsSubId(TXYId(EXType::XFoo, EYType::YBar), TXYId(EYType::YBar)));
        UNIT_ASSERT(!IsSubId(TXYId(), TXYId(EXType::XFoo, EYType::YBar)));
        UNIT_ASSERT(!IsSubId(TXYId(EXType::XFoo, EYType::YBar), TXYId(EYType::YFoo)));
        UNIT_ASSERT(!IsSubId(TXYId(EXType::XFoo), TXYId(EYType::YBar), TIsSubX{}));
        UNIT_ASSERT(!IsSubId(TXYId(EYType::YBar), TXYId(EXType::XFoo), TIsSubX{}));
    }

    void TestStringId() {
        UNIT_ASSERT_EQUAL(ToStringId(TXYId()), TXYId::TStringId());
        UNIT_ASSERT_EQUAL(FromStringId(TXYId::TStringId()), TXYId());

        UNIT_ASSERT_EQUAL(ToStringId(TXYId(EXType::XFoo)), TXYId::TStringId({{EXYPartType::XPart, "XFoo"}}));
        UNIT_ASSERT_EQUAL(FromStringId(TXYId::TStringId({{EXYPartType::XPart, "XFoo"}})), TXYId(EXType::XFoo));

        UNIT_ASSERT_EQUAL(ToStringId(TXYId(EXType::XFoo, EYType::YBar)), TXYId::TStringId({{EXYPartType::XPart, "XFoo"}, {EXYPartType::YPart, "YBar"}}));
        UNIT_ASSERT_EQUAL(FromStringId(TXYId::TStringId({{EXYPartType::XPart, "XFoo"}, {EXYPartType::YPart, "YBar"}})), TXYId(EXType::XFoo, EYType::YBar));

        UNIT_ASSERT_EXCEPTION(FromStringId(TXYId::TStringId({{EXYPartType::XPart, "XWtf"}})), yexception);
    }

    void TestIdHash() {
        THashSet<TXYId> s;
        s.insert(TXYId());
        s.insert(TXYId(EXType::XFoo));

        UNIT_ASSERT(s.contains(TXYId()));
        UNIT_ASSERT(s.contains(TXYId(EXType::XFoo)));
        UNIT_ASSERT(!s.contains(TXYId(EXType::XBar)));
        UNIT_ASSERT(!s.contains(TXYId(EXType::XFoo, EYType::YBar)));
    }

    void TestIdWithEnumBitSetPart() {
        TXYSetId id0;
        TXYSetId id1(EXType::XFoo, EYType::YBar);

        UNIT_ASSERT_EQUAL(id0.FullName(), "");
        UNIT_ASSERT_EQUAL(id1.FullName(), "XFoo_YBar");

        {
            TYBitSet bitSet;
            bitSet.Init(EYType::YBar);
            UNIT_ASSERT_EQUAL(id1.Get<EXYSetPartType::YSetPart>(), bitSet);
        }

        id1.Set(EYType::YFoo);
        UNIT_ASSERT_EQUAL(id1.FullName(), "XFoo_YFooYBar");
        UNIT_ASSERT(id1.IsValid<EXYSetPartType::YSetPart>());

        id1.UnSet<EXYSetPartType::YSetPart>();
        UNIT_ASSERT(!id1.IsValid<EXYSetPartType::YSetPart>());
        UNIT_ASSERT_EQUAL(id1.FullName(), "XFoo");

        id1.Set(EYType::YFoo, EYType::YBar);
        UNIT_ASSERT(id1.IsValid<EXYSetPartType::YSetPart>());
        UNIT_ASSERT_EQUAL(id1.FullName(), "XFoo_YFooYBar");

        id1.Set(EYType::YBar);
        UNIT_ASSERT_EQUAL(id1.FullName(), "XFoo_YFooYBar");

        id1.Set(EYType::YFoo);
        UNIT_ASSERT_EQUAL(id1.FullName(), "XFoo_YFooYBar");

        {
            TYBitSet bitSet;
            bitSet.Init(EYType::YFoo, EYType::YBar);
            UNIT_ASSERT_EQUAL(id1.Get<EXYSetPartType::YSetPart>(), bitSet);
        }
    }

    void TestIdWithEnumBitSetPartHash() {
        THashSet<TXYSetId> s;
        s.insert(TXYSetId());
        s.insert(TXYSetId(EXType::XFoo));
        s.insert(TXYSetId(EYType::YFoo, EYType::YBar));

        UNIT_ASSERT(s.contains(TXYSetId()));
        UNIT_ASSERT(s.contains(TXYSetId(EXType::XFoo)));
        UNIT_ASSERT(s.contains(TXYSetId(EYType::YFoo, EYType::YBar)));
        UNIT_ASSERT(!s.contains(TXYSetId(EXType::XBar)));
        UNIT_ASSERT(!s.contains(TXYSetId(EXType::XFoo, EYType::YBar)));
        UNIT_ASSERT(!s.contains(TXYSetId(EYType::YFoo)));
    }

};

UNIT_TEST_SUITE_REGISTRATION(TIdTest);
