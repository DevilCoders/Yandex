from cloud.mdb.salt.salt._modules import mdb_greenplum  # noqa
from cloud.mdb.salt_tests.common.mocks import mock_pillar, mock_grains
from cloud.mdb.internal.python.pytest.utils import parametrize
from mock import patch


@parametrize(
    {
        "id": "Generate log info with master segment on host",
        "args": {
            "pillar": {
                "data": {
                    "greenplum": {
                        "segments": {
                            "-1": {
                                "mirror": {
                                    "fqdn": "dc1-example1.db.yandex.net",
                                    "mount_point": "/var/lib/greenplum/data1",
                                },
                                "primary": {
                                    "fqdn": "dc1-example2.db.yandex.net",
                                    "mount_point": "/var/lib/greenplum/data1",
                                },
                            },
                        },
                    },
                    "dbaas": {"cluster_id": "abcde"},
                },
            },
            "grains": {"id": "dc1-example1.db.yandex.net"},
            "result": {
                "segments": [
                    {
                        "id": "-1",
                        "name": "gpseg-1",
                        "preferred_role": "mirror",
                        "type": "master",
                        "data_dir": "/var/lib/greenplum/data1/master/gpseg-1/pg_log/greenplum-6-data.csv",
                        "fqdn": "dc1-example1.db.yandex.net",
                    }
                ]
            },
        },
    },
    {
        "id": "Generate log info with multiple segments on host",
        "args": {
            "pillar": {
                "data": {
                    "greenplum": {
                        "segments": {
                            "0": {
                                "mirror": {
                                    "fqdn": "dc1-example1.db.yandex.net",
                                    "mount_point": "/var/lib/greenplum/data1",
                                },
                                "primary": {
                                    "fqdn": "dc2-example2.db.yandex.net",
                                    "mount_point": "/var/lib/greenplum/data1",
                                },
                            },
                            "1": {
                                "primary": {
                                    "fqdn": "dc1-example1.db.yandex.net",
                                    "mount_point": "/var/lib/greenplum/data1",
                                },
                                "mirror": {
                                    "fqdn": "dc1-example2.db.yandex.net",
                                    "mount_point": "/var/lib/greenplum/data1",
                                },
                            },
                        },
                    },
                    "dbaas": {"cluster_id": "abcde"},
                },
            },
            "grains": {"id": "dc1-example1.db.yandex.net"},
            "result": {
                "segments": [
                    {
                        "id": "0",
                        "name": "gpseg0",
                        "preferred_role": "mirror",
                        "type": "segment",
                        "data_dir": "/var/lib/greenplum/data1/mirror/gpseg0/pg_log/greenplum-6-data.csv",
                        "fqdn": "dc1-example1.db.yandex.net",
                    },
                    {
                        "id": "1",
                        "name": "gpseg1",
                        "preferred_role": "primary",
                        "type": "segment",
                        "data_dir": "/var/lib/greenplum/data1/primary/gpseg1/pg_log/greenplum-6-data.csv",
                        "fqdn": "dc1-example1.db.yandex.net",
                    },
                ]
            },
        },
    },
    {
        "id": "Generate log info on a segment without a configured mirror",
        "args": {
            "pillar": {
                "data": {
                    "greenplum": {
                        "segments": {
                            "0": {
                                "mirror": None,
                                "primary": {
                                    "fqdn": "dc1-example1.db.yandex.net",
                                    "mount_point": "/var/lib/greenplum/data1",
                                },
                            },
                        },
                    },
                    "dbaas": {"cluster_id": "abcde"},
                },
            },
            "grains": {"id": "dc1-example1.db.yandex.net"},
            "result": {
                "segments": [
                    {
                        "id": "0",
                        "name": "gpseg0",
                        "preferred_role": "primary",
                        "type": "segment",
                        "data_dir": "/var/lib/greenplum/data1/primary/gpseg0/pg_log/greenplum-6-data.csv",
                        "fqdn": "dc1-example1.db.yandex.net",
                    },
                ]
            },
        },
    },
)
def test_get_segments_log_info(pillar, grains, result):
    mock_pillar(mdb_greenplum.__salt__, pillar)
    mock_grains(mdb_greenplum.__salt__, grains)
    assert mdb_greenplum.get_segments_log_info() == result


@parametrize(
    {
        "id": "segment pillar and data in deployed config are actual on host",
        "args": {
            "pillar": {
                "data": {
                    "greenplum": {
                        "segments": {
                            "1": {
                                "primary": {
                                    "fqdn": "dc1-example1.db.yandex.net",
                                    "mount_point": "/var/lib/greenplum/data1",
                                },
                                "mirror": {
                                    "fqdn": "dc1-example2.db.yandex.net",
                                    "mount_point": "/var/lib/greenplum/data1",
                                },
                            },
                        },
                    },
                    "dbaas": {"cluster_id": "abcde"},
                },
            },
            "mocked_data_get_gp_current_config": {
                1: {
                    "primary": {
                        "fqdn": "dc1-example1.db.yandex.net",
                        "mount_point": "/var/lib/greenplum/data1",
                    },
                    "mirror": {
                        "fqdn": "dc1-example2.db.yandex.net",
                        "mount_point": "/var/lib/greenplum/data1",
                    },
                },
            },
            "result": [
                {
                    "actual": True,
                    "debug_info": {
                        "cur_seg_data": {
                            "mirror": {
                                "fqdn": "dc1-example2.db.yandex.net",
                                "mount_point": "/var/lib/greenplum/data1",
                            },
                            "primary": {
                                "fqdn": "dc1-example1.db.yandex.net",
                                "mount_point": "/var/lib/greenplum/data1",
                            },
                        },
                        "cur_seg_id": 1,
                        "pillar_seg_data": {
                            "mirror": {
                                "fqdn": "dc1-example2.db.yandex.net",
                                "mount_point": "/var/lib/greenplum/data1",
                            },
                            "primary": {
                                "fqdn": "dc1-example1.db.yandex.net",
                                "mount_point": "/var/lib/greenplum/data1",
                            },
                        },
                        "pillar_seg_id": 1,
                    },
                },
            ],
        },
    },
    {
        "id": "segment pillar and data in deployed config are mismatch on host",
        "args": {
            "pillar": {
                "data": {
                    "greenplum": {
                        "segments": {
                            "1": {
                                "primary": {
                                    "fqdn": "dc4-example8.db.yandex.net",
                                    "mount_point": "/var/lib/greenplum/data1",
                                },
                                "mirror": {
                                    "fqdn": "dc1-example2.db.yandex.net",
                                    "mount_point": "/var/lib/greenplum/data1",
                                },
                            },
                        },
                    },
                    "dbaas": {"cluster_id": "abcde"},
                },
            },
            "mocked_data_get_gp_current_config": {
                1: {
                    "primary": {
                        "fqdn": "dc1-example1.db.yandex.net",
                        "mount_point": "/var/lib/greenplum/data1",
                    },
                    "mirror": {
                        "fqdn": "dc1-example2.db.yandex.net",
                        "mount_point": "/var/lib/greenplum/data1",
                    },
                },
            },
            "result": [
                {
                    "actual": False,
                    "debug_info": {
                        "cur_seg_data": {
                            "mirror": {
                                "fqdn": "dc1-example2.db.yandex.net",
                                "mount_point": "/var/lib/greenplum/data1",
                            },
                            "primary": {
                                "fqdn": "dc1-example1.db.yandex.net",
                                "mount_point": "/var/lib/greenplum/data1",
                            },
                        },
                        "cur_seg_id": 1,
                        "pillar_seg_data": {
                            "mirror": {
                                "fqdn": "dc1-example2.db.yandex.net",
                                "mount_point": "/var/lib/greenplum/data1",
                            },
                            "primary": {
                                "fqdn": "dc4-example8.db.yandex.net",
                                "mount_point": "/var/lib/greenplum/data1",
                            },
                        },
                        "pillar_seg_id": 1,
                    },
                    "backup": "backup: dbaas pillar get-key data -c abcde",
                    "changes": 'on cluster: abcde segment 1 changes: {"primary": {"fqdn": '
                    '"dc4-example8.db.yandex.net", "mount_point": "/var/lib/greenplum/data1"}, '
                    '"mirror": {"fqdn": "dc1-example2.db.yandex.net", "mount_point": '
                    '"/var/lib/greenplum/data1"}} -> {"primary": {"fqdn": '
                    '"dc1-example1.db.yandex.net", "mount_point": "/var/lib/greenplum/data1"}, '
                    '"mirror": {"fqdn": "dc1-example2.db.yandex.net", "mount_point": '
                    '"/var/lib/greenplum/data1"}}',
                    "update": 'update: dbaas pillar set-key data:greenplum:segments:1 '
                    '\'{"primary": {"fqdn": "dc1-example1.db.yandex.net",'
                    ' "mount_point": "/var/lib/greenplum/data1"}, "mirror":'
                    ' {"fqdn": "dc1-example2.db.yandex.net",'
                    ' "mount_point": "/var/lib/greenplum/data1"}}\' -c abcde',
                    "revert": 'rollback: dbaas pillar set-key data:greenplum:segments:1 '
                    '\'{"primary": {"fqdn": "dc4-example8.db.yandex.net",'
                    ' "mount_point": "/var/lib/greenplum/data1"}, "mirror":'
                    ' {"fqdn": "dc1-example2.db.yandex.net",'
                    ' "mount_point": "/var/lib/greenplum/data1"}}\' -c abcde',
                },
            ],
        },
    },
)
def test_is_gp_preferred_role_pillar_actual(pillar, mocked_data_get_gp_current_config, result):
    mock_pillar(mdb_greenplum.__salt__, pillar)
    with patch("cloud.mdb.salt.salt._modules.mdb_greenplum.get_gp_current_config") as get_gp_current_config_mock:
        get_gp_current_config_mock.return_value = mocked_data_get_gp_current_config
        assert mdb_greenplum.is_gp_preferred_role_pillar_actual() == result
