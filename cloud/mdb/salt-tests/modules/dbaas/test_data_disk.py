import json
from cloud.mdb.salt.salt._modules import dbaas
from cloud.mdb.salt_tests.common.mocks import mock_pillar


def pretty_json(obj):
    return json.dumps(
        obj,
        sort_keys=True,
        indent=4,
    )


class Test_data_disk:  # noqa
    """
    Collect sample data with:

    lsblk --json --output=NAME,TYPE,MOUNTPOINT,SIZE
    """

    def assert_equal(self, lsblk_in, disk_out):
        assert pretty_json(dbaas._extract_data_disk_from_lsblk_out(lsblk_in)) == pretty_json(disk_out)

    def test_mounted_for_compute(self):
        mock_pillar(dbaas.__salt__, {})
        self.assert_equal(
            """
        {
           "blockdevices": [
              {"name": "vda", "type": "disk", "mountpoint": null, "size": "20G",
                 "children": [
                    {"name": "vda1", "type": "part", "mountpoint": "/", "size": "20G"}
                 ]
              },
              {"name": "vdb", "type": "disk", "mountpoint": null, "size": "256G",
                 "children": [
                    {"name": "vdb1", "type": "part", "mountpoint": "/var/lib/clickhouse", "size": "256G"}
                 ]
              }
           ]
        }
        """,
            {
                'device': {
                    'name': 'vdb',
                    'size': '256G',
                },
                'partition': {
                    'name': 'vdb1',
                    'size': '256G',
                    'exists': True,
                },
            },
        )

    def test_mounted_for_aws(self):
        mock_pillar(dbaas.__salt__, {})
        self.assert_equal(
            """
        {
           "blockdevices": [
              {"name": "loop0", "type": "loop", "mountpoint": "/snap/amazon-ssm-agent/3552", "size": "33.3M"},
              {"name": "loop1", "type": "loop", "mountpoint": "/snap/core18/2074", "size": "55.5M"},
              {"name": "loop2", "type": "loop", "mountpoint": "/snap/snapd/12398", "size": "32.3M"},
              {"name": "loop3", "type": "loop", "mountpoint": "/snap/snapd/12704", "size": "32.3M"},
              {"name": "loop4", "type": "loop", "mountpoint": "/snap/amazon-ssm-agent/4046", "size": "25M"},
              {"name": "nvme1n1", "type": "disk", "mountpoint": null, "size": "40G",
                 "children": [
                    {"name": "nvme1n1p1", "type": "part", "mountpoint": "/var/lib/clickhouse", "size": "40G"}
                 ]
              },
              {"name": "nvme0n1", "type": "disk", "mountpoint": null, "size": "20G",
                 "children": [
                    {"name": "nvme0n1p1", "type": "part", "mountpoint": "/", "size": "20G"}
                 ]
              }
           ]
        }
        """,
            {
                'device': {
                    'name': 'nvme1n1',
                    'size': '40G',
                },
                'partition': {
                    'name': 'nvme1n1p1',
                    'size': '40G',
                    'exists': True,
                },
            },
        )

    def test_not_mounted_aws(self):
        mock_pillar(dbaas.__salt__, {})
        self.assert_equal(
            """
        {
           "blockdevices": [
              {"name": "loop0", "type": "loop", "mountpoint": "/snap/amazon-ssm-agent/4046", "size": "25M"},
              {"name": "loop1", "type": "loop", "mountpoint": "/snap/amazon-ssm-agent/3552", "size": "33.3M"},
              {"name": "loop2", "type": "loop", "mountpoint": "/snap/core18/2066", "size": "55.4M"},
              {"name": "loop3", "type": "loop", "mountpoint": "/snap/snapd/12398", "size": "32.3M"},
              {"name": "loop4", "type": "loop", "mountpoint": "/snap/core18/2074", "size": "55.5M"},
              {"name": "loop5", "type": "loop", "mountpoint": "/snap/snapd/12704", "size": "32.3M"},
              {"name": "nvme1n1", "type": "disk", "mountpoint": null, "size": "10G"},
              {"name": "nvme0n1", "type": "disk", "mountpoint": null, "size": "20G",
                 "children": [
                    {"name": "nvme0n1p1", "type": "part", "mountpoint": "/", "size": "20G"}
                 ]
              }
           ]
        }
        """,
            {
                'device': {
                    'name': 'nvme1n1',
                    'size': '10G',
                },
                'partition': {
                    'name': 'nvme1n1p1',
                    'size': '0',
                    'exists': False,
                },
            },
        )
