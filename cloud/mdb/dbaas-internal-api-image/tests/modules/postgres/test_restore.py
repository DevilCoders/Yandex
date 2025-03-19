"""
Postgre restore Tests
"""
from datetime import datetime

from dateutil.tz import tzutc

from dbaas_internal_api.modules.postgres.pillar import PostgresqlClusterPillar
from dbaas_internal_api.modules.postgres.restore import __name__ as PACKAGE
from dbaas_internal_api.modules.postgres.restore import get_source_max_connections, restore_cluster_args
from dbaas_internal_api.modules.postgres.types import PostgresqlRestorePoint
from dbaas_internal_api.utils.types import ExistedHostResources

# pylint: disable=invalid-name


def test_restore_cluster_args():
    """
    Convert datetime to str
    """
    target_id = '12345678-1234-5678-1234-567812345678'
    restore_args = restore_cluster_args(
        target_pillar_id=target_id,
        source_cluster_id='111-222',
        s3_bucket='bucket',
        restore_point=PostgresqlRestorePoint(
            backup_id='some-backup-id',
            time=datetime(2002, 12, 25, tzinfo=tzutc()),
            time_inclusive=True,
            restore_latest=False,
        ),
        source_cluster_max_connections=333,
    )
    assert restore_args == {
        'target-pillar-id': target_id,
        's3_buckets': {
            'backup': 'bucket',
        },
        'restore-from': {
            'cid': '111-222',
            'backup-id': 'some-backup-id',
            'time': '2002-12-25T00:00:00+00:00',
            'time-inclusive': True,
            'restore-latest': False,
            'source-cluster-options': {
                'max_connections': 333,
            },
            'rename-database-from-to': {},
            'use_backup_service_at_latest_rev': False,
        },
        'use_backup_service': False,
    }


class Test_get_source_max_connections:
    """
    get_source_max_connections test
    """

    source_resources = ExistedHostResources(
        resource_preset_id='db1.test', disk_size=5555555, disk_type_id='test-disk-type'
    )

    def test_return_from_pillar_if_them_defined(self):
        """
        should return max_connections from pillar
        """
        assert (
            get_source_max_connections(
                PostgresqlClusterPillar(
                    {
                        'data': {
                            'config': {
                                'max_connections': 100500,
                                'pgusers': [],
                            },
                            'pgsync': {},
                            'pg': {},
                            'pgbouncer': {},
                        },
                    }
                ),
                source_resources=self.source_resources,
            )
            == 100500
        )

    def test_define_them_from_flavor(self, mocker):
        """
        Should get biggest flavor and calculate
        max_connections from it
        """
        mocker.patch(
            PACKAGE + '.validation.get_flavor_by_name',
            return_value={
                'id': 'db1.test',
                'cpu_guarantee': 30,
                'cpu_limit': 30,
            },
        )
        assert (
            get_source_max_connections(
                PostgresqlClusterPillar(
                    {
                        'data': {
                            'config': {
                                'pgusers': [],
                            },
                            'pgsync': {},
                            'pg': {},
                            'pgbouncer': {},
                        },
                    }
                ),
                source_resources=self.source_resources,
            )
            == 200 * 30
        )
