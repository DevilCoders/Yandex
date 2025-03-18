# encoding: utf-8


def test_custom_path():
    from settings import NAME
    assert NAME=='custom_path'


def test_from_envvar():
    from settings_from_envvar import settings
    assert settings.NAME=='custom_path'
