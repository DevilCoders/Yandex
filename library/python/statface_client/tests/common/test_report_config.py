# coding: utf-8

from __future__ import division, absolute_import, print_function, unicode_literals
import pytest
import yaml
from six import text_type
from statface_client import StatfaceReportConfig
from statface_client import StatfaceReportConfigError


def test_report_api_version():
    from statface_client import StatfaceClient
    client = StatfaceClient(
        client_config={
            'username': 'robot_fake_robot_user',
            'password': 'fake_robot_password',
            'host': 'upload.stat.yandex-team.ru',
        },
        _no_excess_calls=True
    )
    r = client.get_report('Adhoc/Adhoc/MyOwnReport2')
    assert r.report_api_version == 'api'
    assert r._api._report_api_prefix == '_api/report/'
    r.report_api_version = 'v4'
    assert r.report_api_version == 'v4'
    assert r._api._report_api_prefix == '_v4/report/'


def test_base_functional():
    config = StatfaceReportConfig()
    config.title = 'заголовок'
    config.dimensions = config._fields_to_ordered_dict([{'fielddate': 'date'}])
    assert config.is_valid
    config.check_valid()
    etalon_config_dict = {
        'title': 'заголовок',
        'user_config': {
            'dimensions': [{'fielddate': 'date'}],
            'measures': []
        }
    }
    assert config.to_dict() == etalon_config_dict

    config_two = StatfaceReportConfig(title='другой заголовок')
    assert not config_two.is_valid
    config_two.update_from_dict(etalon_config_dict)
    assert config_two.to_dict() == etalon_config_dict
    assert repr(config_two) == repr(config)


def test_from_failed_dict():
    crazy_dict = {
        'lalala': 'pampampam',
        'title': '123'
    }
    config = StatfaceReportConfig()
    with pytest.raises(StatfaceReportConfigError):
        config.update_from_dict(crazy_dict)
    crazy_dict_two = {
        'dimensions': '',
        'measures': ''
    }
    with pytest.raises(StatfaceReportConfigError):
        config.update_from_dict(crazy_dict_two)


def test_yaml(config_yaml):
    yaml_file = config_yaml
    assert isinstance(yaml.safe_load(yaml_file), dict)
    config = StatfaceReportConfig()
    config.update_from_yaml(yaml_file)
    new_config = StatfaceReportConfig()
    dumped_config = config.to_yaml()
    assert isinstance(yaml.load(dumped_config), dict)
    config = StatfaceReportConfig()
    new_config.update_from_yaml(dumped_config)
    assert new_config.to_yaml() == dumped_config


def test_extra_parameters():
    etalon_dict_content = {
        'title': 'sdfsd',
        'extra': 'sdfsdfsdfs',
        'dimensions': [
            {'fielddate': 'date'}
        ],
    }

    etalon_dict = {
        'title': etalon_dict_content['title'],
        'user_config': {
            'dimensions': etalon_dict_content['dimensions'],
            'measures': [],
            'extra': etalon_dict_content['extra']
        }
    }

    config = StatfaceReportConfig(**etalon_dict_content)
    assert config.to_dict() == etalon_dict


def test_buggy_yaml(buggy_yaml):  # pylint: disable=redefined-outer-name
    config = StatfaceReportConfig()
    with pytest.raises(StatfaceReportConfigError):
        config.update_from_yaml(buggy_yaml)


def test_yaml_passthrough(user_config_yaml):  # pylint: disable=redefined-outer-name
    user_config_yaml_text = (
        user_config_yaml.decode('utf-8') if isinstance(user_config_yaml, bytes)
        else user_config_yaml)
    title = u"Отчёт с YAML-конфигом"

    config = StatfaceReportConfig(title=title, user_config=user_config_yaml)
    assert config.user_config_yaml() == user_config_yaml_text
    config.title = 'other title'
    assert config.user_config_yaml() == user_config_yaml_text
    config.measures['m3'] = 'number'
    new_yaml = config.user_config_yaml()
    assert isinstance(new_yaml, text_type)
    assert new_yaml != user_config_yaml_text

    # The reverse check:
    unsourced_config = StatfaceReportConfig(
        title=title, user_config=yaml.safe_load(user_config_yaml))
    assert unsourced_config.user_config_yaml() != user_config_yaml_text
