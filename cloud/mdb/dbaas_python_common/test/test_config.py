# -*- coding: utf-8 -*-
"""
Dict-related functions tests
"""
# pylint: disable=no-self-use
import os
import os.path

import pytest

from dbaas_common.config import ConfigValidationError, ConfigSyntaxError, parse_config, parse_config_set

CONFFILE = 'test.conf'
CONFFILE_EXTRA = 'test_extra.conf'

CONFIG_SCHEMA = {
    'description': 'JSON Schema for DBaaS Worker config',
    'type': 'object',
    'properties': {
        'main': {
            'description': {'Core worker config'},
            'type': 'object',
            'properties': {
                'required_field': {
                    'type': 'string',
                },
                'default_field': {
                    'type': 'string',
                },
            },
            'required': [
                'required_field',
                'default_field',
            ],
        },
        'flat_field': {
            'type': 'string',
        },
    },
    'required': [
        'main',
        'flat_field',
    ],
}

DEFAULT_CONFIG = {
    'main': {
        'default_field': 'default_value',
    },
    'flat_field': 'flat_value',
}


def write_config(conffile, config):
    """
    Write config into specified file
    """
    with open(conffile, 'w') as fil:
        fil.write(str(config))


def delete_file(conffile):
    """
    Delete specified file
    """
    os.unlink(conffile)


class TestParseConfig:
    """
    Test parse_config function
    """

    def teardown_method(self, _):
        """
        Remove config file after tests
        """
        delete_file(CONFFILE)
        if os.path.exists(CONFFILE_EXTRA):
            delete_file(CONFFILE_EXTRA)

    def test_malformed_config(self):
        """
        Test malformed JSON in config
        """
        write_config(CONFFILE, '{malformed_json')
        with pytest.raises(ConfigSyntaxError):
            parse_config(CONFFILE, CONFIG_SCHEMA)

    def test_invalid_config(self):
        """
        Test invalid config
        """
        write_config(CONFFILE, dict())
        with pytest.raises(ConfigValidationError):
            parse_config(CONFFILE, CONFIG_SCHEMA)

    def test_good_config(self):
        """
        Test normal acceptable config
        """
        write_config(CONFFILE, '{"main": {"required_field": "required_value"}}')
        config = parse_config(CONFFILE, CONFIG_SCHEMA, default=DEFAULT_CONFIG)
        assert config.main.required_field == 'required_value'
        assert config.main.default_field == 'default_value'
        assert config.flat_field == 'flat_value'

    def test_parse_good_config_set(self):
        write_config(CONFFILE, '{"main": {"required_field": "required_value"}}')
        write_config(CONFFILE_EXTRA, '{"main": {"default_field": "default_field_from_extra"}}')
        config = parse_config_set([CONFFILE, CONFFILE_EXTRA], CONFIG_SCHEMA, default=DEFAULT_CONFIG)
        assert config.main.required_field == 'required_value'
        assert config.main.default_field == 'default_field_from_extra'
