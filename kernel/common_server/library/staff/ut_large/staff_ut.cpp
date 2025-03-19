#include <kernel/common_server/library/staff/client.h>
#include <library/cpp/testing/unittest/tests_data.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/string/builder.h>
#include <util/system/env.h>
#include <util/generic/vector.h>

namespace {
    NExternalAPI::TSenderConfig GetStaffConfig() {
        NExternalAPI::TSenderConfig config;
        config.SetHost("staff-api.test.yandex-team.ru");
        config.SetPort(443);
        config.SetIsHttps(true);
        return config;
    }

    NCS::TStaffClient GetStaffClient() {
        return NCS::TStaffClient(MakeAtomicShared<NExternalAPI::TSender>(GetStaffConfig(), "staff"));
    }
}

Y_UNIT_TEST_SUITE(StaffClient) {
    Y_UNIT_TEST(GetUserDataByLogin) {
        auto client = GetStaffClient();

        TVector<TStaffEntry> results;
        const TString username = "robot-carsharing";

        UNIT_ASSERT(
            client.GetUserData(
                TStaffEntrySelector(TStaffEntrySelector::EStaffEntryField::Username, username),
                results,
                {} /* all fields from enum */
            )
        );
        UNIT_ASSERT_EQUAL(results.size(), 1);

        auto userData = results.front();

        UNIT_ASSERT_STRINGS_EQUAL(userData.GetUsername(), username);
        UNIT_ASSERT_STRINGS_EQUAL(userData.GetWorkEmail(), "robot-carsharing@yandex-team.ru");
        UNIT_ASSERT(!userData.GetWorkPhone());
        UNIT_ASSERT(!userData.GetMainMobilePhone());
        UNIT_ASSERT_EQUAL(userData.GetMobilePhones().size(), 0);
        UNIT_ASSERT_STRINGS_EQUAL(userData.GetDepartmentUrl(), "virtual_robots");
        UNIT_ASSERT_VALUES_EQUAL(userData.GetUid(), 1120000000067712ul);
        UNIT_ASSERT_VALUES_EQUAL(userData.GetQuitAt(), TInstant::Zero());
        UNIT_ASSERT(!userData.IsDeleted());
        UNIT_ASSERT(!userData.IsDismissed());

        auto userNames = userData.GetName();

        UNIT_ASSERT(userNames.contains(TStaffEntry::EStaffNameLocale::EN));
        UNIT_ASSERT_VALUES_EQUAL(userNames[TStaffEntry::EStaffNameLocale::EN].GetFirstName(), username);
        UNIT_ASSERT_VALUES_EQUAL(userNames[TStaffEntry::EStaffNameLocale::EN].GetLastName(), username);
        UNIT_ASSERT_VALUES_EQUAL(userNames[TStaffEntry::EStaffNameLocale::EN].GetMiddleName(), "");

        UNIT_ASSERT(userNames.contains(TStaffEntry::EStaffNameLocale::RU));
        UNIT_ASSERT_VALUES_EQUAL(userNames[TStaffEntry::EStaffNameLocale::RU].GetFirstName(), username);
        UNIT_ASSERT_VALUES_EQUAL(userNames[TStaffEntry::EStaffNameLocale::RU].GetLastName(), username);
        UNIT_ASSERT_VALUES_EQUAL(userNames[TStaffEntry::EStaffNameLocale::RU].GetMiddleName(), "");
    }

    Y_UNIT_TEST(GetUserDataByUid) {
        auto client = GetStaffClient();

        TVector<TStaffEntry> results;

        const TString username = "robot-carsharing";
        const ui64 uid = 1120000000067712ul;

        UNIT_ASSERT(
            client.GetUserData(
                TStaffEntrySelector(TStaffEntrySelector::EStaffEntryField::Uid, ToString(uid)),
                results,
                {} /* all fields from enum */
            )
        );
        UNIT_ASSERT_EQUAL(results.size(), 1);

        auto userData = results.front();

        UNIT_ASSERT_STRINGS_EQUAL(userData.GetUsername(), username);
        UNIT_ASSERT_VALUES_EQUAL(userData.GetUid(), uid);
    }

    Y_UNIT_TEST(GetUserDataByWorkEmail) {
        auto client = GetStaffClient();

        TVector<TStaffEntry> results;

        const TString username = "robot-carsharing";
        const TString workEmail = "robot-carsharing@yandex-team.ru";

        UNIT_ASSERT(
            client.GetUserData(
                TStaffEntrySelector(TStaffEntrySelector::EStaffEntryField::WorkEmail, workEmail),
                results,
                {} /* all fields from enum */
            )
        );
        UNIT_ASSERT_EQUAL(results.size(), 1);

        auto userData = results.front();

        UNIT_ASSERT_STRINGS_EQUAL(userData.GetUsername(), username);
        UNIT_ASSERT_VALUES_EQUAL(userData.GetWorkEmail(), workEmail);
    }

    Y_UNIT_TEST(GetUserDataByWorkPhone) {
        auto client = GetStaffClient();

        TVector<TStaffEntry> results;

        const TString username = "mg";
        const ui64 workPhone = 4105;

        UNIT_ASSERT(
            client.GetUserData(
                TStaffEntrySelector(TStaffEntrySelector::EStaffEntryField::WorkPhone, ToString(workPhone)),
                results,
                {} /* all fields from enum */
            )
        );
        UNIT_ASSERT_EQUAL(results.size(), 1);

        auto userData = results.front();

        UNIT_ASSERT_STRINGS_EQUAL(userData.GetUsername(), username);
        UNIT_ASSERT_VALUES_EQUAL(userData.GetWorkPhone(), workPhone);
    }

    Y_UNIT_TEST(GetUserDataFromDepatment) {
        auto client = GetStaffClient();

        TVector<TStaffEntry> results;

        const TString departmentUrl = "yandex_content_8006";  // Yandex Drive
        const size_t limit = 50;

        UNIT_ASSERT(
            client.GetUserData(
                TStaffEntrySelector(TStaffEntrySelector::EStaffEntryField::DepartmentUrl, departmentUrl),
                results,
                limit /* to split on pages */,
                {} /* all fields from enum */
            )
        );
        UNIT_ASSERT_GE(results.size(), limit);  // total number of Yandex Drive employees
    }
}
