import copy
from cloud.mdb.dbaas_worker.internal import config
from dbaas_common.config import validate_config, Validator


def test_config_schema_is_valid():
    """
    Verify that config schema is valid json schema
    """
    Validator.check_schema(config.CONFIG_SCHEMA)


def _rec_all_props_required(path, data):
    """
    Recursive check that all defined properties are required
    """
    if data.get('type') != 'object':
        return
    if 'properties' not in data:
        return
    properties = set(data['properties'].keys())
    required = set(data.get('required', []))

    diff = properties.difference(required)

    assert not diff, f'object {"->".join(path)} has non-required properties: {", ".join(sorted(diff))}'

    rev_diff = required.difference(properties)

    assert not diff, f'object {"->".join(path)} requires undefined properties: {", ".join(sorted(rev_diff))}'

    for key, value in data['properties'].items():
        _rec_all_props_required(path + [key], value)


def test_config_all_props_required():
    """
    Verify that all defined properties are required config schema objects
    """
    _rec_all_props_required(['root'], config.CONFIG_SCHEMA)


def _rec_has_no_additional_props(path, data):
    """
    Recursive check that no additional properties are set on all object children
    """
    if data.get('type') != 'object':
        return
    if 'properties' not in data:
        return
    opt = data.get('additionalProperties')
    assert not opt and opt is not None, f'object {"->".join(path)} has enabled additional properties'
    for key, value in data['properties'].items():
        _rec_has_no_additional_props(path + [key], value)


def test_config_no_additional_props():
    """
    Verify that no additional properties allowed in config schema objects
    """
    _rec_has_no_additional_props(['root'], config.CONFIG_SCHEMA)


def test_default_config_right_type():
    """
    Verify that default config properties are correct
    """
    schema_no_required = copy.deepcopy(config.CONFIG_SCHEMA)
    _rec_remove_all_props_required(['root'], schema_no_required)
    for def_key, def_value in schema_no_required['definitions'].items():
        _rec_remove_all_props_required(['definitions', def_key], def_value)

    validate_config(config.DEFAULT_CONFIG, schema_no_required)


def _rec_remove_all_props_required(path, data):
    """
    Recursively remove requirements from properties
    """
    if data.get('type') != 'object':
        return
    if 'properties' not in data:
        return

    try:
        del data['required']
    except KeyError:
        pass

    for key, value in data['properties'].items():
        _rec_remove_all_props_required(path + [key], value)
