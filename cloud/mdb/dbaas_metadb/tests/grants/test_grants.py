import yatest.common
import pytest
from cloud.mdb.dbaas_metadb.tests.grants.lib import get_grants_users, get_pillar_users


@pytest.mark.parametrize(
    'pillar_path',
    [
        'cloud/mdb/salt/pillar/mdb_controlplane_porto_test/mdb_meta_test.sls',
        'cloud/mdb/salt/pillar/mdb_controlplane_porto_prod/mdb_meta_prod.sls',
        'cloud/mdb/salt/pillar/mdb_controlplane_compute_preprod/mdb_meta_preprod.sls',
        'cloud/mdb/salt/pillar/mdb_controlplane_compute_prod/mdb_meta_prod.sls',
    ],
)
def test_all_users_are_present_in_pillar(pillar_path):
    grants = get_grants_users(yatest.common.source_path('cloud/mdb/dbaas_metadb/head/grants'))
    pillar = get_pillar_users(yatest.common.source_path(pillar_path))
    if missing := grants - pillar:
        first_missing = list(missing)[0]
        pytest.fail(
            f"""Users {missing} are not found in '{pillar_path}' pillar.
1. create user in pgusers/{first_missing}.sls:

    $ cat {first_missing}.sls
    data:
        config:
            pgusers:
                {first_missing}:
                    superuser: False
                    replication: False
                    create: True

2. include them in metadb pillar: {pillar_path}
"""
        )
