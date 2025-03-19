#include "record_identifier.h"

#include "exception.h"
#include <library/cpp/testing/unittest/registar.h>

using namespace NUgc::NSecurity;

class TRecordIdentifierTest: public TTestBase {
public:
    UNIT_TEST_SUITE(TRecordIdentifierTest);
    UNIT_TEST(GeneratesByHash);
    UNIT_TEST(GeneratesByHashFromString);
    UNIT_TEST(GeneratesByHashWithContextId);
    UNIT_TEST(GeneratesByHashWithContextIdFromString);
    UNIT_TEST(GeneratesByHashWithParentId);
    UNIT_TEST(GeneratesByHashWithParentIdFromString);
    UNIT_TEST(GenerateByHashFailsWithNoNamespace);
    UNIT_TEST(GenerateByHashFailsWithShortNamespace);
    UNIT_TEST(GenerateByHashFailsWithShortNamespaceFromString);
    UNIT_TEST(GenerateByHashFailsWithNoUserId);
    UNIT_TEST(GenerateByHashFailsWithShortUserId);
    UNIT_TEST(GenerateByHashFailsWithShortUserIdFromString);
    UNIT_TEST_SUITE_END();

    void SetUp() override {
        Bundle.SetNamespace("/my/awesome/ugc/");
        Bundle.SetUserId("/user/123456");
    }

    void TearDown() override {
        Bundle.Clear();
    }

    // Simply pinning down some values to keep them stable and also to validate the length and
    // variability expectations.
    void GeneratesByHash() {
        TString identifier;
        UNIT_ASSERT_NO_EXCEPTION(identifier = GenerateRecordIdentifierByHash(Bundle));
        UNIT_ASSERT_STRINGS_EQUAL("CQ7IK_jWBKgp4rF65kAejuTu6dybzl", identifier);
    }

    void GeneratesByHashFromString() {
        TString identifier;
        UNIT_ASSERT_NO_EXCEPTION(identifier = TRecordIdentifierGenerator("/my/awesome/ugc/", "/user/123456").Build());
        UNIT_ASSERT_STRINGS_EQUAL("CQ7IK_jWBKgp4rF65kAejuTu6dybzl", identifier);
    }

    void GeneratesByHashWithContextId() {
        Bundle.SetContextId("some identifier");
        TString identifier;
        UNIT_ASSERT_NO_EXCEPTION(identifier = GenerateRecordIdentifierByHash(Bundle));
        UNIT_ASSERT_STRINGS_EQUAL("pa23nK6lpJWtXDUs_T_mrWOHonSUFC", identifier);
    }

    void GeneratesByHashWithContextIdFromString() {
        TString identifier;
        UNIT_ASSERT_NO_EXCEPTION(identifier = TRecordIdentifierGenerator("/my/awesome/ugc/", "/user/123456").ContextId("some identifier").Build());
        UNIT_ASSERT_STRINGS_EQUAL("pa23nK6lpJWtXDUs_T_mrWOHonSUFC", identifier);
    }

    void GeneratesByHashWithParentId() {
        Bundle.SetParentId("some identifier");
        TString identifier;
        UNIT_ASSERT_NO_EXCEPTION(identifier = GenerateRecordIdentifierByHash(Bundle));
        UNIT_ASSERT_STRINGS_EQUAL("FLkrN4n7FoKbhagsNAB-75EgQcY_9pPG", identifier);
    }

    void GeneratesByHashWithParentIdFromString() {
        TString identifier;
        UNIT_ASSERT_NO_EXCEPTION(identifier = TRecordIdentifierGenerator("/my/awesome/ugc/", "/user/123456").ParentId("some identifier").Build());
        UNIT_ASSERT_STRINGS_EQUAL("FLkrN4n7FoKbhagsNAB-75EgQcY_9pPG", identifier);
    }

    void GenerateByHashFailsWithNoNamespace() {
        Bundle.ClearNamespace();
        TString identifier;
        UNIT_ASSERT_EXCEPTION(
            identifier = GenerateRecordIdentifierByHash(Bundle),
            TApplicationException);
    }

    void GenerateByHashFailsWithShortNamespace() {
        Bundle.SetNamespace("");
        TString identifier;
        UNIT_ASSERT_EXCEPTION(
                identifier = GenerateRecordIdentifierByHash(Bundle),
                TApplicationException);
    }

    void GenerateByHashFailsWithShortNamespaceFromString() {
        TString identifier;
        UNIT_ASSERT_EXCEPTION(
                identifier = TRecordIdentifierGenerator("", "").Build(),
                TApplicationException);
    }

    void GenerateByHashFailsWithNoUserId() {
        Bundle.ClearUserId();
        TString identifier;
        UNIT_ASSERT_EXCEPTION(
            identifier = GenerateRecordIdentifierByHash(Bundle),
            TApplicationException);
    }

    void GenerateByHashFailsWithShortUserId() {
        Bundle.SetUserId("");
        TString identifier;
        UNIT_ASSERT_EXCEPTION(
            identifier = GenerateRecordIdentifierByHash(Bundle),
            TApplicationException);
    }

    void GenerateByHashFailsWithShortUserIdFromString() {
        TString identifier;
        UNIT_ASSERT_EXCEPTION(
                identifier = TRecordIdentifierGenerator("/my/awesome/ugc/", "").Build(),
                TApplicationException);
    }

    TRecordIdentifierBundle Bundle;
};

UNIT_TEST_SUITE_REGISTRATION(TRecordIdentifierTest);
