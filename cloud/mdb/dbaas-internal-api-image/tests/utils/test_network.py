import json
from dbaas_internal_api.utils.network import group_subnets_by_zones
from cloud.mdb.internal.python.compute.vpc import Subnet


def test_group_subnets_by_zones():
    ret = group_subnets_by_zones(
        iter(
            [
                Subnet(
                    folder_id='f1',
                    zone_id='zone_a',
                    subnet_id='s1',
                    network_id='n1',
                    v4_cidr_blocks=[],
                    v6_cidr_blocks=[],
                    egress_nat_enable=False,
                ),
                Subnet(
                    folder_id='f2',
                    zone_id='zone_a',
                    subnet_id='s2',
                    network_id='n1',
                    v4_cidr_blocks=[],
                    v6_cidr_blocks=[],
                    egress_nat_enable=False,
                ),
                Subnet(
                    folder_id='f3',
                    zone_id='zone_b',
                    subnet_id='s3',
                    network_id='n1',
                    v4_cidr_blocks=[],
                    v6_cidr_blocks=[],
                    egress_nat_enable=False,
                ),
            ]
        )
    )

    expected = {
        'zone_a': {
            's1': 'f1',
            's2': 'f2',
        },
        'zone_b': {
            's3': 'f3',
        },
    }

    assert json.dumps(ret, sort_keys=True, indent=2) == json.dumps(expected, sort_keys=True, indent=2)
