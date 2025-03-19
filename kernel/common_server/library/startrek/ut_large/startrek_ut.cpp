#include <kernel/common_server/library/startrek/client.h>
#include <kernel/common_server/library/startrek/config.h>
#include <library/cpp/testing/unittest/tests_data.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/string/builder.h>
#include <util/system/env.h>
#include <util/generic/vector.h>

namespace {
    TStartrekClientConfig GetStartrekConfig() {
        auto config = TStartrekClientConfig::ParseFromString(
            TStringBuilder()
            << "Host: st-api.test.yandex-team.ru" << Endl
            << "Port: 443" << Endl
            << "IsHttps: true" << Endl
            << "Account: robot-carsharing" << Endl
            << "Token: " << GetEnv("TESTING_STARTREK_TOKEN") << Endl
            << "TokenPath: " << GetEnv("TESTING_STARTREK_TOKEN_PATH") << Endl
            << "RequestTimeout: 1s" << Endl
            << "<RequestConfig>" << Endl
            << "    MaxAttempts: 1" << Endl
            << "</RequestConfig>" << Endl
        );
        return config;
    }

    TStartrekClient GetStartrekClient() {
        return TStartrekClient(GetStartrekConfig());
    }

    TString GetTicketStatus(const TStartrekClient& client, const TString& issueName) {
        TStartrekTicket ticketInfo;
        UNIT_ASSERT_C(client.GetIssueInfo(issueName, ticketInfo), "error obtaining ticket info");
        TString status;
        UNIT_ASSERT(ticketInfo.GetAdditionalValue(TStartrekTicket::ETicketField::StatusKey, status));
        return status;
    }

    void CheckUpdateTicketTags(const TStartrekClient& client, const TString& issueName, const TStartrekTicket& ticketUpdate, const TVector<TString>& expectedTags) {
        TStartrekTicket updatedInfo;
        UNIT_ASSERT(client.PatchIssue(issueName, ticketUpdate, updatedInfo));

        TVector<TString> tags;
        updatedInfo.GetAdditionalContainerValue("tags", tags);
        UNIT_ASSERT_VALUES_EQUAL(tags, expectedTags);
    }

    void CleanupOldComments(const TStartrekClient& client, const TString& issueName, const TDuration& limit = TDuration::Hours(1)) {
        TInstant now = TInstant::Now();

        TVector<ui64> commentIdsToDelete;
        do {
            commentIdsToDelete.clear();

            TStartrekClient::TComments comments;
            UNIT_ASSERT_C(client.GetAllComments(issueName, comments), "unable to get all comments");

            for (const auto& comment : comments) {
                if (now - comment.GetUpdatedAt() > limit) {
                    commentIdsToDelete.push_back(comment.GetId());
                }
            }

            for (const ui64 commentId : commentIdsToDelete) {
                client.DeleteComment(issueName, ToString(commentId));
            }
        } while (!commentIdsToDelete.empty());
    }
}

Y_UNIT_TEST_SUITE(StartrekClient) {
    Y_UNIT_TEST(ProcessComment) {
        static const TString issueName = "DRIVETEST-1";

        auto client = GetStartrekClient();
        TString comment = "Startrek client test comment at "+ TInstant::Now().ToString();

        TStartrekComment result;
        UNIT_ASSERT_C(client.AddComment(issueName, comment, result), "error adding comment");

        TStartrekComment check;
        UNIT_ASSERT_C(client.GetComment(issueName, ToString(result.GetId()), check), "error obtaining comment: " << errors.GetStringReport());
        UNIT_ASSERT_VALUES_EQUAL(result.GetId(), check.GetId());
        UNIT_ASSERT_STRINGS_EQUAL(comment, check.GetText());

        client.DeleteComment(issueName, ToString(result.GetId()));
        UNIT_ASSERT_C(!client.GetComment(issueName, ToString(result.GetId()), check), "comment has not been deleted");

        CleanupOldComments(client, issueName);
    }

    Y_UNIT_TEST(UpdateBody) {
        static const TString issueName = "DRIVETEST-1";
        static const size_t maxDescriptionLength = 4096;

        auto client = GetStartrekClient();

        TStartrekTicket ticketInfo;
        UNIT_ASSERT_C(client.GetIssueInfo(issueName, ticketInfo, obtainInfoErrors), "error obtaining ticket info");

        TString description = ticketInfo.GetDescription();

        if (description.length() >= maxDescriptionLength) {
            size_t defaultOffset = description.length() - maxDescriptionLength;
            size_t offset = description.find('\n', defaultOffset);
            if (offset == TString::npos) {
                offset = defaultOffset;
            }
            description = description.substr(offset + 1);
        }

        description += "\nStartrek client update at "+ TInstant::Now().ToString();

        TStartrekTicket issueUpdate;
        issueUpdate.SetDescription(description);

        TStartrekTicket patchResult;
        UNIT_ASSERT_C(client.PatchIssue(issueName, issueUpdate, patchResult, updateErrors), "error updating ticket");
        UNIT_ASSERT_STRINGS_EQUAL_C(description, patchResult.GetDescription(), "description set differs from target one");
    }

    Y_UNIT_TEST(ExecuteTransition) {
        static const TString issueName = "DRIVETEST-1";

        auto client = GetStartrekClient();
        TString status = GetTicketStatus(client, issueName);

        TString transition, targetStatus;
        if (status == "open") {
            transition = "start_progress";
            targetStatus = "inProgress";
        } else if (status == "inProgress") {
            transition = "stop_progress";
            targetStatus = "open";
        } else {
            UNIT_FAIL("cannot change ticket " << issueName << " status: status " << status << " is unknown");
        }

        UNIT_ASSERT_C(client.ExecuteTransition(issueName, transition), "error executing transition");

        TString updatedStatus = GetTicketStatus(client, issueName);
        UNIT_ASSERT_STRINGS_EQUAL_C(updatedStatus, targetStatus, "status set differs from target one");
    }

    Y_UNIT_TEST(ExecuteInvalidTransition) {
        static const TString issueName = "DRIVETEST-1";

        auto client = GetStartrekClient();
        TString status = GetTicketStatus(client, issueName);

        TString transition;
        if (status == "open") {
            transition = "stop_progress";
        } else if (status == "inProgress") {
            transition = "start_progress";
        } else {
            UNIT_FAIL("cannot change ticket " << issueName << " status: status " << status << " is unknown");
        }

        UNIT_ASSERT(!client.ExecuteTransition(issueName, transition));
    }

    Y_UNIT_TEST(ProcessTicketTags) {
        static const TString issueName = "DRIVETEST-4";

        auto client = GetStartrekClient();

        TStartrekTicket ticketSetTagsUpdate;
        ticketSetTagsUpdate.SetAdditionalContainerValue("tags", TVector<TString>({"some_tag_1"}));
        CheckUpdateTicketTags(client, issueName, ticketSetTagsUpdate, {"some_tag_1"});

        TStartrekTicket ticketAddTagsUpdate;
        ticketAddTagsUpdate.AddAdditionalContainerValue("tags", TVector<TString>({"some_tag_2"}));
        CheckUpdateTicketTags(client, issueName, ticketAddTagsUpdate, {"some_tag_1", "some_tag_2"});

        TStartrekTicket ticketRemoveTagsUpdate;
        ticketRemoveTagsUpdate.RemoveAdditionalContainerValue("tags", TVector<TString>({"some_tag_1", "some_tag_2"}));
        CheckUpdateTicketTags(client, issueName, ticketRemoveTagsUpdate, {});
    }

    Y_UNIT_TEST(ProcessTicketTagsInplace) {
        static const TString issueName = "DRIVETEST-4";

        auto client = GetStartrekClient();
        TStartrekTicket ticketInfo;
        UNIT_ASSERT(client.GetIssueInfo(issueName, ticketInfo));

        ticketInfo.SetAdditionalContainerValue("tags", TVector<TString>({"some_tag_1"}), true);

        TVector<TString> setTags;
        ticketInfo.GetAdditionalContainerValue("tags", setTags);
        UNIT_ASSERT_VALUES_EQUAL(setTags, TVector<TString>({"some_tag_1"}));

        ticketInfo.AddAdditionalContainerValue("tags", TVector<TString>({"some_tag_2"}), true);

        TVector<TString> addedTags;
        ticketInfo.GetAdditionalContainerValue("tags", addedTags);
        UNIT_ASSERT_VALUES_EQUAL(addedTags, TVector<TString>({"some_tag_1", "some_tag_2"}));

        ticketInfo.RemoveAdditionalContainerValue("tags", TVector<TString>({"some_tag_1", "some_tag_2"}), true);

        TVector<TString> removedTags;
        ticketInfo.GetAdditionalContainerValue("tags", removedTags);
        UNIT_ASSERT_VALUES_EQUAL(removedTags, TVector<TString>());
    }
}
