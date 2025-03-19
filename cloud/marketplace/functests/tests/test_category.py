from yc_common.misc import timestamp
from cloud.marketplace.common.yc_marketplace_common.client import MarketplaceClient
from cloud.marketplace.common.yc_marketplace_common.client import MarketplacePrivateClient
from cloud.marketplace.common.yc_marketplace_common.models.category import Category


def test_create_category(marketplace_private_client: MarketplacePrivateClient):
    create_operation = marketplace_private_client.create_category(
        name={"en": "test category {}".format(timestamp())},
        type=Category.Type.SYSTEM,
        score=1,
    )

    assert create_operation.done is True


def test_get_category(marketplace_client: MarketplaceClient, db_fixture):
    cat = marketplace_client.get_category(db_fixture["category"][0]["id"])

    assert cat.name == db_fixture["category"][0]["name"]
    assert cat.type == db_fixture["category"][0]["type"]
    assert cat.score == db_fixture["category"][0]["score"]


# TODO when front stop using this handle
# def test_list_category(marketplace_client: MarketplaceClient, db_fixture):
#     cl = marketplace_client.list_categories(filter_query="parent_id='{}'".format(db_fixture["category"][0]["id"]))
#
#     assert len(cl.categories) == 2
#     assert len(list(filter(lambda x: x.name[:5] == "child", cl.categories))) == 2


def test_default_category_list(marketplace_client: MarketplaceClient, db_fixture):
    def _check(categories, type):
        assert len(categories.categories) == len(list(filter(lambda x: x["type"] == type, db_fixture["category"])))

    _check(marketplace_client.list_isv_categories(), Category.Type.ISV)
    _check(marketplace_client.list_var_categories(), Category.Type.VAR)
    _check(marketplace_client.list_os_products_categories(), Category.Type.PUBLIC)
    _check(marketplace_client.list_publisher_categories(), Category.Type.PUBLISHER)
