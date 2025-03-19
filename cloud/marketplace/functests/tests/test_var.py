import pytest

from yc_common.exceptions import ApiError
from cloud.marketplace.common.yc_marketplace_common.client import MarketplaceClient
from cloud.marketplace.common.yc_marketplace_common.models.var import Var

TEST_DATA = {
    "var": None,
    "var_name": "Test var",
    "var_name_2": "Test 2 var",
    "var_contacts": {
        "uri": "Test uri",
    },
}


def test_var_create(marketplace_client: MarketplaceClient, generate_id, default_var_category):
    TEST_DATA["var_logo_id"] = generate_id()
    create_operation = marketplace_client.create_var(
        name=TEST_DATA["var_name"],
        logo_id=TEST_DATA["var_logo_id"],
        contact_info=TEST_DATA["var_contacts"],
        billing_account_id=generate_id(),
        meta={
            "test": {"ru": "ru text"},
        },
    )

    assert create_operation.done is False
    assert create_operation.response is None

    var = marketplace_client.get_manage_var(create_operation.metadata.var_id)

    assert var.status == Var.Status.PENDING
    assert var.name == TEST_DATA["var_name"]
    assert var.meta.get("test") == "ru text"
    assert var.contact_info is not None and var.contact_info.get("uri") == TEST_DATA["var_contacts"][
        "uri"
    ]

    assert len(var.categories) == 1
    assert var.categories[0] == default_var_category


def test_var_create_duplication(marketplace_client: MarketplaceClient, generate_id, db_fixture):
    TEST_DATA["var_logo_id"] = generate_id()
    with pytest.raises(ApiError) as api_err:
        marketplace_client.create_var(
            name=TEST_DATA["var_name"],
            logo_id=TEST_DATA["var_logo_id"],
            contact_info=TEST_DATA["var_contacts"],
            billing_account_id=db_fixture["var"][0]["billing_account_id"],
            meta={
                "test": {"ru": "ru text"},
            },
        )

    assert "Var attributes conflict" in str(api_err.value)


def test_var_get(marketplace_client, db_fixture):
    var_id = db_fixture["var"][0]["id"]
    var = marketplace_client.get_var(var_id)
    assert set(var.categories) == {
        ordering["category_id"] for ordering in db_fixture["ordering"] if ordering["resource_id"] == var_id}

    var2_id = db_fixture["var"][1]["id"]
    with pytest.raises(ApiError) as api_err:
        marketplace_client.get_var(var2_id)
    assert "Invalid var ID." in str(api_err.value)

    var2 = marketplace_client.get_manage_var(var2_id)
    assert set(var2.categories) == {
        ordering["category_id"] for ordering in db_fixture["ordering"] if ordering["resource_id"] == var2_id}


def test_var_listing(marketplace_client, db_fixture, default_var_category):
    vars = marketplace_client.list_vars()
    assert len(vars.vars) == 1
    assert set(vars.vars[0].categories) == {
        db_fixture["category"][1]["id"],
        db_fixture["category"][2]["id"],
        default_var_category,
    }
    vars2 = marketplace_client.list_manage_vars(billing_account_id=db_fixture["var"][0]["billing_account_id"])
    assert len(vars2.vars) == 2

    assert db_fixture["var"][0]["id"] == vars2.vars[1].id
    assert db_fixture["var"][1]["id"] == vars2.vars[0].id

    vars3 = marketplace_client.list_manage_vars(billing_account_id=db_fixture["var"][0]["billing_account_id"],
                                                filter_query="categoryId='{}'".format(db_fixture["category"][1]["id"]))
    assert len(vars3.vars) == 2
    assert db_fixture["var"][0]["id"] == vars3.vars[0].id
    assert db_fixture["var"][1]["id"] == vars3.vars[1].id

    vars4 = marketplace_client.list_manage_vars(billing_account_id=db_fixture["var"][0]["billing_account_id"],
                                                filter_query="categoryId='{}'".format(
                                                    db_fixture["category"][2]["id"]))
    assert len(vars4.vars) == 1
    assert db_fixture["var"][0]["id"] == vars4.vars[0].id

    vars5 = marketplace_client.list_manage_vars(billing_account_id=db_fixture["var"][0]["billing_account_id"],
                                                filter_query="categoryId='{}'".format(
                                                    db_fixture["category"][0]["id"]))
    assert len(vars5.vars) == 0
