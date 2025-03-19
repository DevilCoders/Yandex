import os
from typing import Callable

import pytest
from natsort import natsorted
from cloud.marketplace.common.yc_marketplace_common.client import MarketplaceClient
from cloud.marketplace.common.yc_marketplace_common.client import MarketplacePrivateClient
from cloud.marketplace.common.yc_marketplace_common.models.category import Category
from cloud.marketplace.common.yc_marketplace_common.models.os_product import OsProduct
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family import OsProductFamilyResponse
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family_version import OsProductFamilyVersion
from cloud.marketplace.common.yc_marketplace_common.models.product_type import ProductType
from cloud.marketplace.functests.yc_marketplace_functests.utils import BA_ID
from cloud.marketplace.functests.yc_marketplace_functests.utils import PRODUCT_ID
from cloud.marketplace.functests.yc_marketplace_functests.utils import _create_product
from cloud.marketplace.functests.yc_marketplace_functests.utils import _create_product_family
from yc_common.exceptions import ApiError
from yc_common.misc import timestamp
from yc_common.validation import ResourceIdType


def _translate_id(obj, field):
    return "mkt_i18n://%s.{id}.meta.%s" % (obj, field)


def _create_product_id(*args, **kwargs) -> ResourceIdType:
    return _create_product(*args, **kwargs).metadata.os_product_id


def _create_family_for_product(marketplace_client: MarketplaceClient,
                               marketplace_private_client: MarketplacePrivateClient,
                               generate_id: Callable[[], ResourceIdType], name: str, os_product_id: ResourceIdType,
                               active: bool, meta=None) -> OsProductFamilyResponse:
    family_op = _create_product_family(marketplace_client,
                                       generate_id,
                                       name,
                                       os_product_id,
                                       meta=meta,
                                       )
    list_result = marketplace_client.list_os_product_family_version(BA_ID,
                                                                    filter_query="osProductFamilyId='{}'".format(
                                                                        family_op.metadata.os_product_family_id))
    if active:
        for v in list_result.os_product_family_versions:
            marketplace_private_client.set_image_id_product_family_version(v.id, generate_id())
            marketplace_private_client.set_status_product_family_version(v.id, OsProductFamilyVersion.Status.REVIEW)
            marketplace_private_client.set_status_product_family_version(v.id, OsProductFamilyVersion.Status.ACTIVATING)
            marketplace_private_client.set_status_product_family_version(v.id, OsProductFamilyVersion.Status.ACTIVE)
    return list_result.os_product_family_versions[0]


def test_get_product_error(marketplace_client: MarketplaceClient):
    with pytest.raises(ApiError) as api_err:
        marketplace_client.get_os_product(PRODUCT_ID)

    assert "Invalid marketplace product ID." in str(api_err.value)


def test_product_create(marketplace_client: MarketplaceClient):
    name = "product-{}".format(timestamp())
    create_operation = _create_product(marketplace_client, name)

    assert create_operation.done is False


def test_product_create_invalid_publisher(marketplace_client: MarketplaceClient):
    name = "product-{}".format(timestamp())
    with pytest.raises(ApiError):
        _create_product(marketplace_client, name)


def test_get_non_public_product(marketplace_client: MarketplaceClient, gauthling_context):
    name = "product-{}".format(timestamp())
    create_operation = _create_product(marketplace_client, name)

    #  unauthorized user should not get non-public product
    with pytest.raises(ApiError), gauthling_context(auth=True, authz=False):
        marketplace_client.get_os_product(create_operation.metadata.os_product_id)

    #  non-public product should not be accessible via public get endpoint
    with pytest.raises(ApiError):
        marketplace_client.get_public_os_product(create_operation.metadata.os_product_id)


def test_get_product_slug(marketplace_client: MarketplaceClient,
                          marketplace_private_client: MarketplacePrivateClient,
                          generate_id):
    name = "public-product-{}".format(timestamp())
    vendor = "Мазай"
    public_product = _create_product(marketplace_client, name, vendor=vendor, meta={}, slug=name)
    product_id = public_product.metadata.os_product_id
    _create_family_for_product(marketplace_client, marketplace_private_client, generate_id, "family-1",
                               product_id, True)

    pp = marketplace_client.get_public_os_product(product_id)
    assert pp.status in OsProduct.Status.PUBLIC
    assert pp.vendor == vendor
    assert name == pp.slugs[0].slug

    pp2 = marketplace_client.get_public_os_product(name)
    assert pp2.id == pp.id

    marketplace_private_client.add_slug("test_slug2", pp2.id, product_type=ProductType.OS.upper())
    pp3 = marketplace_client.get_public_os_product("test_slug2")
    assert pp3.id == pp.id
    assert len(pp3.slugs) == 2
    assert {pp3.slugs[0].slug, pp3.slugs[1].slug} == {name, "test_slug2"}

    marketplace_private_client.del_slug("test_slug2")
    pp4 = marketplace_client.get_public_os_product(name)
    assert pp4.id == pp.id
    assert name == pp4.slugs[0].slug
    assert len(pp4.slugs) == 1


def test_get_product_slug_update(marketplace_client: MarketplaceClient,
                                 marketplace_private_client: MarketplacePrivateClient,
                                 generate_id):
    name = "public-product-{}".format(timestamp())
    vendor = "Мазай"
    public_product = _create_product(marketplace_client, name, vendor=vendor, meta={}, slug=name)
    product_id = public_product.metadata.os_product_id
    _create_family_for_product(marketplace_client, marketplace_private_client, generate_id, "family-3", product_id,
                               True)

    pp = marketplace_client.get_public_os_product(product_id)
    assert pp.status in OsProduct.Status.PUBLIC
    assert pp.vendor == vendor
    assert name == pp.slugs[0].slug

    marketplace_client.update_os_product(
        product_id=pp.id,
        slug=name,
    )


def test_get_public_product(marketplace_client: MarketplaceClient,
                            marketplace_private_client: MarketplacePrivateClient,
                            generate_id,
                            gauthling_context):
    name = "public-product-{}".format(timestamp())
    vendor = "Мазай"
    meta_key = "asd"
    meta_value = "фывапролджэ"
    public_product = _create_product(marketplace_client, name, vendor=vendor, meta={
        meta_key: {
            "en": meta_value,
            "ru": meta_value,
        },
    })
    product_id = public_product.metadata.os_product_id
    _create_family_for_product(marketplace_client, marketplace_private_client, generate_id, "family-1",
                               product_id, True)

    #  unauthorized user should not get product via `manage` endpoint
    with pytest.raises(ApiError), gauthling_context(auth=True, authz=False):
        marketplace_client.get_os_product(product_id)
    #
    #  public product should be accessible via public get endpoint
    pp = marketplace_client.get_public_os_product(product_id)
    assert pp.status in OsProduct.Status.PUBLIC
    assert pp.vendor == vendor
    assert pp.meta.get(meta_key) == meta_value


def test_own_product_list(marketplace_client: MarketplaceClient, generate_id,
                          marketplace_private_client: MarketplacePrivateClient):
    category = marketplace_private_client.create_category({"en": "test cat {}".format(timestamp())},
                                                          Category.Type.PUBLIC,
                                                          0).response
    first_product_id = _create_product_id(marketplace_client, "product-product", category_ids=[category.id])
    _create_family_for_product(marketplace_client, marketplace_private_client, generate_id, "family-1",
                               first_product_id, True)
    second_product_id = _create_product_id(marketplace_client, "product-product", category_ids=[category.id])
    _create_family_for_product(marketplace_client, marketplace_private_client, generate_id, "family-2",
                               second_product_id, False)
    os_products = marketplace_client.list_os_products(BA_ID,
                                                      filter_query="categoryId='{}'".format(category.id)).os_products

    assert len(os_products) == 2
    assert sorted(x.id for x in os_products) == sorted([first_product_id, second_product_id])
    assert [x.status for x in os_products if x.status == OsProduct.Status.ACTIVE]
    assert [x.status for x in os_products if x.status == OsProduct.Status.NEW]


def test_public_product_list(marketplace_client: MarketplaceClient, generate_id,
                             marketplace_private_client: MarketplacePrivateClient):
    category = marketplace_private_client.create_category({"en": "test cat {}".format(timestamp())},
                                                          Category.Type.PUBLIC,
                                                          0).response
    first_product_id = _create_product_id(marketplace_client, "product-product", category_ids=[category.id])
    _create_family_for_product(marketplace_client, marketplace_private_client, generate_id, "family-1",
                               first_product_id, True)
    second_product_id = _create_product_id(marketplace_client, "product-product", category_ids=[category.id])
    _create_family_for_product(marketplace_client, marketplace_private_client, generate_id, "family-2",
                               second_product_id, False)
    list_result = marketplace_client.list_public_os_products(filter_query="categoryId='{}'".format(category.id))

    assert len(list_result.os_products) == 1
    assert list_result.os_products[0].id == first_product_id
    assert list_result.os_products[0].status == OsProduct.Status.ACTIVE


def test_public_product_list_families_alphabetically(marketplace_client: MarketplaceClient, generate_id,
                                                     marketplace_private_client: MarketplacePrivateClient):
    category = marketplace_private_client.create_category({"en": "test cat {}".format(timestamp())},
                                                          Category.Type.PUBLIC,
                                                          0).response
    names = [{
        "en": "alpha",
        "ru": "аз",
    }, {
        "en": "bravo",
        "ru": "буки",
    }, {
        "en": "charlie",
        "ru": "веди",
    }]

    pid = _create_product_id(marketplace_client, _translate_id("os_product", "name"),
                             category_ids=[category.id],
                             meta={"name": {"en": "alpha", "ru": "аз", }})
    for name in names:
        _create_family_for_product(marketplace_client, marketplace_private_client, generate_id,
                                   _translate_id("os_product_family", "name"),
                                   pid, True,
                                   meta={
                                       "name": name,
                                   })
    list_result = marketplace_client.list_public_os_products(filter_query="categoryId='{}'".format(category.id))

    assert len(list_result.os_products) == 1
    for index, name in enumerate(natsorted(n["en"] for n in names)):
        assert list_result.os_products[0].os_product_families[index].name == name

    list_result = marketplace_client.list_public_os_products(filter_query="categoryId='{}'".format(category.id),
                                                             lang="ru")

    assert len(list_result.os_products) == 1
    for index, name in enumerate(natsorted(n["ru"] for n in names)):
        assert list_result.os_products[0].os_product_families[index].name == name


def test_public_product_list_families_natsort(marketplace_client: MarketplaceClient, generate_id,
                                              marketplace_private_client: MarketplacePrivateClient):
    category = marketplace_private_client.create_category({"en": "test cat {}".format(timestamp())},
                                                          Category.Type.PUBLIC,
                                                          0).response
    names = [{
        "en": "5 lic",
        "ru": "5 лицензий",
    }, {
        "en": "200 lic",
        "ru": "200 лицензий",
    }, {
        "en": "10 lic",
        "ru": "10 лицензий",
    }]

    pid = _create_product_id(marketplace_client, _translate_id("os_product", "name"),
                             category_ids=[category.id],
                             meta={"name": {"en": "alpha", "ru": "аз", }})
    for name in names:
        _create_family_for_product(marketplace_client, marketplace_private_client, generate_id,
                                   _translate_id("os_product_family", "name"),
                                   pid, True,
                                   meta={
                                       "name": name,
                                   })
    list_result = marketplace_client.list_public_os_products(filter_query="categoryId='{}'".format(category.id))

    assert len(list_result.os_products) == 1
    for index, name in enumerate(natsorted(n["en"] for n in names)):
        assert list_result.os_products[0].os_product_families[index].name == name

    list_result = marketplace_client.list_public_os_products(filter_query="categoryId='{}'".format(category.id),
                                                             lang="ru")

    assert len(list_result.os_products) == 1
    for index, name in enumerate(natsorted(n["ru"] for n in names)):
        assert list_result.os_products[0].os_product_families[index].name == name


def test_public_product_batch(marketplace_client: MarketplaceClient, generate_id,
                              marketplace_private_client: MarketplacePrivateClient):
    first_product_id = _create_product_id(marketplace_client, "product-product", )
    ver_1 = _create_family_for_product(marketplace_client, marketplace_private_client, generate_id, "family-1",
                                       first_product_id, True)
    second_product_id = _create_product_id(marketplace_client, "product-product", )
    ver_2 = _create_family_for_product(marketplace_client, marketplace_private_client, generate_id, "family-2",
                                       second_product_id, False)
    result = marketplace_client.get_batch_public_os_products(ids=[ver_1.id, ver_2.id])

    assert len(result) == 1
    assert ver_1.id in result
    assert ver_2.id not in result


def test_product_update(marketplace_client: MarketplaceClient, generate_id,
                        marketplace_private_client: MarketplacePrivateClient):
    category = marketplace_private_client.create_category({"en": "test cat {}".format(timestamp())},
                                                          Category.Type.PUBLIC,
                                                          0).response
    first_product_id = _create_product_id(marketplace_client, "product-product", category_ids=[category.id])
    _create_family_for_product(marketplace_client, marketplace_private_client, generate_id, "family-1",
                               first_product_id, True)
    second_product_id = _create_product_id(marketplace_client, "product-product", category_ids=[category.id])
    _create_family_for_product(marketplace_client, marketplace_private_client, generate_id, "family-2",
                               second_product_id, False)
    list_result = marketplace_client.list_os_products(BA_ID)

    first_product = list_result.os_products[0]
    family = _create_family_for_product(marketplace_client, marketplace_private_client, generate_id, "family-1",
                                        first_product.id, True)

    new_name = "updated-product-{}".format(timestamp())
    new_description = "updated description {}".format(timestamp())
    new_short_description = "updated short desc {}".format(timestamp())
    update_operation = marketplace_client.update_os_product(first_product.id, new_name,
                                                            primary_family_id=family.id,
                                                            short_description=new_short_description,
                                                            description=new_description)

    assert update_operation.metadata.os_product_id == first_product.id
    assert update_operation.done is False


@pytest.mark.skip(reason="Not worked with fixtures")
def test_product_ordering(marketplace_client: MarketplaceClient, generate_id, default_os_product_category,
                          marketplace_private_client: MarketplacePrivateClient):
    ids = []
    for i in range(1, 4):
        create_op = _create_product(marketplace_client, "ordered-product-{}".format(i))
        pid = create_op.metadata.os_product_id
        _create_family_for_product(marketplace_client, marketplace_private_client, generate_id,
                                   "ord-family-{}".format(i), pid, True)
        ids.append(pid)
    list_result = marketplace_client.list_public_os_products()
    list_ids = [item.id for item in list_result.os_products]
    for id in ids:
        assert id in list_ids

    id100500 = ids[0]
    marketplace_private_client.set_order_for_os_product(id100500, {default_os_product_category: timestamp()})

    updated_list = marketplace_client.list_public_os_products()
    updated_product = marketplace_client.get_os_product(id100500)
    # Check that order changed
    assert id100500 != updated_list.os_products[0].id

    # newly created product should have only default category, that should be listed in `category_ids`
    assert [default_os_product_category] == updated_product.category_ids

    category = marketplace_private_client.create_category({"en": "test cat {}".format(timestamp())},
                                                          Category.Type.PUBLIC,
                                                          0).response

    first_two_ids = ids[:-1]
    for id in first_two_ids:
        marketplace_client.update_os_product(id, category_ids=[category.id])
    query = "categoryId='{}'".format(category.id)
    products_in_cat = marketplace_client.list_public_os_products(filter_query=query)
    assert len(products_in_cat.os_products) == 2
    assert set(first_two_ids) == {item.id for item in products_in_cat.os_products}

    first_id = products_in_cat.os_products[0].id
    second_id = products_in_cat.os_products[1].id
    marketplace_private_client.set_order_for_os_product(second_id, {category.id: timestamp()})

    ordered_products_in_cat = marketplace_client.list_public_os_products(filter_query=query)

    assert ordered_products_in_cat.os_products[-1].id == second_id

    marketplace_private_client.remove_from_category(category.id, [second_id])

    only_first_in_cat = marketplace_client.list_public_os_products(filter_query=query)

    assert len(only_first_in_cat.os_products) == 1
    assert only_first_in_cat.os_products[0].id == first_id

    marketplace_private_client.remove_from_category(category.id, [id100500])


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
        create_op = _create_product(marketplace_client, name, [category.id],
                                    slug="test_product_listing_by_non_unique_field_{}".format(i))
        pid = create_op.metadata.os_product_id
        _create_family_for_product(marketplace_client, marketplace_private_client, generate_id,
                                   "ord-family-{}".format(i), pid, True)
        ids.append((name, pid))

    # Act
    query = "categoryId='{}'".format(category.id)
    products = marketplace_client.list_public_os_products(3, filter_query=query, order_by="name")
    listed_ids = [p.id for p in products.os_products]
    next_page_token = products.next_page_token
    while next_page_token:
        more_products = marketplace_client.list_public_os_products(3, page_token=next_page_token, filter_query=query,
                                                                   order_by="name")
        listed_ids += [p.id for p in more_products.os_products]
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
        create_op = _create_product(marketplace_client, name, [category.id])
        pid = create_op.metadata.os_product_id
        _create_family_for_product(marketplace_client, marketplace_private_client, generate_id,
                                   "ord-family-{}".format(i), pid, True)
        ids[c] = pid

    # Act
    marketplace_private_client.put_os_product_on_place(ids["A"], category.id, 2)
    marketplace_private_client.put_os_product_on_place(ids["C"], category.id, 2)
    marketplace_private_client.put_os_product_on_place(ids["F"], category.id, 2)
    marketplace_private_client.put_os_product_on_place(ids["H"], category.id, 2)
    query = "categoryId='{}'".format(category.id)
    public_products = marketplace_client.list_public_os_products(10, filter_query=query).os_products
    listed_ids = [p.id for p in public_products]

    # Assert
    # as half items in category have same value to be ordered by, inside they will be sorted by id
    assert listed_ids == [ids["B"], ids["A"], ids["H"], ids["F"], ids["C"], ids["D"], ids["E"], ids["G"], ids["I"],
                          ids["J"]]


def test_product_update_does_not_affect_ordering(marketplace_client: MarketplaceClient, generate_id,
                                                 marketplace_private_client: MarketplacePrivateClient,
                                                 default_os_product_category):
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
        create_op = _create_product(marketplace_client, name, [category_a.id, category_b.id])
        pid = create_op.metadata.os_product_id
        _create_family_for_product(marketplace_client, marketplace_private_client, generate_id,
                                   "ord-family-{}".format(i), pid, True)
        ids[c] = pid

    marketplace_private_client.put_os_product_on_place(ids["D"], category_a.id, 0)
    new_d_name = "Updated D"
    marketplace_client.update_os_product(ids["D"], new_d_name, category_ids=[category_a.id])
    updated_d = marketplace_client.get_os_product(ids["D"])

    assert updated_d.name == new_d_name
    assert sorted(updated_d.category_ids) == sorted([category_a.id, default_os_product_category])

    query = "categoryId='{}'".format(category_a.id)
    category_a_list = marketplace_client.list_public_os_products(filter_query=query)

    assert category_a_list.os_products[0].id == ids["D"]


script_dir = os.path.dirname(__file__)
test_image = "test_data/ubuntu.png"
