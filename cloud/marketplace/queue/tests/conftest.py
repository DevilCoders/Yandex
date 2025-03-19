import pytest

import yc_common
from yc_common.clients.identity_v3 import IdentityClient


@pytest.fixture(autouse=True)
def patchers(monkeypatch):
    config = {
        "endpoints": {
            "cloud_api": "http://test.cloud.yandex.net",
            "billing": {
                "url": "/billing",
            },
            "identity": {
                "url": "/identity",
            },
            "kikimr": {
                "marketplace": {
                    "host": "kikimr:2135",
                    "root": "/Root/Marketplace",
                    "database": "/Root/Marketplace",
                },
            },
            "instance_metadata": {
                "url": "http://169.254.169.254",
                "enabled": False,
            },
            "compute": {
                "url": "compute",
            },
            "memcached": {
                "host": "localhost",
                "port": "11211",
            },
            "logbroker": {
                "host": "localhost",
                "port": "2135",
                "topic_template": "yc-pre/marketplace/export-{subject}",
                "auth": {
                    "client_id": 1,
                    "secret": "secret",
                    "destination": 2,
                },
            },
        },
        "marketplace": {
            "id_prefix": "abc",
            "token": "nonce",
            "service_account": {
                "key_id": "b0000000000000000007",
                "cloud_id": "foo0000service0cloud",
                "folder_id": "foo00000000000000002",
                "service_account_id": "foo40000000000000007",
                "service_account_login": "marketplace",
                "private_key_path": "/path/to/nowhere",
            },
            "service_id": "a6q18hf9bmtibm3ev42v",
            "balance_product_id": "509071",
        },
        "logbroker_export": {
            "tables": ["i18n"],
        },
    }

    monkeypatch.setattr(yc_common.config, "load", lambda *a, **k: True)
    monkeypatch.setattr(yc_common.config, "_CONFIG", config)
    monkeypatch.setattr(IdentityClient, "get_iam_token",
                        lambda *a, **k: "test_token")
