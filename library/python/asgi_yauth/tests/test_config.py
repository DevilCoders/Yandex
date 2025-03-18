import pytest
import pydantic

from asgi_yauth.settings import AsgiYauthConfig
from asgi_yauth.types import BlackboxClient
from asgi_yauth.backends.tvm2 import Backend


def test_settings_base_success(monkeypatch):
    monkeypatch.setenv('YAUTH_TVM2_CLIENT', '28')
    monkeypatch.setenv('YAUTH_TVM2_ALLOWED_CLIENTS', '["2000324", 234]')

    config = AsgiYauthConfig()

    assert config.tvm2_secret is None
    assert config.tvm2_client == '28'
    assert config.tvm2_allowed_clients == [2000324, 234]
    assert config.tvm2_blackbox_client == BlackboxClient.BB_MAP['prod_yateam']
    assert config.backends == [Backend]


def test_settings_override_success(monkeypatch):
    monkeypatch.setenv('YAUTH_TVM2_SECRET', 'my_secret')
    monkeypatch.setenv('YAUTH_TVM2_BLACKBOX_CLIENT', 'testing')

    config = AsgiYauthConfig()

    assert str(config.tvm2_secret) == '**********'
    assert config.tvm2_secret.get_secret_value() == 'my_secret'
    assert config.tvm2_blackbox_client == BlackboxClient.BB_MAP['testing']


def test_settings_validate_success(monkeypatch):
    monkeypatch.setenv('YAUTH_BACKENDS', '["asgi_yauth.backends.some_backend"]')
    with pytest.raises(pydantic.error_wrappers.ValidationError):
        AsgiYauthConfig()
