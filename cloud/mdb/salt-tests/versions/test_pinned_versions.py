"""
Check pinned salt components
"""

import pathlib

import yaml
from yatest.common import source_path

TEST_ONLY = {
    'components/arcadia-build-node/',
    'components/blackbox-sso/',
    'components/buildvm/',
    'components/datacloud/',
    'components/dataproc-infratests/',
    'components/disklock/',
    'components/docker-common/',
    'components/docker-host/',
    'components/gateway/',
    'components/gerrit/',
    'components/graphene-load-node/',
    'components/jenkins/',
    'components/kvm-host/',
    'components/mocks/',
    'components/vector_agent_dataplane/',
    'components/mdb-tank/',
    'components/mdb-telegraf-windows/',
}

PORTO_ONLY = {
    'components/cauth/',
    'components/dbaas-porto-controlplane/',
    'components/dbaas-porto/',
    'components/db-maintenance/',
    'components/dom0porto/',
    'components/gideon/',
    'components/hw-watcher/',
    'components/mdb_cms/',
    'components/netmon/',
    'components/wall-e/',
}


SKIP = {
    'prod': TEST_ONLY,
    'compute-prod': TEST_ONLY.union(PORTO_ONLY),
}


def get_components():
    """
    Get a set with all salt components
    """
    return {
        f'{"/".join(str(x).split("/")[-2:])}/'
        for x in pathlib.Path(source_path('cloud/mdb/salt/salt/components')).glob('*')
        if not str(x).endswith('ya.make')
    }


def get_pinned_components():
    """
    Get a map of env -> set of pinned components
    """
    with open(source_path('cloud/mdb/salt/pillar/versions.sls')) as versions:
        parsed = yaml.safe_load(versions)
    res = {}
    for env, components_map in parsed.items():
        if 'prod' not in env:
            continue
        res[env] = {
            component: pin for component, pin in components_map.items() if component.endswith('/') and pin != 'trunk'
        }
    return res


def test_pinned_versions():
    """
    Check if all prod components are pinned in versions
    """
    components = get_components()
    pinned = get_pinned_components()
    for env, pinned_components in pinned.items():
        delta = components - set(pinned_components.keys()) - SKIP.get(env, set())
        msg_delta = ', '.join(sorted(delta))
        assert not delta, f'Unpinned components on {env} detected: {msg_delta}'


def test_compute_prod_behind_prod():
    """
    Check that all versions on compute-prod are <= versions on prod
    """
    pinned = get_pinned_components()
    errors = []
    for component, version in pinned['compute-prod'].items():
        prod_version = pinned['prod'].get(component)
        if prod_version and int(prod_version) < int(version):
            errors.append(f'Component {component} has {version} on compute-prod and {prod_version} on prod')
    assert not errors, ', '.join(errors)
