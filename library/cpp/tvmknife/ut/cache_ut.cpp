#include <library/cpp/tvmknife/cache.h>

#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/system/fs.h>

using namespace NTvmknife;

Y_UNIT_TEST_SUITE(Cache) {
    Y_UNIT_TEST(serializeSrvTickets) {
        std::map<TCache::TSrvTick, TString> s;
        s.emplace(TCache::TSrvTick{123, 456, "vasya", std::numeric_limits<time_t>::max()}, "ticket_body");
        s.emplace(TCache::TSrvTick{789, 456, "vasya", 1}, "ticket_body3");
        s.emplace(TCache::TSrvTick{123, 456, "vasya", std::numeric_limits<time_t>::max() - 1}, "ticket_body");
        s.emplace(TCache::TSrvTick{123, 456, "", std::numeric_limits<time_t>::max() - 1}, "ticket_body2");

        TString ser = TCache::SerializeSrvTickets(s);
        UNIT_ASSERT_STRINGS_EQUAL("Ch0IexDIAyIMdGlja2V0X2JvZHkyKP7_________fwojCHsQyAMaBXZhc3lhIgt0aWNrZXRfYm9keSj__________38KHQiVBhDIAxoFdmFzeWEiDHRpY2tldF9ib2R5MygB",
                                  Base64EncodeUrl(ser));

        s = TCache::ParseSrvTickets(ser);
        UNIT_ASSERT_VALUES_EQUAL(2, s.size());
        UNIT_ASSERT_VALUES_EQUAL(123, s.rbegin()->first.Src);
        UNIT_ASSERT_VALUES_EQUAL(456, s.rbegin()->first.Dst);
        UNIT_ASSERT_STRINGS_EQUAL("vasya", s.rbegin()->first.Login);
        UNIT_ASSERT_VALUES_EQUAL(std::numeric_limits<time_t>::max(), s.rbegin()->first.Ts);
        UNIT_ASSERT_STRINGS_EQUAL("ticket_body", s.rbegin()->second);
        UNIT_ASSERT_VALUES_EQUAL(123, s.begin()->first.Src);
        UNIT_ASSERT_VALUES_EQUAL(456, s.begin()->first.Dst);
        UNIT_ASSERT_STRINGS_EQUAL("", s.begin()->first.Login);
        UNIT_ASSERT_VALUES_EQUAL(std::numeric_limits<time_t>::max() - 1, s.begin()->first.Ts);
        UNIT_ASSERT_STRINGS_EQUAL("ticket_body2", s.begin()->second);
    }

    Y_UNIT_TEST(writeAndRead) {
        NFs::Remove("./service_tickets");

        TCache c;
        c.SetCacheDir("./");

        const TString tick1 = "tick #1";
        const TString tick2 = "tick #2";

        UNIT_ASSERT_STRINGS_EQUAL("", c.GetSrvTicket(789, 456, "vasya"));
        UNIT_ASSERT(!NFs::Exists("./service_tickets"));
        c.SetSrvTicket(tick1, 789, 456);
        c.SetSrvTicket(tick2, 789, 456, "vasya");
        UNIT_ASSERT(NFs::Exists("./service_tickets"));

        UNIT_ASSERT_STRINGS_EQUAL(tick1, c.GetSrvTicket(789, 456));
        UNIT_ASSERT_STRINGS_EQUAL(tick2, c.GetSrvTicket(789, 456, "vasya"));
        UNIT_ASSERT_STRINGS_EQUAL("", c.GetSrvTicket(789, 456, "oleg"));
        UNIT_ASSERT_STRINGS_EQUAL("", c.GetSrvTicket(123, 456));

        c.Reset();

        UNIT_ASSERT_STRINGS_EQUAL(tick1, c.GetSrvTicket(789, 456));
        UNIT_ASSERT_STRINGS_EQUAL(tick2, c.GetSrvTicket(789, 456, "vasya"));
        UNIT_ASSERT_STRINGS_EQUAL("", c.GetSrvTicket(789, 456, "oleg"));
        UNIT_ASSERT_STRINGS_EQUAL("", c.GetSrvTicket(123, 456));
    }
}
