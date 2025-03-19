#include "../simpledate.h"
#include <kernel/common_server/library/simpledate/ut/proto/simpledate.pb.h>
#include <kernel/common_server/util/instant_model.h>
#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(SimpleDate) {
    Y_UNIT_TEST(CreateFromInstant) {
        TInstant date;
        CHECK_WITH_LOG(TInstant::TryParseIso8601("2015-11-22T02:30:00.000000+0300", date));

        NCS::TSimpleDate simpleDate(date);

        UNIT_ASSERT_EQUAL(simpleDate.GetYear(), 2015);
        UNIT_ASSERT_EQUAL(simpleDate.GetMonth(), 11);
        UNIT_ASSERT_EQUAL(simpleDate.GetDay(), 21);
    }

    Y_UNIT_TEST(Comparison) {
        TInstant date;
        CHECK_WITH_LOG(TInstant::TryParseIso8601("2015-11-22T02:30:00.000000+0300", date));
        NCS::TSimpleDate simpleDate(date);

        UNIT_ASSERT(simpleDate < NCS::TSimpleDate(ModelingNow()));
        UNIT_ASSERT(NCS::TSimpleDate(ModelingNow()) > simpleDate);
        UNIT_ASSERT(simpleDate == NCS::TSimpleDate(date));

        TInstant date2;
        CHECK_WITH_LOG(TInstant::TryParseIso8601("2015-11-20T02:30:00.000000+0300", date2));
        UNIT_ASSERT(simpleDate > NCS::TSimpleDate(date2));
    }

    Y_UNIT_TEST(DeserializeFromProto) {
        TProtoSimpleDate protoSimpleDate;
        protoSimpleDate.SetYear(2020);
        protoSimpleDate.SetMonth(1);
        protoSimpleDate.SetDay(1);
        NCS::TSimpleDate date;
        CHECK_WITH_LOG(date.DeserializeFromProto(protoSimpleDate));
    }
}
