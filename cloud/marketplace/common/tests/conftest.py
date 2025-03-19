import pytest

import yc_common


@pytest.fixture(autouse=True)
def patchers(monkeypatch):
    config = {
        "endpoints": {
            "cloud_api": "http://test.cloud.yandex.net",
            "identity": {
                "url": "/identity",
            },
            "billing": {
                "url": "/billing",
            },
            "kikimr": {
                "marketplace": {
                    "host": "kikimr:2135",
                    "root": "/marketplace",
                },
            },
            "instance_metadata": {
                "url": "http://169.254.169.254",
                "enabled": False,
            },
        },
        "marketplace": {
            "service_account": {
                "key_id": "foo40000000000000007",
                "service_account_id": "foo40000000000000007",
                "folder_id": "foo00000000000000002",
                "service_account_login": "marketplace",
                "cloud_id": "foo0000service0cloud",
                "private_key_path": "/none/service_key",
            },
            "id_prefix": "d00"
        },
        "memcached": {
            "host": "localhost",
            "port": "11211",
        },
    }

    monkeypatch.setattr(yc_common.config, "load", lambda *a, **k: True)
    monkeypatch.setattr(yc_common.config, "_CONFIG", config)
