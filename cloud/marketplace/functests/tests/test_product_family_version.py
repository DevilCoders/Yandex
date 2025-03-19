import pytest

from cloud.marketplace.common.yc_marketplace_common.client import MarketplaceClient
from cloud.marketplace.common.yc_marketplace_common.client import MarketplacePrivateClient
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family_version import OsProductFamilyVersion
from cloud.marketplace.functests.yc_marketplace_functests.utils import BA_ID
from cloud.marketplace.functests.yc_marketplace_functests.utils import _create_product_family
from yc_common.exceptions import ApiError


def test_product_family_version_create_with_sku(marketplace_client: MarketplaceClient, generate_id,
                                                marketplace_private_client: MarketplacePrivateClient, db_fixture):
    skus = [
        {
            "id": generate_id(),
            "checkFormula": "some formula",
        },
    ]
    family = _create_product_family(marketplace_client,
                                    generate_id,
                                    "product-family",
                                    db_fixture["os_product"][0]["id"],
                                    skus,
                                    ).metadata
    filter_query = "os_product_family_id='{}'".format(family.os_product_family_id)
    list_result = marketplace_client.list_os_product_family_version(BA_ID,
                                                                    filter_query=filter_query)
    for v in list_result.os_product_family_versions:
        assert all(s.id == skus[0]["id"] for s in v.skus)
        assert all(s.check_formula == skus[0]["checkFormula"] for s in v.skus)
        assert len(v.skus) == len(skus)


def test_product_family_version_list(marketplace_client: MarketplaceClient, generate_id,
                                     db_fixture):
    filter_query = "os_product_family_id='{}'".format(db_fixture["os_product_family"][0]["id"])
    list_result = marketplace_client.list_os_product_family_version(BA_ID,
                                                                    filter_query=filter_query)
    assert len(list_result.os_product_family_versions) == 1
    assert list_result.os_product_family_versions[0].id == db_fixture["os_product_family_version"][0]["id"]

    assert all(v.skus == [] for v in list_result.os_product_family_versions)


def test_product_family_version_publish(marketplace_client: MarketplaceClient, generate_id,
                                        marketplace_private_client: MarketplacePrivateClient,
                                        db_fixture):
    list_result = marketplace_client.list_os_product_family_version(
        BA_ID,
        filter_query="os_product_family_id='{}'".format(db_fixture["os_product_family"][0]["id"]),
    )
    assert len(list_result.os_product_family_versions) == 2

    # Assert can not publish from wrong state
    with pytest.raises(ApiError):
        marketplace_private_client.publish_product_version(db_fixture["os_product_family_version"][0]["id"])

    # Assert publish ok from REVIEW status
    pub_op = marketplace_private_client.publish_product_version(db_fixture["os_product_family_version"][1]["id"])
    assert pub_op.metadata.os_product_family_id == db_fixture["os_product_family"][0]["id"]


def test_version_get(marketplace_client: MarketplaceClient, generate_id,
                     db_fixture):
    active_version = marketplace_client.get_os_product_family_version(
        db_fixture["os_product_family_version"][0]["id"],
    )
    assert active_version.status == OsProductFamilyVersion.Status.ACTIVE
    assert active_version.resource_spec.billing_account_requirements.usage_status[0] == "paid"

    deprecated_version = marketplace_client.get_os_product_family_version(
        db_fixture["os_product_family_version"][1]["id"],
    )
    assert deprecated_version.status == OsProductFamilyVersion.Status.DEPRECATED

    with pytest.raises(ApiError):
        marketplace_client.get_os_product_family_version(
            db_fixture["os_product_family_version"][2]["id"],
        )
