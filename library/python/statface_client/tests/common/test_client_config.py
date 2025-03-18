from __future__ import division, absolute_import, print_function, unicode_literals
import pytest

from statface_client.api import ClientConfig
from statface_client.errors import StatfaceClientValueError


def test_modification():
    with pytest.raises(StatfaceClientValueError):
        client = ClientConfig({1: 2})

    client = ClientConfig({})
    with pytest.raises(StatfaceClientValueError):
        client.check_valid()
    with pytest.raises(StatfaceClientValueError):
        client = ClientConfig({'reports': 1})

    client = ClientConfig(dict(username='1', password='2', host='3'))
    client.check_valid()
    with pytest.raises(KeyError):
        client['lalala'] = 1
    with pytest.raises(AttributeError):
        del client['host']


def test_statface_config(statface_conf_yaml):
    client = ClientConfig(statface_conf_yaml)
    client.check_valid()
