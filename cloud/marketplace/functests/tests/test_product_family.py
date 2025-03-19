import pytest

from cloud.marketplace.common.yc_marketplace_common.client import MarketplaceClient
from cloud.marketplace.common.yc_marketplace_common.client import MarketplacePrivateClient
from cloud.marketplace.common.yc_marketplace_common.models.deprecation import Deprecation
from cloud.marketplace.common.yc_marketplace_common.models.os_product import OsProduct
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family import OsProductFamily
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family_version import OsProductFamilyVersion
from cloud.marketplace.common.yc_marketplace_common.models.resource_spec import ResourceSpec
from cloud.marketplace.functests.yc_marketplace_functests.utils import BA_ID
from cloud.marketplace.functests.yc_marketplace_functests.utils import FAMILY_ID
from cloud.marketplace.functests.yc_marketplace_functests.utils import GB
from cloud.marketplace.functests.yc_marketplace_functests.utils import MB
from cloud.marketplace.functests.yc_marketplace_functests.utils import _create_product
from yc_common.exceptions import ApiError
from yc_common.misc import timestamp


def _create_product_family(client: MarketplaceClient, generate_id, name,
                           logo_id=None,
                           related_products=None,
                           license_rules=None):
    create_op = _create_product(client, name="product-{}".format(timestamp()))
    pid = create_op.metadata.os_product_id
    return client.create_os_product_family(
        billing_account_id=BA_ID,
        name=name,
        image_id=generate_id(),
        pricing_options=OsProductFamily.PricingOptions.FREE,
        resource_spec=ResourceSpec({
            "memory": 4 * MB,
            "cores": 4,
            "disk_size": 30 * GB,
            "user_data_form_id": "linux",
            "service_account_roles": ["editor"],
            "billing_account_requirements": {
                "usage_status": ["paid"],
            }
        }),
        os_product_id=pid,
        logo_id=logo_id,
        meta={
            "test": {
                "en": "en_text",
                "ru": "ru_text",
            },
        },
        form_id=generate_id(),
        related_products=related_products,
        license_rules=license_rules
    )


def _update_product_family(client: MarketplaceClient, id, generate_id,
                           logo_id=None,
                           related_products=None):
    return client.update_os_product_family(
        id,
        image_id=generate_id(),
        pricing_options=OsProductFamily.PricingOptions.FREE,
        resource_spec=ResourceSpec({
            "memory": 4 * MB,
            "cores": 4,
            "disk_size": 30 * GB,
            "user_data_form_id": "linux",
        }),
        logo_id=logo_id,
        related_products=related_products,
    )


def test_get_product_family_error(marketplace_client: MarketplaceClient):
    with pytest.raises(ApiError) as api_err:
        marketplace_client.get_os_product_family(FAMILY_ID)

    assert "Invalid marketplace product family ID." in str(api_err.value)


def test_product_family_create(marketplace_client: MarketplaceClient, generate_id):
    create_operation = _create_product_family(marketplace_client, generate_id, "product-family")

    assert create_operation.done is False


def test_product_family_related_saas_product(marketplace_client: MarketplaceClient, generate_id, db_fixture):
    saas_product_id = db_fixture["saas_product"][0]["id"]

    create_operation = _create_product_family(marketplace_client, generate_id, "product-family",
                                              related_products=[{
                                                  "product_id": saas_product_id,
                                                  "product_type": "saas",
                                              }])

    assert create_operation.done is False


def test_product_family_update(marketplace_client: MarketplaceClient, generate_id):  # todo check content of operations
    create_operation = _create_product_family(marketplace_client, generate_id, "product-family")
    assert create_operation.done is False

    update_operation = _update_product_family(marketplace_client, create_operation.metadata.os_product_family_id,
                                              generate_id)
    assert update_operation.done is False


def test_product_family_related_saas_product_update(marketplace_client: MarketplaceClient, generate_id, db_fixture):
    saas_product_id = db_fixture["saas_product"][0]["id"]
    new_saas_product_id = db_fixture["saas_product"][1]["id"]

    create_operation = _create_product_family(marketplace_client, generate_id, "product-family",
                                              related_products=[{
                                                  "product_id": saas_product_id,
                                                  "product_type": "saas",
                                              }])

    _update_product_family(marketplace_client,
                           create_operation.metadata.os_product_family_id,
                           generate_id,
                           related_products=[{
                               "product_id": new_saas_product_id,
                               "product_type": "saas",
                           }])


def test_product_family_related_saas_product_update_error(marketplace_client: MarketplaceClient, generate_id,
                                                          db_fixture):
    saas_product_id = db_fixture["saas_product"][0]["id"]
    new_saas_product_id = db_fixture["saas_product"][2]["id"]

    create_operation = _create_product_family(marketplace_client, generate_id, "product-family",
                                              related_products=[{
                                                  "product_id": saas_product_id,
                                                  "product_type": "saas",
                                              }])
    with pytest.raises(ApiError):
        _update_product_family(marketplace_client,
                               create_operation.metadata.os_product_family_id,
                               generate_id,
                               related_products=[{
                                   "product_id": new_saas_product_id,
                                   "product_type": "saas",
                               }])


def test_product_family_update_inherit_logo(marketplace_client: MarketplaceClient, generate_id, db_fixture):
    family_id = db_fixture["os_product_family"][0]["id"]
    update_operation = _update_product_family(marketplace_client, family_id,
                                              generate_id)
    print(update_operation)

    assert update_operation.done is False


def test_product_family_update_desc_only(marketplace_client: MarketplaceClient,
                                         generate_id):  # todo check content of operations
    create_operation = _create_product_family(marketplace_client, generate_id, "product-family")
    assert create_operation.done is False

    update_operation = marketplace_client.update_os_product_family(
        create_operation.metadata.os_product_family_id,
        description="New description",
    )
    assert update_operation.done is False


def test_create_licence_rules(marketplace_client: MarketplaceClient, db_fixture,
                              generate_id):  # todo check content of operations
    create_operation = _create_product_family(marketplace_client, generate_id, "product-family", license_rules=[{
        "category": "blacklist",
        "entity": "billing_account",
        "path": "usage_status",
        "expected": ["paid"]
    }])
    assert create_operation.done is False


def test_update_licence_rules(marketplace_client: MarketplaceClient, db_fixture,
                              generate_id):  # todo check content of operations
    family_id = db_fixture["os_product_family"][0]["id"]

    update_operation = marketplace_client.update_os_product_family(
        family_id,
        description="New description",
        license_rules=[
            {
                "category": "whitelist",
                "entity": "billing_account",
                "path": "usage_status",
                "expected": ["paid"]
            }
        ]
    )
    assert update_operation.done is False


def test_product_family_deprecate(marketplace_client: MarketplaceClient,
                                  marketplace_private_client: MarketplacePrivateClient,
                                  generate_id):
    # Create product with 2 families, deprecate first family, check if family status is
    # deprecated. Check if product is still active and his primary family id changed.
    # Then deprecate second family and check if product becomes deprecated

    create_operation = _create_product_family(marketplace_client, generate_id, "product-family")
    first_family_id = create_operation.metadata.os_product_family_id
    list_result = marketplace_client.list_os_product_family_version(BA_ID,
                                                                    filter_query="osProductFamilyId='{}'".format(
                                                                        first_family_id))

    marketplace_private_client.set_status_product_family_version(list_result.os_product_family_versions[0].id,
                                                                 OsProductFamilyVersion.Status.ACTIVE)

    first_family = marketplace_client.get_os_product_family(first_family_id)
    product = marketplace_client.get_os_product(first_family.os_product_id)

    assert product.status == OsProduct.Status.ACTIVE
    assert first_family.status == OsProductFamily.Status.ACTIVE

    create_operation = marketplace_client.create_os_product_family(billing_account_id=BA_ID,
                                                                   name="nextdeprecated",
                                                                   image_id=generate_id(),
                                                                   pricing_options=OsProductFamily.PricingOptions.FREE,
                                                                   resource_spec=ResourceSpec({
                                                                       "memory": 4 * MB,
                                                                       "cores": 4,
                                                                       "disk_size": 30 * GB,
                                                                       "user_data_form_id": "linux",
                                                                   }),
                                                                   os_product_id=product.id,
                                                                   form_id=generate_id())

    second_family_id = create_operation.metadata.os_product_family_id

    list_result = marketplace_client.list_os_product_family_version(BA_ID,
                                                                    filter_query="osProductFamilyId='{}'".format(
                                                                        second_family_id))

    marketplace_private_client.set_status_product_family_version(list_result.os_product_family_versions[0].id,
                                                                 OsProductFamilyVersion.Status.ACTIVE)

    second_family = marketplace_client.get_os_product_family(second_family_id)
    assert product.primary_family_id == first_family_id
    assert second_family.status == OsProductFamily.Status.ACTIVE

    marketplace_client.deprecate_os_product_family(first_family_id, status="DEPRECATED")
    marketplace_client.deprecate_os_product_family(first_family_id, status="OBSOLETE")

    family = marketplace_client.get_os_product_family(first_family_id)
    product = marketplace_client.get_os_product(first_family.os_product_id)

    assert family.status == OsProductFamily.Status.DEPRECATED
    assert product.primary_family_id == second_family_id

    marketplace_client.deprecate_os_product_family(second_family_id, status="DEPRECATED")
    marketplace_client.deprecate_os_product_family(second_family_id, status="OBSOLETE")

    family = marketplace_client.get_os_product_family(second_family_id)
    product = marketplace_client.get_os_product(family.os_product_id)

    assert family.status == OsProductFamily.Status.DEPRECATED
    assert product.status == OsProduct.Status.DEPRECATED


def test_own_product_family_list(marketplace_client: MarketplaceClient, generate_id):
    _create_product_family(marketplace_client, generate_id, "product-family")

    list_result = marketplace_client.list_os_product_families(BA_ID)
    assert len(list_result.os_product_families) > 0


def test_public_product_family_list(marketplace_client: MarketplaceClient,
                                    marketplace_private_client: MarketplacePrivateClient,
                                    generate_id):
    family_op = _create_product_family(marketplace_client, generate_id, "product-family")
    list_result = marketplace_client.list_os_product_family_version(
        BA_ID,
        filter_query="osProductFamilyId='{}'".format(family_op.metadata.os_product_family_id))
    for v in list_result.os_product_family_versions:
        marketplace_private_client.set_image_id_product_family_version(v.id, generate_id())
        marketplace_private_client.set_status_product_family_version(v.id, OsProductFamilyVersion.Status.REVIEW)
        marketplace_private_client.set_status_product_family_version(v.id, OsProductFamilyVersion.Status.ACTIVATING)
        marketplace_private_client.set_status_product_family_version(v.id, OsProductFamilyVersion.Status.ACTIVE)

    families = marketplace_client.list_public_os_product_families().os_product_families

    assert all(f.status in OsProductFamily.Status.PUBLIC for f in families)


def test_product_family_list_filtered(marketplace_client: MarketplaceClient, generate_id):
    _create_product_family(marketplace_client, generate_id, "product-family-filter'")
    query = r"createdAt > {} and name in ('product-family-filter\'', 'product-family')".format(timestamp() - 60)
    list_result = marketplace_client.list_os_product_families(BA_ID, filter_query=query)

    assert len(list_result.os_product_families) > 0


def test_deprecation_flow(marketplace_client: MarketplaceClient, generate_id):
    create_operation = _create_product_family(marketplace_client, generate_id, "product-family-{}".format(timestamp()))
    family_id = create_operation.metadata.os_product_family_id
    status = Deprecation.Status

    test_flow = (
        # First stage
        (status.DELETED, False),
        (status.OBSOLETE, False),
        (status.DEPRECATED, True),

        # Second stage
        (status.DEPRECATED, True),
        (status.DELETED, False),
        (status.DEPRECATED, True),
        (status.OBSOLETE, True),

        # Third stage
        (status.DEPRECATED, False),
        (status.OBSOLETE, True),
        (status.DELETED, True),

        # Fourth stage
        (status.DELETED, True),
        (status.DEPRECATED, False),
        (status.OBSOLETE, False),
        (status.DELETED, True),
    )

    for trying in test_flow:
        if not trying[1]:
            with pytest.raises(ApiError):
                marketplace_client.deprecate_os_product_family(family_id, status=trying[0])
        else:
            marketplace_client.deprecate_os_product_family(family_id, status=trying[0])
