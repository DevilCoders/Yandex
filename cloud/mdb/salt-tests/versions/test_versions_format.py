import yatest.common
import yaml


def test_versions_format():
    """
    test that versions.sls is Dict[str, Dict[str, str]]
    """
    with open(yatest.common.source_path('cloud/mdb/salt/pillar/versions.sls')) as inp:
        versions = yaml.safe_load(inp)
        assert isinstance(versions, dict), 'versions.sls should be a dict'
        for env, env_pins in versions.items():
            assert isinstance(env, str), f'env key should be a str. got {env!r}'
            assert isinstance(env_pins, dict), f'{env} value should be a dict'
            for component, pin in env_pins.items():
                assert isinstance(component, str), f'component should be a str. got {component!r}'
                assert isinstance(pin, str), f'pin should be a str. got {pin!r} for {component!r}'
                if pin == 'trunk':
                    continue
                assert pin.isdigit(), f'pin {pin!r} for {component!r} should be \'trunk\' or svn revision'
