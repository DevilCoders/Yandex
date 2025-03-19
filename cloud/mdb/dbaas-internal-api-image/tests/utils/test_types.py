"""
Test for types
"""

import pytest

from dbaas_internal_api.utils.types import (
    ComparableEnum,
    ExistedHostResources,
    NestedConfigSpec,
    PlainConfigSpec,
    RequestedHostResources,
    ClusterStatus,
)

from .. import mocks


class PlainTestConfigSpec(PlainConfigSpec):
    cluster_type = 'postgresql_cluster'
    config_key_prefix = 'postgresql_config'


class NestedTestConfigSpec(NestedConfigSpec):
    cluster_type = 'mongodb_cluster'
    config_key_prefix = 'mongodb_spec'
    valid_roles = ['mongod', 'mongos']


class FoundationDBTask(ComparableEnum):
    """
    FoundationDBTask
    """

    create = 'foundationdb_create'
    modify = 'foundationdb_modify'
    delete = 'foundationdb_delete'


class RedisTask(ComparableEnum):
    """
    Redis tasks
    """

    create = 'redis_create'


def mock_valid_versions(mocker):
    mocks.mock_valid_versions('dbaas_internal_api.utils.types', mocker)


class TestComparableEnum:
    """
    Test for TaskType enum
    """

    def test_eq_enum(self):
        assert FoundationDBTask.create == FoundationDBTask.create

    def test_not_eq_enum(self):
        assert FoundationDBTask.create != FoundationDBTask.delete

    def test_eq_str(self):
        assert FoundationDBTask.create == 'foundationdb_create'

    def test_not_eq_str(self):
        assert FoundationDBTask.create != 'mongodb_create'

    def test_raises_type_error_for_other_enums(self):
        with pytest.raises(TypeError):
            FoundationDBTask.create == RedisTask.create

    def test_from_string(self):
        assert FoundationDBTask.from_string('foundationdb_create') is FoundationDBTask.create

    def test_from_string_for_not_existed_value(self):
        with pytest.raises(KeyError):
            FoundationDBTask.from_string('postgresql_create')

    def test_hash(self):
        assert (
            hash(FoundationDBTask.create)
            == hash(FoundationDBTask.from_string('foundationdb_create'))
            == hash(FoundationDBTask['create'])
        )


class TestRequestedHostResources:
    def test_init(self):
        resources = RequestedHostResources(resource_preset_id='db1.nano', disk_size=1024, disk_type_id='str')
        assert resources.resource_preset_id == 'db1.nano'
        assert resources.disk_size == 1024
        assert resources.disk_type_id == 'str'

    @pytest.mark.parametrize(
        ['resource_preset_id', 'disk_size', 'disk_type_id', 'result'],
        [
            ('db1.nano', 1024, 'str', True),
            ('', 0, '', False),
            (None, None, None, False),
        ],
    )
    def test_bool_op(self, resource_preset_id, disk_size, disk_type_id, result):
        resources = RequestedHostResources(
            resource_preset_id=resource_preset_id, disk_size=disk_size, disk_type_id=disk_type_id
        )
        assert bool(resources) == result


class TestExistedHostResources:
    def test_init(self):
        resources = ExistedHostResources(resource_preset_id='db1.nano', disk_size=1024, disk_type_id='str')
        assert resources.resource_preset_id == 'db1.nano'
        assert resources.disk_size == 1024
        assert resources.disk_type_id == 'str'

    @pytest.mark.parametrize(
        ['resource_preset_id', 'disk_size', 'disk_type_id'],
        [
            ('db1.small', 10240, 'str2'),
            ('db1.small', None, None),
            (None, 10240, None),
            (None, None, 'str2'),
            (None, None, None),
        ],
    )
    def test_update(self, resource_preset_id, disk_size, disk_type_id):
        expected = ExistedHostResources(
            resource_preset_id=resource_preset_id or 'db1.nano',
            disk_size=disk_size or 1024,
            disk_type_id=disk_type_id or 'str',
        )

        actual = ExistedHostResources(resource_preset_id='db1.nano', disk_size=1024, disk_type_id='str')
        actual.update(
            RequestedHostResources(
                resource_preset_id=resource_preset_id, disk_size=disk_size, disk_type_id=disk_type_id
            )
        )
        assert expected == actual


class TestPlainConfigSpec:
    """
    Test for plain config spec wrapper
    """

    config_spec = {
        'postgresql_config_10': {'pg': 'config'},
        'pooler_config': {'pooler': 'config'},
        'resources': {
            'resource_preset_id': 'db1.nano',
            'disk_size': '1024',
            'disk_type_id': 'str',
        },
    }

    bad_ver_config_spec = {
        'postgresql_config_10': {'pg': 'config'},
        'pooler_config': {'pooler': 'config'},
        'resources': {
            'resource_preset_id': 'db1.nano',
            'disk_size': '1024',
            'disk_type_id': 'str',
        },
        'version': '11',
    }
    no_resources = {
        'postgresql_config_10': {'pg': 'config'},
        'pooler_config': {'pooler': 'config'},
    }
    empty_config = {
        'version': '11',
    }

    @pytest.fixture
    def config(self, mocker) -> PlainTestConfigSpec:
        mock_valid_versions(mocker)
        return PlainTestConfigSpec(self.config_spec)

    @pytest.fixture
    def config_no_resources(self, mocker) -> PlainTestConfigSpec:
        mock_valid_versions(mocker)
        return PlainTestConfigSpec(self.no_resources)

    @pytest.fixture
    def config_empty(self, mocker) -> PlainTestConfigSpec:
        mock_valid_versions(mocker)
        return PlainTestConfigSpec(self.empty_config)

    def test_resources(self, config: PlainTestConfigSpec):
        assert config.get_resources() == RequestedHostResources(
            resource_preset_id='db1.nano', disk_size=1024, disk_type_id='str'
        )

    def test_config_pg(self, config: PlainTestConfigSpec):
        assert config.get_config() == {'pg': 'config'}

    def test_empty_config_pg(self, config_empty: PlainTestConfigSpec):
        assert config_empty.get_config() == {}

    def test_config_pooler(self, config: PlainTestConfigSpec):
        assert config.get_config('pooler_config') == {'pooler': 'config'}

    def test_has_key(self, config: PlainTestConfigSpec):
        assert config.has_config('nonexistent') is False

    def test_has_config(self, config: PlainTestConfigSpec):
        assert config.has_config()

    def test_has_resources(self, config: PlainTestConfigSpec):
        assert config.has_resources()

    def test_does_not_have_resources(self, config_no_resources: PlainTestConfigSpec):
        assert not config_no_resources.has_resources()


class TestNestedConfigSpec:
    """
    Test for plain config spec wrapper
    """

    config_spec = {
        'mongodb_spec_3_6': {
            'mongod': {
                'config': {'mongod': 'config'},
                'resources': {
                    'resource_preset_id': 'db1.nano',
                    'disk_size': '1024',
                    'disk_type_id': 'str',
                },
            },
            'mongos': {
                'config': {'mongos': 'config'},
                'resources': {
                    'resource_preset_id': 'db1.nano',
                    'disk_size': '1024',
                    'disk_type_id': 'str',
                },
            },
        }
    }
    no_resources = {
        'mongodb_spec_3_6': {
            'mongod': {
                'config': {'mongod': 'config'},
            },
            'mongos': {
                'config': {'mongos': 'config'},
            },
        }
    }

    @pytest.fixture
    def config(self, mocker) -> NestedTestConfigSpec:
        mock_valid_versions(mocker)
        return NestedTestConfigSpec(self.config_spec)

    @pytest.fixture
    def config_no_resources(self, mocker) -> NestedTestConfigSpec:
        mock_valid_versions(mocker)
        return NestedTestConfigSpec(self.no_resources)

    def test_resources(self, config: NestedTestConfigSpec):
        assert config.get_resources('mongod') == RequestedHostResources(
            resource_preset_id='db1.nano', disk_size=1024, disk_type_id='str'
        )

    def test_config_mongod(self, config: NestedTestConfigSpec):
        assert config.get_config('mongod') == {'mongod': 'config'}

    def test_config_mongos(self, config: NestedTestConfigSpec):
        assert config.get_config('mongos') == {'mongos': 'config'}

    def test_config_whatever(self, config: NestedTestConfigSpec):
        with pytest.raises(KeyError):
            config.get_config('whatever')

    def test_has_key(self, config: NestedTestConfigSpec):
        assert config.has_config('nonexistent') is False

    def test_has_config(self, config: NestedTestConfigSpec):
        assert config.has_config(role='mongod')

    def test_has_resources(self, config: NestedTestConfigSpec):
        assert config.has_resources('mongod')

    def test_wrong_role_no_resources(self, config_no_resources: NestedTestConfigSpec):
        assert not config_no_resources.has_resources('nonexistent')

    def test_does_not_have_resources(self, config_no_resources: NestedTestConfigSpec):
        assert not config_no_resources.has_resources('mongod')


class TestConfigSpecModification:
    """
    Test for plain config spec modification
    """

    config_spec = {
        'postgresql_config_10': {'pg': 'config'},
        'pooler_config': {'pooler': 'config'},
        'resources': {
            'resource_preset_id': 'db1.nano',
            'disk_size': '1024',
            'disk_type_id': 'str',
        },
    }

    @pytest.fixture
    def config(self, mocker) -> PlainTestConfigSpec:
        mock_valid_versions(mocker)
        return PlainTestConfigSpec(self.config_spec)

    def test_update_existing_value(self, config: PlainTestConfigSpec):
        config.set_config(['pooler_config', 'pooler'], 'odyssey')
        assert config.get_config('pooler_config') == {'pooler': 'odyssey'}

    def test_add_new_value_to_existing_object(self, config: PlainTestConfigSpec):
        config.set_config(['pooler_config', 'odyssey_cfg'], {'port': 123})
        assert config.get_config('pooler_config') == {'pooler': 'odyssey', 'odyssey_cfg': {'port': 123}}

    def test_add_new_value_to_new_path(self, config: PlainTestConfigSpec):
        config.set_config(['security_group_ids'], [10, 20, 30])
        assert config.get_config('security_group_ids') == [10, 20, 30]


CLUSTER_DELETING_STATUSES = [
    ClusterStatus.deleting,
    ClusterStatus.deleted,
    ClusterStatus.delete_error,
    ClusterStatus.metadata_deleting,
    ClusterStatus.metadata_deleted,
    ClusterStatus.metadata_delete_error,
    ClusterStatus.purging,
    ClusterStatus.purged,
    ClusterStatus.purge_error,
]


class TestClusterStatus:
    @pytest.mark.parametrize(
        'enum_value',
        CLUSTER_DELETING_STATUSES,
    )
    def test_is_deleting(self, enum_value):
        assert enum_value.is_deleting()

    @pytest.mark.parametrize(
        'enum_value',
        [e for e in ClusterStatus if e not in CLUSTER_DELETING_STATUSES],
    )
    def test_not_deleting(self, enum_value):
        assert not enum_value.is_deleting()
