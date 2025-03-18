#include <library/cpp/testing/unittest/registar.h>

#include "cbb_banned_ip_holder.h"

#include <antirobot/lib/spravka.h>

using namespace NAntiRobot;

class TTestRequest : public TRequest {
public:
    TTestRequest(TUid uid, const TAddr& addr, bool canShowCaptcha=true) {
        Uid = uid;
        UserAddr = TFeaturedAddr(addr, TReloadableData{}, HOST_OTHER, false, false, false);
        IsSearch = canShowCaptcha;
    }

    void PrintData(IOutputStream&, bool /* forceMaskCookies */) const override {
    }
    void PrintHeaders(IOutputStream&, bool /* forceMaskCookies */) const override {
    }
    void SerializeTo(NCacheSyncProto::TRequest&) const override {
    }
};

auto EMPTY_LIST = MakeAtomicShared<NThreading::TRcuAccessor<TCbbAddrSet>>();

const TAddr& IPV4_ADDRESS = TAddr{"1.1.1.1"};
const TIpInterval& IPV4_INTERVAL = TIpInterval{TAddr{"1.1.1.1"}, TAddr{"1.1.1.2"}};

const TAddr& IPV4_ADDRESS_2 = TAddr{"2.2.2.2"};
const TIpInterval& IPV4_INTERVAL_2 = TIpInterval{TAddr{"2.2.2.2"}, TAddr{"2.2.2.3"}};

const TAddr& IPV4_ADDRESS_3 = TAddr{"3.3.3.3"};

const TAddr& IPV6_ADDRESS = TAddr{"2a02:6b8:0:40c:f5c6:cfef:6960:caa1"};
const TIpInterval& IPV6_INTERVAL = TIpInterval{IPV6_ADDRESS, IPV6_ADDRESS};

auto CreateList(std::initializer_list<std::pair<TIpInterval, TInstant>> params) {
    TCbbAddrSet addrSet;
    for (const auto&[interval, expired] : params) {
        addrSet.Add(interval, expired);
    }
    return MakeAtomicShared<NThreading::TRcuAccessor<TCbbAddrSet>>(addrSet);
}

class TTestCbbBannedIpHolderParams : public TTestBase {
public:
    void SetUp() override {
        ANTIROBOT_DAEMON_CONFIG_MUTABLE.LoadFromString("<Daemon>\n"
                                                       "CbbFlagIpBasedIdentificationsBan = 1111\n"
                                                       "CaptchaApiHost = ::\n"
                                                       "CbbApiHost = ::\n"
                                                       "</Daemon>");

        ANTIROBOT_DAEMON_CONFIG_MUTABLE.JsonConfig[HOST_OTHER].CbbFlagsFarmableIdentificationsBan = {TCbbGroupId{2222}};
    }
};

Y_UNIT_TEST_SUITE_IMPL(TTestCbbBannedIpHolder, TTestCbbBannedIpHolderParams) {
    Y_UNIT_TEST(TestIsBannedIpBased) {
        auto ipBasedList = CreateList({{IPV4_INTERVAL, TInstant::Max()}});
        TCbbBannedIpHolder holder(TCbbGroupId{1111}, {TVector<TCbbGroupId>{TCbbGroupId{2222}}}, ipBasedList, {EMPTY_LIST});

        UNIT_ASSERT_STRINGS_EQUAL("CBB_1111", holder.GetBanReason(TTestRequest{TUid::FromAddr(IPV4_ADDRESS), IPV4_ADDRESS}));

        UNIT_ASSERT_STRINGS_EQUAL("", holder.GetBanReason(TTestRequest{TUid::FromAddr(IPV4_ADDRESS_2), IPV4_ADDRESS_2}));
        UNIT_ASSERT_STRINGS_EQUAL("", holder.GetBanReason(TTestRequest{TUid::FromSpravka(TSpravka{}), IPV4_ADDRESS}));
        UNIT_ASSERT_STRINGS_EQUAL("", holder.GetBanReason(TTestRequest{TUid::FromFlashCookie(TFlashCookie{}), IPV4_ADDRESS}));
        UNIT_ASSERT_STRINGS_EQUAL("", holder.GetBanReason(TTestRequest{TUid::FromLCookie(NLCookie::TLCookie{}), IPV4_ADDRESS}));
        UNIT_ASSERT_STRINGS_EQUAL("", holder.GetBanReason(TTestRequest{TUid::FromICookie(NIcookie::TIcookieDataWithRaw{}), IPV4_ADDRESS}));
    }

    Y_UNIT_TEST(TestIsBannedFarmable) {
        auto farmableList = CreateList({{IPV4_INTERVAL_2, TInstant::Max()}});
        TCbbBannedIpHolder holder(TCbbGroupId{1111}, {TVector<TCbbGroupId>{TCbbGroupId{2222}}}, EMPTY_LIST, {farmableList});

        UNIT_ASSERT_STRINGS_EQUAL("CBB_2222", holder.GetBanReason(TTestRequest{TUid::FromAddr(IPV4_ADDRESS_2), IPV4_ADDRESS_2}));
        UNIT_ASSERT_STRINGS_EQUAL("CBB_2222", holder.GetBanReason(TTestRequest{TUid::FromFlashCookie(TFlashCookie{}), IPV4_ADDRESS_2}));
        UNIT_ASSERT_STRINGS_EQUAL("CBB_2222", holder.GetBanReason(TTestRequest{TUid::FromICookie(NIcookie::TIcookieDataWithRaw{}), IPV4_ADDRESS_2}));

        UNIT_ASSERT_STRINGS_EQUAL("", holder.GetBanReason(TTestRequest{TUid::FromFlashCookie(TFlashCookie{}), IPV4_ADDRESS}));
        UNIT_ASSERT_STRINGS_EQUAL("", holder.GetBanReason(TTestRequest{TUid::FromSpravka(TSpravka{}), IPV4_ADDRESS_2}));
        UNIT_ASSERT_STRINGS_EQUAL("", holder.GetBanReason(TTestRequest{TUid::FromLCookie(NLCookie::TLCookie{}), IPV4_ADDRESS_2}));
    }

    Y_UNIT_TEST(TestIsBannedIpV6) {
        auto ipBasedList = CreateList({{IPV6_INTERVAL, TInstant::Max()}});
        TCbbBannedIpHolder holder(TCbbGroupId{1111}, {TVector<TCbbGroupId>{TCbbGroupId{2222}}}, ipBasedList, {EMPTY_LIST});

        UNIT_ASSERT_EQUAL("CBB_1111", holder.GetBanReason(TTestRequest{TUid::FromAddr(IPV6_ADDRESS), IPV6_ADDRESS}));
    }

    Y_UNIT_TEST(TestGetBanReason) {
        auto ipBasedList = CreateList({{IPV4_INTERVAL, TInstant::Max()}});
        auto farmableList = CreateList({{IPV4_INTERVAL_2, TInstant::Max()}});
        TCbbBannedIpHolder holder(TCbbGroupId{1111}, {TVector<TCbbGroupId>{TCbbGroupId{2222}}}, ipBasedList, {farmableList});

        UNIT_ASSERT_STRINGS_EQUAL(holder.GetBanReason(TTestRequest{TUid::FromAddr(IPV4_ADDRESS), IPV4_ADDRESS}), "CBB_1111");
        UNIT_ASSERT_STRINGS_EQUAL(holder.GetBanReason(TTestRequest{TUid::FromAddr(IPV4_ADDRESS_2), IPV4_ADDRESS_2}), "CBB_2222");

        UNIT_ASSERT_STRINGS_EQUAL(holder.GetBanReason(TTestRequest{TUid::FromAddr(IPV4_ADDRESS_3), IPV4_ADDRESS_3}), "");
    }

    Y_UNIT_TEST(TestUpdateStats) {
        auto ipBasedList = CreateList({{IPV4_INTERVAL, TInstant::Max()}});
        auto farmableList = CreateList({{IPV4_INTERVAL_2, TInstant::Max()}});
        TCbbBannedIpHolder holder(TCbbGroupId{1111}, {TVector<TCbbGroupId>{TCbbGroupId{2222}}}, ipBasedList, {farmableList});

        holder.UpdateStats(TTestRequest{TUid::FromAddr(IPV4_ADDRESS), IPV4_ADDRESS});
        holder.UpdateStats(TTestRequest{TUid::FromAddr(IPV4_ADDRESS_2), IPV4_ADDRESS_2});
        holder.UpdateStats(TTestRequest{TUid::FromAddr(IPV4_ADDRESS), IPV4_ADDRESS, /* canShowCaptcha := */ false});

        UNIT_ASSERT_EQUAL(holder.Counters.Get(HOST_OTHER, TCbbBannedIpHolder::ECounter::Total), 2);
        UNIT_ASSERT_EQUAL(holder.Counters.Get(HOST_OTHER, TCbbBannedIpHolder::ECounter::IpBased), 1);
    }
}
