"""
Base utils test
"""
import pytest
from hamcrest import assert_that, contains_inanyorder

from cloud.mdb.dbaas_worker.internal.utils import get_first_key, to_host_list, to_host_map, get_image_by_major_version

# pylint: disable=invalid-name, missing-docstring


def test_get_first_key_for_empty_dict_raise_IndexError():
    with pytest.raises(IndexError):
        get_first_key({})


def test_get_first_key_return_first_key():
    assert get_first_key({1: 1, 2: 2}) == 1


def test_to_host_list():
    hosts = {
        'man-1': {
            'subcid': 'subcid1',
            'shard_id': 'shard1',
        },
        'vla-1': {
            'subcid': 'subcid1',
            'shard_id': 'shard2',
        },
    }
    assert_that(
        to_host_list(hosts),
        contains_inanyorder(
            {
                'fqdn': 'man-1',
                'subcid': 'subcid1',
                'shard_id': 'shard1',
            },
            {
                'fqdn': 'vla-1',
                'subcid': 'subcid1',
                'shard_id': 'shard2',
            },
        ),
    )


def test_to_host_map():
    hosts = [
        {
            'fqdn': 'man-1',
            'subcid': 'subcid1',
            'shard_id': 'shard1',
        },
        {
            'fqdn': 'vla-1',
            'subcid': 'subcid1',
            'shard_id': 'shard2',
        },
    ]
    assert_that(
        to_host_map(hosts),
        {
            'man-1': {
                'subcid': 'subcid1',
                'shard_id': 'shard1',
            },
            'vla-1': {
                'subcid': 'subcid1',
                'shard_id': 'shard2',
            },
        },
    )


class Test_get_image_by_major_version:  # noqa
    def test_non_versioned_image(self):
        assert get_image_by_major_version({}, 'mdb_pg.sh', {}) == 'mdb_pg.sh'

    def test_with_valid_major_version(self):
        assert (
            get_image_by_major_version(
                {
                    'template': 'mdb_pg_{major_version}.sh',
                    'task_arg': 'major_version',
                    'whitelist': {
                        '12': '12',
                    },
                },
                'mdb_pg.sh',
                {'major_version': '12'},
            )
            == 'mdb_pg_12.sh'
        )

    def test_without_major_version_in_task_args(self):
        assert (
            get_image_by_major_version(
                {
                    'template': 'mdb_pg_{major_version}.sh',
                    'task_arg': 'major_version',
                    'whitelist': {
                        '12': '12',
                    },
                },
                'mdb_pg_fallback.sh',
                {},
            )
            == 'mdb_pg_fallback.sh'
        )

    def test_with_no_whitelisted_major_version(self):
        assert (
            get_image_by_major_version(
                {
                    'template': 'mdb_pg_{major_version}.sh',
                    'task_arg': 'major_version',
                    'whitelist': {
                        '12': '12',
                    },
                },
                'mdb_pg_fallback.sh',
                {
                    'major_version': '13',
                },
            )
            == 'mdb_pg_fallback.sh'
        )

    def test_raises_when_template_require_different_argument(self):
        with pytest.raises(RuntimeError):
            get_image_by_major_version(
                {
                    'template': 'mdb_pg_{missing_arg}.sh',
                    'task_arg': 'major_version',
                    'whitelist': {
                        '12': '12',
                    },
                },
                'mdb_pg_fallback.sh',
                {
                    'major_version': '12',
                },
            )
