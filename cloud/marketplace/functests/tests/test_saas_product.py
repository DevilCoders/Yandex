import os

import pytest
from yc_common.exceptions import ApiError
from yc_common.misc import timestamp
from yc_common.validation import ResourceIdType

from cloud.marketplace.common.yc_marketplace_common.client import MarketplaceClient
from cloud.marketplace.common.yc_marketplace_common.client import MarketplacePrivateClient
from cloud.marketplace.common.yc_marketplace_common.models.category import Category
from cloud.marketplace.common.yc_marketplace_common.models.product_type import ProductType
from cloud.marketplace.common.yc_marketplace_common.models.saas_product import SaasProduct
from cloud.marketplace.functests.yc_marketplace_functests.utils import BA_ID
from cloud.marketplace.functests.yc_marketplace_functests.utils import PRODUCT_ID
from cloud.marketplace.functests.yc_marketplace_functests.utils import _create_saas_product


def _create_saas_product_id(*args, **kwargs) -> ResourceIdType:
    return _create_saas_product(*args, **kwargs).metadata.saas_product_id


def test_get_product_error(marketplace_client: MarketplaceClient):
    with pytest.raises(ApiError) as api_err:
        marketplace_client.get_saas_product(PRODUCT_ID)
    assert "Invalid SaaS product ID." in str(api_err.value)


def test_product_create(marketplace_client: MarketplaceClient):
    name = "product-{}".format(timestamp())
    create_operation = _create_saas_product(marketplace_client, name)
    assert create_operation.done is False


def test_get_non_public_product(marketplace_client: MarketplaceClient, gauthling_context):
    name = "product-{}".format(timestamp())
    create_operation = _create_saas_product(marketplace_client, name)
    #  unauthorized user should not get non-public product
    with pytest.raises(ApiError), gauthling_context(auth=True, authz=False):
        marketplace_client.get_saas_product(create_operation.metadata.saas_product_id)
    #  non-public product should not be accessible via public get endpoint
    with pytest.raises(ApiError):
        marketplace_client.get_public_saas_product(create_operation.metadata.saas_product_id)


def test_get_product_slug(marketplace_client: MarketplaceClient,
                          marketplace_private_client: MarketplacePrivateClient,
                          generate_id):
    name = "public-product-{}".format(timestamp())
    vendor = "Мазай"
    public_product = _create_saas_product(marketplace_client, name, vendor=vendor, meta={}, slug=name)
    product_id = public_product.metadata.saas_product_id
    marketplace_private_client.publish_saas_product(product_id)
    pp = marketplace_client.get_public_saas_product(product_id)
    assert pp.status in SaasProduct.Status.PUBLIC
    assert pp.vendor == vendor
    assert name == pp.slugs[0].slug
    pp2 = marketplace_client.get_public_saas_product(name)
    assert pp2.id == pp.id
    marketplace_private_client.add_slug("test_slug2", pp2.id, product_type=ProductType.SAAS.upper())
    pp3 = marketplace_client.get_public_saas_product("test_slug2")
    assert pp3.id == pp.id
    assert len(pp3.slugs) == 2
    assert {pp3.slugs[0].slug, pp3.slugs[1].slug} == {name, "test_slug2"}
    marketplace_private_client.del_slug("test_slug2")
    pp4 = marketplace_client.get_public_saas_product(name)
    assert pp4.id == pp.id
    assert name == pp4.slugs[0].slug
    assert len(pp4.slugs) == 1


def test_get_product_slug_update(marketplace_client: MarketplaceClient,
                                 marketplace_private_client: MarketplacePrivateClient,
                                 generate_id):
    name = "public-product-{}".format(timestamp())
    vendor = "Мазай"
    public_product = _create_saas_product(marketplace_client, name, vendor=vendor, meta={}, slug=name)
    product_id = public_product.metadata.saas_product_id
    marketplace_private_client.publish_saas_product(product_id)
    pp = marketplace_client.get_public_saas_product(product_id)
    assert pp.status in SaasProduct.Status.PUBLIC
    assert pp.vendor == vendor
    assert name == pp.slugs[0].slug
    marketplace_client.update_saas_product(
        product_id=pp.id,
        slug=name,
    )


def test_get_product_sku_update(marketplace_client: MarketplaceClient,
                                marketplace_private_client: MarketplacePrivateClient,
                                generate_id):
    name = "public-product-{}".format(timestamp())
    vendor = "Мазай"
    public_product = _create_saas_product(marketplace_client, name, vendor=vendor, meta={}, slug=name)
    product_id = public_product.metadata.saas_product_id
    marketplace_private_client.publish_saas_product(product_id)
    pp = marketplace_client.get_public_saas_product(product_id)
    assert pp.status in SaasProduct.Status.PUBLIC
    assert pp.vendor == vendor
    assert name == pp.slugs[0].slug
    sku_id = generate_id()
    marketplace_client.update_saas_product(
        product_id=pp.id,
        sku_ids=[sku_id],
    )
    pp = marketplace_client.get_public_saas_product(product_id)
    assert pp.sku_ids[0] == sku_id

    new_sku_id = generate_id()
    marketplace_client.update_saas_product(
        product_id=pp.id,
        sku_ids=[new_sku_id],
    )
    pp = marketplace_client.get_public_saas_product(product_id)
    assert pp.sku_ids[0] == new_sku_id


def test_get_public_product(marketplace_client: MarketplaceClient,
                            marketplace_private_client: MarketplacePrivateClient,
                            generate_id,
                            gauthling_context):
    name = "public-product-{}".format(timestamp())
    vendor = "Мазай"
    meta_key = "asd"
    meta_value = "фывапролджэ"
    public_product = _create_saas_product(marketplace_client, name, vendor=vendor, meta={
        meta_key: {
            "en": meta_value,
            "ru": meta_value,
        },
    })
    product_id = public_product.metadata.saas_product_id
    marketplace_private_client.publish_saas_product(product_id)
    #  unauthorized user should not get product via `manage` endpoint
    with pytest.raises(ApiError), gauthling_context(auth=True, authz=False):
        marketplace_client.get_saas_product(product_id)
    #
    #  public product should be accessible via public get endpoint
    pp = marketplace_client.get_public_saas_product(product_id)
    assert pp.status in SaasProduct.Status.PUBLIC
    assert pp.vendor == vendor
    assert pp.meta.get(meta_key) == meta_value


def test_own_product_list(marketplace_client: MarketplaceClient, generate_id,
                          marketplace_private_client: MarketplacePrivateClient):
    category = marketplace_private_client.create_category({"en": "test cat {}".format(timestamp())},
                                                          Category.Type.PUBLIC,
                                                          0).response
    first_product_id = _create_saas_product_id(marketplace_client, "product-product", category_ids=[category.id])
    marketplace_private_client.publish_saas_product(first_product_id)
    second_product_id = _create_saas_product_id(marketplace_client, "product-product", category_ids=[category.id])
    filter_query = "categoryId='{}'".format(category.id)
    saas_products = marketplace_client.list_saas_products(BA_ID, filter_query=filter_query).products
    assert len(saas_products) == 2
    assert sorted(x.id for x in saas_products) == sorted([first_product_id, second_product_id])
    assert [x.status for x in saas_products if x.status == SaasProduct.Status.ACTIVE]
    assert [x.status for x in saas_products if x.status == SaasProduct.Status.CREATING]


def test_public_product_list(marketplace_client: MarketplaceClient, generate_id,
                             marketplace_private_client: MarketplacePrivateClient):
    category = marketplace_private_client.create_category({"en": "test cat {}".format(timestamp())},
                                                          Category.Type.PUBLIC,
                                                          0).response
    first_product_id = _create_saas_product_id(marketplace_client, "product-product", category_ids=[category.id])
    _create_saas_product_id(marketplace_client, "product-product", category_ids=[category.id])
    marketplace_private_client.publish_saas_product(first_product_id)
    list_result = marketplace_client.list_public_saas_products(filter_query="categoryId='{}'".format(category.id))
    assert len(list_result.products) == 1
    assert list_result.products[0].id == first_product_id
    assert list_result.products[0].status == SaasProduct.Status.ACTIVE


def test_product_update(marketplace_client: MarketplaceClient, generate_id,
                        marketplace_private_client: MarketplacePrivateClient):
    category = marketplace_private_client.create_category({"en": "test cat {}".format(timestamp())},
                                                          Category.Type.PUBLIC,
                                                          0).response
    _create_saas_product_id(marketplace_client, "product-product", category_ids=[category.id])
    list_result = marketplace_client.list_saas_products(BA_ID)
    first_product = list_result.products[0]
    new_name = "updated-product-{}".format(timestamp())
    new_description = "updated description {}".format(timestamp())
    new_short_description = "updated short desc {}".format(timestamp())
    update_operation = marketplace_client.update_saas_product(first_product.id, new_name,
                                                              short_description=new_short_description,
                                                              description=new_description)
    assert update_operation.metadata.saas_product_id == first_product.id
    assert update_operation.done is False


def test_product_listing_by_non_unique_field(marketplace_client: MarketplaceClient, generate_id,
                                             marketplace_private_client: MarketplacePrivateClient):
    # Arrange
    category = marketplace_private_client.create_category({"en": "non uniq {}".format(timestamp())},
                                                          Category.Type.PUBLIC,
                                                          0).response
    ids = []
    # create
    n = 10
    for i in range(n):
        k = 1 if n / 2 > i else 2
        name = "non unique name {}".format(k)
        create_op = _create_saas_product(marketplace_client, name, [category.id],
                                         slug="test_product_listing_by_non_unique_field_{}".format(i))
        pid = create_op.metadata.saas_product_id
        marketplace_private_client.publish_saas_product(pid)
        ids.append((name, pid))
    # Act
    query = "categoryId='{}'".format(category.id)
    products = marketplace_client.list_public_saas_products(3, filter_query=query, order_by="name")
    listed_ids = [p.id for p in products.products]
    next_page_token = products.next_page_token
    while next_page_token:
        more_products = marketplace_client.list_public_saas_products(3,
                                                                     page_token=next_page_token,
                                                                     filter_query=query,
                                                                     order_by="name")
        listed_ids += [p.id for p in more_products.products]
        next_page_token = more_products.next_page_token
    # Assert
    # as half items in category have same value to be ordered by, inside they will be sorted by id
    assert [i[1] for i in sorted(ids)] == listed_ids


def test_put_product_on_place_in_category(marketplace_client: MarketplaceClient, generate_id,
                                          marketplace_private_client: MarketplacePrivateClient):
    # Arrange
    category = marketplace_private_client.create_category(
        {"en": "category {}".format(timestamp())},
        Category.Type.PUBLIC,
        0,
    ).response
    ids = {}
    # create
    n = 10
    for i in range(n):
        c = chr(ord("A") + i)
        name = "Product {}".format(c)
        create_op = _create_saas_product(marketplace_client, name, [category.id])
        pid = create_op.metadata.saas_product_id
        marketplace_private_client.publish_saas_product(pid)
        ids[c] = pid
    # Act
    marketplace_private_client.put_saas_product_on_place(ids["A"], category.id, 2)
    marketplace_private_client.put_saas_product_on_place(ids["C"], category.id, 2)
    marketplace_private_client.put_saas_product_on_place(ids["F"], category.id, 2)
    marketplace_private_client.put_saas_product_on_place(ids["H"], category.id, 2)
    query = "categoryId='{}'".format(category.id)
    public_products = marketplace_client.list_public_saas_products(10, filter_query=query).products
    listed_ids = [p.id for p in public_products]
    # Assert
    # as half items in category have same value to be ordered by, inside they will be sorted by id
    assert listed_ids == [ids["B"], ids["A"], ids["H"], ids["F"], ids["C"], ids["D"], ids["E"], ids["G"], ids["I"],
                          ids["J"]]


def test_product_update_does_not_affect_ordering(marketplace_client: MarketplaceClient, generate_id,
                                                 marketplace_private_client: MarketplacePrivateClient,
                                                 default_saas_product_category):
    category_a = marketplace_private_client.create_category({"en": "category A {}".format(timestamp())},
                                                            Category.Type.PUBLIC,
                                                            0).response
    category_b = marketplace_private_client.create_category({"en": "category B {}".format(timestamp())},
                                                            Category.Type.PUBLIC,
                                                            0).response
    ids = {}
    # create
    n = 5
    for i in range(n):
        c = chr(ord("A") + i)
        name = "Product {}".format(c)
        create_op = _create_saas_product(marketplace_client, name, [category_a.id, category_b.id])
        pid = create_op.metadata.saas_product_id
        marketplace_private_client.publish_saas_product(pid)
        ids[c] = pid
    marketplace_private_client.put_saas_product_on_place(ids["D"], category_a.id, 0)
    new_d_name = "Updated D"
    marketplace_client.update_saas_product(ids["D"], new_d_name, category_ids=[category_a.id])
    updated_d = marketplace_client.get_saas_product(ids["D"])
    assert updated_d.name == new_d_name
    assert sorted(updated_d.category_ids) == sorted([category_a.id, default_saas_product_category])
    query = "categoryId='{}'".format(category_a.id)
    category_a_list = marketplace_client.list_public_saas_products(filter_query=query)
    assert category_a_list.products[0].id == ids["D"]


script_dir = os.path.dirname(__file__)
test_image = "test_data/ubuntu.png"
