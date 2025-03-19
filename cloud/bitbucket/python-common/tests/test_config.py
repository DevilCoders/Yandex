import pytest

import yaml

from yc_common.config import load_raw_config, merge_raw_config


@pytest.fixture(scope="session")
def config_yaml(tmpdir_factory):
    fname = tmpdir_factory.mktemp("config").join("config.yaml")
    with open(str(fname), "w") as config_f:
        yaml.dump({"section": {"value1": 1}}, config_f)

    return fname


def test_load_raw_config(config_yaml):
    cfg = load_raw_config(str(config_yaml))

    assert cfg == {"section": {"value1": 1}}


def test_merge_raw_config():
    cfg = {"section": {"value1": 1}}
    merge_raw_config(cfg, {"section": {"value1": 2, "value2": 2}})

    # Use default value2 from defaults, but do not touch value1
    assert cfg == {"section": {"value1": 1, "value2": 2}}


def test_merge_raw_config_update():
    cfg = {"section": {"value1": 1}}
    merge_raw_config(cfg, {"section": {"value1": 2, "value2": 2}},
                     update=True)

    assert cfg == {"section": {"value1": 2, "value2": 2}}


def test_merge_raw_config_custom_merger():
    def sum_merger(d1, d2):
        result = dict()
        for k in set(d1) & set(d2):
            result[k] = d1[k] + d2[k]
        return result

    def empty_merger(d1, d2):
        return dict()

    custom_mergers = {
        ("key1", "subkey1"): sum_merger,
        ("key1", "subkey2"): empty_merger
    }

    config = {"key1": {"subkey0": 123, "subkey1": {"subsubkey1": 3, "subsubkey3": 15}, "subkey2": {"y": 111}}}
    other = {"key1": {"subkey1": {"subsubkey2": "gagaga", "subsubkey3": 20}, "subkey2": {"z": 222}}}

    merge_raw_config(config, other, update=True, custom_mergers=custom_mergers)

    assert config == {"key1": {"subkey0": 123, "subkey1": {"subsubkey3": 35}, "subkey2" : {}}}
