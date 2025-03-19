import os

import pytest

from yc_common.exceptions import ApiError
from cloud.marketplace.common.yc_marketplace_common.client import MarketplaceClient
from cloud.marketplace.common.yc_marketplace_common.models.isv import Isv

TEST_DATA = {
    "isv": None,
    "isv_name": "Test isv",
    "isv_name_2": "Test 2 isv",
    "isv_contacts": {
        "uri": "Test uri",
    },
}
script_dir = os.path.dirname(__file__)
test_image = "test_data/ubuntu.png"


def test_isv_create(marketplace_client: MarketplaceClient, generate_id, default_isv_category):
    TEST_DATA["isv_logo_id"] = generate_id()
    create_operation = marketplace_client.create_isv(
        name=TEST_DATA["isv_name"],
        logo_id=TEST_DATA["isv_logo_id"],
        contact_info=TEST_DATA["isv_contacts"],
        billing_account_id=generate_id(),
        meta={
            "test": {"ru": "ru text"},
        },
    )

    assert create_operation.done is False
    assert create_operation.response is None

    isv = marketplace_client.get_manage_isv(create_operation.metadata.isv_id)

    assert isv.meta.get("test") == "ru text"
    assert isv.status == Isv.Status.PENDING
    assert isv.name == TEST_DATA["isv_name"]
    assert isv.contact_info is not None and isv.contact_info.get("uri") == TEST_DATA["isv_contacts"][
        "uri"
    ]

    assert len(isv.categories) == 1
    assert isv.categories[0] == default_isv_category


def test_isv_create_duplication(marketplace_client: MarketplaceClient, generate_id, db_fixture):
    TEST_DATA["isv_logo_id"] = generate_id()
    with pytest.raises(ApiError) as api_err:
        marketplace_client.create_isv(
            name=TEST_DATA["isv_name"],
            logo_id=TEST_DATA["isv_logo_id"],
            contact_info=TEST_DATA["isv_contacts"],
            billing_account_id=db_fixture["isv"][0]["billing_account_id"],
            meta={
                "test": {"ru": "ru text"},
            },
        )

    assert "Isv attributes conflict" in str(api_err.value)


def test_isv_get(marketplace_client, db_fixture):
    print(db_fixture)
    isv_id = db_fixture["isv"][0]["id"]
    isv = marketplace_client.get_isv(isv_id)
    assert set(isv.categories) == {
        ordering["category_id"] for ordering in db_fixture["ordering"] if ordering["resource_id"] == isv_id}

    isv2_id = db_fixture["isv"][1]["id"]
    with pytest.raises(ApiError) as api_err:
        marketplace_client.get_isv(isv2_id)
    assert "Invalid isv ID." in str(api_err.value)

    isv2 = marketplace_client.get_manage_isv(isv2_id)
    assert set(isv2.categories) == {
        ordering["category_id"] for ordering in db_fixture["ordering"] if ordering["resource_id"] == isv2_id}


def test_isv_listing(marketplace_client, db_fixture, default_isv_category):
    isvs = marketplace_client.list_isvs()
    assert len(isvs.isvs) == 1
    assert set(isvs.isvs[0].categories) == {
        db_fixture["category"][1]["id"],
        db_fixture["category"][2]["id"],
        default_isv_category,
    }
    isvs2 = marketplace_client.list_manage_isvs(billing_account_id=db_fixture["isv"][0]["billing_account_id"])
    assert len(isvs2.isvs) == 2

    assert db_fixture["isv"][0]["id"] == isvs2.isvs[1].id
    assert db_fixture["isv"][1]["id"] == isvs2.isvs[0].id

    isvs3 = marketplace_client.list_manage_isvs(billing_account_id=db_fixture["isv"][0]["billing_account_id"],
                                                filter_query="categoryId='{}'".format(db_fixture["category"][1]["id"]))
    assert len(isvs3.isvs) == 2
    assert db_fixture["isv"][0]["id"] == isvs3.isvs[0].id
    assert db_fixture["isv"][1]["id"] == isvs3.isvs[1].id

    isvs4 = marketplace_client.list_manage_isvs(billing_account_id=db_fixture["isv"][0]["billing_account_id"],
                                                filter_query="categoryId='{}'".format(
                                                    db_fixture["category"][2]["id"]))
    assert len(isvs4.isvs) == 1
    assert db_fixture["isv"][0]["id"] == isvs4.isvs[0].id

    isvs5 = marketplace_client.list_manage_isvs(billing_account_id=db_fixture["isv"][0]["billing_account_id"],
                                                filter_query="categoryId='{}'".format(
                                                    db_fixture["category"][0]["id"]))
    assert len(isvs5.isvs) == 0
