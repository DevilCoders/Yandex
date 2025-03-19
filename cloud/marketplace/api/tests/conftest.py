import pytest

from yc_common import config


@pytest.fixture(autouse=True)
def patchers(monkeypatch):
    cfg = {
        "endpoints": {
            "cloud_api": "http://test.cloud.yandex.net",
            "identity": {
                "url": "/identity",
            },
        },
        "memcached": {
            "host": "localhost",
            "port": "11211",
        },
    }

    monkeypatch.setattr(config, "load", lambda *a, **k: True)
    monkeypatch.setattr(config, "_CONFIG", cfg)
