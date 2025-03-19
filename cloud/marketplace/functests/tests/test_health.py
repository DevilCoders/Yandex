import requests

from cloud.marketplace.common.yc_marketplace_common.client import MarketplacePrivateClient
from yc_common import config


def test_health_check(marketplace_private_client: MarketplacePrivateClient):
    response = marketplace_private_client.health()

    assert response.health == "ok"
    assert response.details.get("database:marketplace") == "ok"


def test_ping(marketplace_private_client: MarketplacePrivateClient):
    response = marketplace_private_client.ping()

    assert response


def test_health_monrun():
    url = "{}/health?monrun=true".format(config.get_value("endpoints.marketplace_private.url"))
    response = requests.get(url)

    assert response.text == "marketplace;0;OK"
