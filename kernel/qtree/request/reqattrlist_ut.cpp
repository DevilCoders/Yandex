#include <library/cpp/testing/unittest/registar.h>
#include "reqattrlist.h"

class TReqAttrListTest : public TTestBase {
    UNIT_TEST_SUITE(TReqAttrListTest);
        UNIT_TEST(TestConstructFromString);
        UNIT_TEST(TestConstructFromArray);
        UNIT_TEST(TestTemplateAndExact);
    UNIT_TEST_SUITE_END();
public:
    void TestConstructFromString();
    void TestConstructFromArray();
    void TestTemplateAndExact();
};

UNIT_TEST_SUITE_REGISTRATION(TReqAttrListTest);

static const wchar16 zoneZ[] = u"z";
static const wchar16 zoneA[] = u"a";
static const wchar16 zoneB[] = u"b";
static const wchar16 attrDomain[] = u"domain";
static const wchar16 attrLink[] = u"link";
static const wchar16 zoneTitle[] = u"title";
static const wchar16 attrHost[] = u"host";
static const wchar16 attrRHost[] = u"rhost";
static const wchar16 attrURL[] = u"url";
static const wchar16 zoneAnchor[] = u"anchor";

static const wchar16 attrInpos[] = u"inpos";
static const wchar16 attrKeyPrefix[] = u"keyprefix";
static const wchar16 attrPrevreq[] = u"prevreq";
static const wchar16 attrSoftness[] = u"softness";

void TReqAttrListTest::TestConstructFromString() {
    TReqAttrList attrList(
        "z:ZONE\n"
        "a:ZONE\n"
        "b:ZONE\n"
        "domain:ATTR_URL,docurl\n"
        "link:ATTR_URL,zone\n"
        " \t title \t : \t ZONE \t \n"
        "host:ATTR_URL,doc\n"
        "rhost:ATTR_URL,doc\n"
        "url:ATTR_URL,doc\n"
        "anchor:ZONE");

    UNIT_ASSERT(attrList.GetType(zoneZ) == RA_ZONE);
    UNIT_ASSERT(attrList.GetType(zoneA) == RA_ZONE);
    UNIT_ASSERT(attrList.GetType(zoneB) == RA_ZONE);
    UNIT_ASSERT(attrList.GetType(attrDomain) == RA_ATTR_URL);
    UNIT_ASSERT(attrList.GetType(attrLink) == RA_ATTR_URL);
    UNIT_ASSERT(attrList.GetType(zoneTitle) == RA_ZONE);
    UNIT_ASSERT(attrList.GetType(attrHost) == RA_ATTR_URL);
    UNIT_ASSERT(attrList.GetType(attrRHost) == RA_ATTR_URL);
    UNIT_ASSERT(attrList.GetType(attrURL) == RA_ATTR_URL);
    UNIT_ASSERT(attrList.GetType(zoneAnchor) == RA_ZONE);

    UNIT_ASSERT(attrList.GetType(attrInpos) == RA_ATTR_SPECIAL);
    UNIT_ASSERT(attrList.GetType(attrKeyPrefix) == RA_ATTR_SPECIAL);
    UNIT_ASSERT(attrList.GetType(attrPrevreq) == RA_ATTR_SPECIAL);
    UNIT_ASSERT(attrList.GetType(attrSoftness) == RA_ATTR_SPECIAL);
}

void TReqAttrListTest::TestConstructFromArray() {
    TVector<TString> attrs{
        "z:ZONE",
        "a:ZONE",
        "b:ZONE",
        "domain:ATTR_URL,docurl",
        "link:ATTR_URL,zone",
        " \t title \t : \t ZONE \t ",
        "host:ATTR_URL,doc",
        "rhost:ATTR_URL,doc",
        "url:ATTR_URL,doc",
        "anchor:ZONE"};

    TReqAttrList attrList(attrs);

    UNIT_ASSERT(attrList.GetType(zoneZ) == RA_ZONE);
    UNIT_ASSERT(attrList.GetType(zoneA) == RA_ZONE);
    UNIT_ASSERT(attrList.GetType(zoneB) == RA_ZONE);
    UNIT_ASSERT(attrList.GetType(attrDomain) == RA_ATTR_URL);
    UNIT_ASSERT(attrList.GetType(attrLink) == RA_ATTR_URL);
    UNIT_ASSERT(attrList.GetType(zoneTitle) == RA_ZONE);
    UNIT_ASSERT(attrList.GetType(attrHost) == RA_ATTR_URL);
    UNIT_ASSERT(attrList.GetType(attrRHost) == RA_ATTR_URL);
    UNIT_ASSERT(attrList.GetType(attrURL) == RA_ATTR_URL);
    UNIT_ASSERT(attrList.GetType(zoneAnchor) == RA_ZONE);

    UNIT_ASSERT(attrList.GetType(attrInpos) == RA_ATTR_SPECIAL);
    UNIT_ASSERT(attrList.GetType(attrKeyPrefix) == RA_ATTR_SPECIAL);
    UNIT_ASSERT(attrList.GetType(attrPrevreq) == RA_ATTR_SPECIAL);
    UNIT_ASSERT(attrList.GetType(attrSoftness) == RA_ATTR_SPECIAL);
}

static const wchar16 author[] = u"author";
static const wchar16 author_[] = u"author_";
static const wchar16 author_name[] = u"author_name";
static const wchar16 author_id[] = u"author_id";
static const wchar16 author_iq[] = u"author_iq";

void TReqAttrListTest::TestTemplateAndExact() {
    TReqAttrList list(
        "author_:ATTR_LITERAL,doc,template\n"
        "author_id:ATTR_INTEGER,doc\n"
    );
    UNIT_ASSERT(list.GetType(author_id) == RA_ATTR_INTEGER);
    UNIT_ASSERT(list.GetType(author_name) == RA_ATTR_LITERAL);
    UNIT_ASSERT_EXCEPTION(list.Add(author_id, "ATTR_BOOLEAN,doc,template"), yexception);
    UNIT_ASSERT_EXCEPTION(list.Add(author_iq, "ATTR_BOOLEAN,doc,template"), yexception);
    UNIT_ASSERT_EXCEPTION(list.Add(author, "ATTR_BOOLEAN,doc,template"), yexception);
    UNIT_ASSERT_EXCEPTION(list.Add(author_, "ATTR_BOOLEAN,doc"), yexception);
}
