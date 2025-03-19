import os
import pytest
from yc_common.exceptions import ApiError

from cloud.marketplace.common.yc_marketplace_common.client import MarketplaceClient
from cloud.marketplace.common.yc_marketplace_common.models.billing.person import CompanyPersonData
from cloud.marketplace.common.yc_marketplace_common.models.billing.person import Person
from cloud.marketplace.common.yc_marketplace_common.models.billing.person import PersonRequest
from cloud.marketplace.common.yc_marketplace_common.models.billing.publisher_account import VAT
from cloud.marketplace.common.yc_marketplace_common.models.publisher import Publisher

TEST_DATA = {
    "publisher": None,
    "publisher_name": "Test publisher",
    "publisher_name_2": "Test 2 publisher",
    "publisher_contacts": {
        "uri": "Test uri",
    },
}
script_dir = os.path.dirname(__file__)
test_image = "test_data/ubuntu.png"


@pytest.mark.parametrize("vat", VAT.ALL)
def test_publisher_create(marketplace_client: MarketplaceClient, generate_id, default_publisher_category, vat):
    TEST_DATA["publisher_logo_id"] = generate_id()
    create_operation = marketplace_client.create_publisher(
        name=TEST_DATA["publisher_name"],
        # logo_id=TEST_DATA["publisher_logo_id"],
        contact_info=TEST_DATA["publisher_contacts"],
        billing_account_id=generate_id(),
        billing=PersonRequest.new(
            company=CompanyPersonData({
                "name": "Test",
                "longname": "Test Company",
                "phone": "89680000000",
                "email": "test@yandex.ru",
                "post_code": "117534",
                "post_address": "Moscow, Russia",
                "legal_address": "Moscow, Russia",
                "inn": "1234567890",
                "kpp": "071143234",
                "bik": "040173604",
                "is_partner": False,
            }),
            type=Person.Type.COMPANY,
        ),
        meta={
            "test": {"ru": "ru text"},
        },
        vat=vat,
    )

    assert create_operation.done is False
    assert create_operation.response is None

    publisher = marketplace_client.get_manage_publisher(create_operation.metadata.publisher_id)

    assert publisher.status == Publisher.Status.PENDING
    assert publisher.meta.get("test") == "ru text"
    assert publisher.name == TEST_DATA["publisher_name"]
    assert publisher.contact_info is not None and publisher.contact_info.get("uri") == TEST_DATA["publisher_contacts"][
        "uri"
    ]

    assert len(publisher.categories) == 1
    assert publisher.categories[0] == default_publisher_category


def test_publisher_create_duplication(marketplace_client: MarketplaceClient, generate_id, db_fixture):
    TEST_DATA["publisher_logo_id"] = generate_id()
    with pytest.raises(ApiError) as api_err:
        marketplace_client.create_publisher(
            name=TEST_DATA["publisher_name"],
            logo_id=TEST_DATA["publisher_logo_id"],
            contact_info=TEST_DATA["publisher_contacts"],
            billing_account_id=db_fixture["publishers"][0]["billing_account_id"],
            billing=PersonRequest.new(
                company=CompanyPersonData({
                    "name": "Test",
                    "longname": "Test Company",
                    "phone": "89680000000",
                    "email": "test@yandex.ru",
                    "post_code": "117534",
                    "post_address": "Moscow, Russia",
                    "legal_address": "Moscow, Russia",
                    "inn": "1234567890",
                    "kpp": "071143234",
                    "bik": "040173604",
                    "is_partner": False,
                }),
                type=Person.Type.COMPANY,
            ),
            meta={
                "test": {"ru": "ru text"},
            },
        )

    assert "Publisher attributes conflict" in str(api_err.value)


def test_publisher_get(marketplace_client, db_fixture):
    publisher_id = db_fixture["publishers"][0]["id"]
    publisher = marketplace_client.get_publisher(publisher_id)
    assert set(publisher.categories) == {
        ordering["category_id"] for ordering in db_fixture["ordering"] if ordering["resource_id"] == publisher_id}

    publisher2_id = db_fixture["publishers"][1]["id"]
    with pytest.raises(ApiError) as api_err:
        marketplace_client.get_publisher(publisher2_id)
    assert "Invalid publisher ID." in str(api_err.value)

    publisher2 = marketplace_client.get_manage_publisher(publisher2_id)
    assert set(publisher2.categories) == {
        ordering["category_id"] for ordering in db_fixture["ordering"] if ordering["resource_id"] == publisher2_id}


def test_publisher_listing(marketplace_client, db_fixture, default_publisher_category):
    publishers = marketplace_client.list_publishers()
    assert len(publishers.publishers) == 1
    assert set(publishers.publishers[0].categories) == {
        db_fixture["category"][1]["id"],
        db_fixture["category"][2]["id"],
        default_publisher_category,
    }
    publishers2 = marketplace_client.list_manage_publishers(
        billing_account_id=db_fixture["publishers"][0]["billing_account_id"])
    assert len(publishers2.publishers) == 2
    assert db_fixture["publishers"][0]["id"] == publishers2.publishers[1].id
    assert db_fixture["publishers"][1]["id"] == publishers2.publishers[0].id

    publishers3 = marketplace_client.list_manage_publishers(
        billing_account_id=db_fixture["publishers"][0]["billing_account_id"],
        filter_query="categoryId='{}'".format(
            db_fixture["category"][1]["id"]))
    assert len(publishers3.publishers) == 2
    assert db_fixture["publishers"][0]["id"] == publishers3.publishers[0].id
    assert db_fixture["publishers"][1]["id"] == publishers3.publishers[1].id

    publishers4 = marketplace_client.list_manage_publishers(
        billing_account_id=db_fixture["publishers"][0]["billing_account_id"],
        filter_query="categoryId='{}'".format(
            db_fixture["category"][2]["id"]))
    assert len(publishers4.publishers) == 1
    assert db_fixture["publishers"][0]["id"] == publishers4.publishers[0].id

    publishers5 = marketplace_client.list_manage_publishers(
        billing_account_id=db_fixture["publishers"][0]["billing_account_id"],
        filter_query="categoryId='{}'".format(
            db_fixture["category"][0]["id"]))
    assert len(publishers5.publishers) == 0
