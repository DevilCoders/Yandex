#include <kernel/ugc/datatypes/objectid.h>
#include <library/cpp/testing/unittest/registar.h>

namespace NUgc {
    class TObjectIdTest: public TTestBase {
    public:
        UNIT_TEST_SUITE(TObjectIdTest)
        UNIT_TEST(StandardObjectIdsAreValid)
        UNIT_TEST(AppendOntodbPrefixForIdsWithoudNamespace)
        UNIT_TEST(AppendOntodbPrefixForIdsWithNamespace)
        UNIT_TEST(EmptyIdIsInvalid)
        UNIT_TEST(NotNumericPermalinkIsInvalid)
        UNIT_TEST(CreateObjectIdFromPermalink)
        UNIT_TEST(CreateObjectIdFromPermalinkInteger)
        UNIT_TEST(CreateObjectIdFromOntoid)
        UNIT_TEST(StripOntodbPrefixOnlyForUgcdbKey)
        UNIT_TEST(CreateObjectIdFromSiteId)
        UNIT_TEST(CreateObjectIdFromSiteOwner)
        UNIT_TEST(CreateOwnerFromSiteId)
        UNIT_TEST(CreateOwnerFromPunycode)
        UNIT_TEST(CreateObjectIdFromMarketModelId)
        UNIT_TEST(CreateObjectIdFromMarketShopId)
        UNIT_TEST(CreateMarketModelObjectIdFromNumeric)
        UNIT_TEST(CreateMarketShopObjectIdFromNumeric)
        UNIT_TEST_SUITE_END();

        void StandardObjectIdsAreValid() {
            UNIT_ASSERT_NO_EXCEPTION(TObjectId("/sprav/123"));
            UNIT_ASSERT_NO_EXCEPTION(TObjectId("/ontoid/ruw123"));
        }

        void AppendOntodbPrefixForIdsWithoudNamespace() {
            auto objectId = TObjectId("ruw123");
            UNIT_ASSERT_STRINGS_EQUAL(objectId.AsString(), "/ontoid/ruw123");
        }

        void AppendOntodbPrefixForIdsWithNamespace() {
            auto objectId = TObjectId("/ontoid/ruw123");
            UNIT_ASSERT_STRINGS_EQUAL(objectId.AsString(), "/ontoid/ruw123");
        }

        void EmptyIdIsInvalid() {
            UNIT_ASSERT_EXCEPTION(TObjectId(""), TBadArgumentException);
        }

        void NotNumericPermalinkIsInvalid() {
            UNIT_ASSERT_EXCEPTION(TObjectId::FromSpravPermalink("abc"), TBadArgumentException);
        }

        void CreateObjectIdFromPermalink() {
            auto objectId = TObjectId::FromSpravPermalink("123");
            UNIT_ASSERT(objectId.IsSpravObject());
            UNIT_ASSERT_STRINGS_EQUAL(objectId.AsString(), "/sprav/123");
        }

        void CreateObjectIdFromPermalinkInteger() {
            auto objectId = TObjectId::FromSpravPermalink(1234567890123ul);
            UNIT_ASSERT(objectId.IsSpravObject());
            UNIT_ASSERT_STRINGS_EQUAL(objectId.AsString(), "/sprav/1234567890123");
        }

        void CreateObjectIdFromOntoid() {
            auto objectId = TObjectId::FromOntodbOntoid("ruw123");
            UNIT_ASSERT(objectId.IsOntodbObject());
            UNIT_ASSERT_STRINGS_EQUAL(objectId.AsString(), "/ontoid/ruw123");
        }

        void StripOntodbPrefixOnlyForUgcdbKey() {
            UNIT_ASSERT_STRINGS_EQUAL(TObjectId::FromOntodbOntoid("ruw123").AsUgcdbKey(), "ruw123");
            UNIT_ASSERT_STRINGS_EQUAL(TObjectId::FromSpravPermalink("123").AsUgcdbKey(), "/sprav/123");
        }

        void CreateObjectIdFromSiteId() {
            auto objectId = TObjectId("/site/123");
            UNIT_ASSERT(objectId.IsSiteObject());
            UNIT_ASSERT_STRINGS_EQUAL(objectId.AsString(), "/site/123");
        }

        void CreateObjectIdFromSiteOwner() {
            UNIT_ASSERT_STRINGS_EQUAL(TObjectId::FromSiteOwner("yandex.ru").AsString(), "/site/eWFuZGV4LnJ1");
            UNIT_ASSERT_STRINGS_EQUAL(TObjectId::FromSiteOwner("baza.io").AsString(), "/site/YmF6YS5pbw==");
        }

        void CreateOwnerFromSiteId() {
            {
                auto objectId = TObjectId("/site/eWFuZGV4LnJ1");
                UNIT_ASSERT_STRINGS_EQUAL(TObjectId::AsSiteOwner(objectId), "yandex.ru");
            }
            {
                auto objectId = TObjectId("/site/YmF6YS5pbw==");
                UNIT_ASSERT_STRINGS_EQUAL(TObjectId::AsSiteOwner(objectId), "baza.io");
            }
        }

        void CreateOwnerFromPunycode() {
            {
                auto objectId = TObjectId::FromSiteOwner("xn--e1aaupct.xn--p1ai");
                UNIT_ASSERT_STRINGS_EQUAL(TObjectId::AsSiteOwner(objectId), "фермер.рф");
            }
            {
                auto objectId = TObjectId::FromSiteOwner("asdf.xn--p1ai");
                UNIT_ASSERT_STRINGS_EQUAL(TObjectId::AsSiteOwner(objectId), "asdf.рф");
            }
        }

        void CreateObjectIdFromMarketModelId() {
            auto objectId = TObjectId("/market/model/123");
            UNIT_ASSERT(objectId.IsMarketModelObject());
            UNIT_ASSERT_STRINGS_EQUAL(objectId.AsString(), "/market/model/123");
            UNIT_ASSERT_STRINGS_EQUAL(objectId.GetMarketModel(), "123");
        }

        void CreateObjectIdFromMarketShopId() {
            auto objectId = TObjectId("/market/shop/123");
            UNIT_ASSERT(objectId.IsMarketShopObject());
            UNIT_ASSERT_STRINGS_EQUAL(objectId.AsString(), "/market/shop/123");
            UNIT_ASSERT_STRINGS_EQUAL(objectId.GetMarketShop(), "123");
        }

        void CreateMarketModelObjectIdFromNumeric() {
            auto objectId = TObjectId::FromMarketModelId(111);
            UNIT_ASSERT(objectId.IsMarketModelObject());
            UNIT_ASSERT_STRINGS_EQUAL(objectId.AsString(), "/market/model/111");
            UNIT_ASSERT_STRINGS_EQUAL(objectId.GetMarketModel(), "111");
        }

        void CreateMarketShopObjectIdFromNumeric() {
            auto objectId = TObjectId::FromMarketShopId(111);
            UNIT_ASSERT(objectId.IsMarketShopObject());
            UNIT_ASSERT_STRINGS_EQUAL(objectId.AsString(), "/market/shop/111");
            UNIT_ASSERT_STRINGS_EQUAL(objectId.GetMarketShop(), "111");
        }
    };

    UNIT_TEST_SUITE_REGISTRATION(TObjectIdTest);
} // namespace NUgc

