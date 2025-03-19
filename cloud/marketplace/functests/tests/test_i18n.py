from cloud.marketplace.common.yc_marketplace_common.client import MarketplacePrivateClient
from cloud.marketplace.common.yc_marketplace_common.models.i18n import I18n


def test_i18n_simple(marketplace_private_client: MarketplacePrivateClient):
    res = marketplace_private_client.bulk_set_i18n({"translations": [
        {
            "ru": "ФЫВА",
        },
        {
            "ru": "ФЫВА2",
        },
    ]})
    for t in res.translations:
        print(t)
        assert t["ru"] == marketplace_private_client.get_i18n(t["_id"], "ru").get("text")


def test_i18n_langs(marketplace_private_client: MarketplacePrivateClient):
    res = marketplace_private_client.bulk_set_i18n({"translations": [
        {
            "ru": "ФЫВА",
            "en": "Fifa",
        },
    ]})
    for t in res.translations:
        assert t["ru"] == marketplace_private_client.get_i18n(t["_id"], "ru").get("text")
        assert t["en"] == marketplace_private_client.get_i18n(t["_id"], "en").get("text")


def test_i18n_own_id(marketplace_private_client: MarketplacePrivateClient):
    translations = {
        "ru": "ФЫВА7",
        "en": "Fifa7",
        "_id": "testtttttt",
    }
    res = marketplace_private_client.bulk_set_i18n({"translations": [translations]})
    assert I18n.PREFIX + translations["_id"] == res.translations[0]["_id"]
    assert translations["ru"] == marketplace_private_client.get_i18n(res.translations[0]["_id"], "ru").get("text")
