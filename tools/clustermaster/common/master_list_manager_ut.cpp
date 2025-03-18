#include "master_list_manager.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/algorithm.h>

Y_UNIT_TEST_SUITE(MasterListTest) {
    Y_UNIT_TEST_DECLARE(HostlistParser);
}

Y_UNIT_TEST_SUITE_IMPLEMENTATION(MasterListTest) {
    Y_UNIT_TEST(HostlistParser) {
        TString in("TAG1 one\nTAG1 TAG2 two\nthree\nTAG3:host1 0 1 2\nTAG3:host2 3..5\nTAG3 other\nTAG4:host1 0\nTAG4:host2 1\nTAG5 host0,host1..host3,host4");
        TMasterListManager manager;
        manager.LoadHostlistFromString(in);

        {
            TVector<TString> default_hosts;
            manager.GetList("hostlist", "", default_hosts);
            UNIT_ASSERT_EQUAL(default_hosts.size(), 1);
            UNIT_ASSERT_EQUAL(default_hosts.at(0), "three");
        }{
            TVector<TString> tag1_hosts;
            manager.GetList("hostlist:TAG1", "", tag1_hosts);
            UNIT_ASSERT_EQUAL(tag1_hosts.size(), 2);
            Sort(tag1_hosts.begin(), tag1_hosts.end()); // order doesn't matter
            UNIT_ASSERT_EQUAL(tag1_hosts.at(0), "one");
            UNIT_ASSERT_EQUAL(tag1_hosts.at(1), "two");
        }{
            TVector<TString> tag2_hosts;
            manager.GetList("hostlist:TAG2", "", tag2_hosts);
            UNIT_ASSERT_EQUAL(tag2_hosts.size(), 1);
            UNIT_ASSERT_EQUAL(tag2_hosts.at(0), "two");
        }{
            TVector<TString> tag3_items_host1;
            TVector<TString> tag3_items_host2;
            TVector<TString> tag3_items_generic;
            manager.GetList("hostlist:TAG3:clusters", "host1", tag3_items_host1);
            manager.GetList("hostlist:TAG3:clusters", "host2", tag3_items_host2);
            manager.GetList("hostlist:TAG3", "nonexistinghost", tag3_items_generic);
            UNIT_ASSERT_EQUAL(tag3_items_host1.size(), 3);
            UNIT_ASSERT_EQUAL(tag3_items_host1.at(0), "0");
            UNIT_ASSERT_EQUAL(tag3_items_host1.at(1), "1");
            UNIT_ASSERT_EQUAL(tag3_items_host1.at(2), "2");
            UNIT_ASSERT_EQUAL(tag3_items_host2.size(), 3);
            UNIT_ASSERT_EQUAL(tag3_items_host2.at(0), "3");
            UNIT_ASSERT_EQUAL(tag3_items_host2.at(1), "4");
            UNIT_ASSERT_EQUAL(tag3_items_host2.at(2), "5");
            UNIT_ASSERT_EQUAL(tag3_items_generic.size(), 3);
            UNIT_ASSERT_EQUAL(tag3_items_generic.at(0), "host1");
            UNIT_ASSERT_EQUAL(tag3_items_generic.at(1), "host2");
            UNIT_ASSERT_EQUAL(tag3_items_generic.at(2), "other");
        }{
            TVector<TString> hosts;
            UNIT_ASSERT_EXCEPTION(manager.GetList("hostlist:TAG4:clusters", "", hosts), yexception);
        }{
            TVector<TString> tag5_items;
            manager.GetList("hostlist:TAG5", "", tag5_items);
            UNIT_ASSERT_EQUAL(tag5_items.size(), 5);
            UNIT_ASSERT_EQUAL(tag5_items.at(0), "host0");
            UNIT_ASSERT_EQUAL(tag5_items.at(1), "host1");
            UNIT_ASSERT_EQUAL(tag5_items.at(2), "host2");
            UNIT_ASSERT_EQUAL(tag5_items.at(3), "host3");
            UNIT_ASSERT_EQUAL(tag5_items.at(4), "host4");
        }{
            TVector<TString> hosts;
            UNIT_ASSERT_EXCEPTION(manager.GetList("hostlist:NEVERWHERETAG", "", hosts), yexception);
        }
    }
}
