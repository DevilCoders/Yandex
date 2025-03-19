"""
Validation utils test
"""

from datetime import datetime
from typing import NamedTuple

import pytest
from dateutil.tz import tzutc
from marshmallow import Schema, ValidationError, post_load
from marshmallow.fields import Int, List, Nested, Str

from dbaas_internal_api.core.exceptions import (
    HostNotExistsError,
    LastHostInHaGroupError,
    MalformedReplicationChain,
    PreconditionFailedError,
    QuotaViolationHttpError,
    DiskTypeIdError,
)
from dbaas_internal_api.modules.clickhouse.constants import MY_CLUSTER_TYPE as clickhouse_cluster_type
from dbaas_internal_api.modules.clickhouse.traits import ClickhouseRoles
from dbaas_internal_api.modules.mongodb.constants import MY_CLUSTER_TYPE as mongodb_cluster_type
from dbaas_internal_api.modules.mongodb.traits import MongoDBRoles
from dbaas_internal_api.modules.mysql.constants import MY_CLUSTER_TYPE as mysql_cluster_type
from dbaas_internal_api.modules.mysql.traits import MySQLRoles
from dbaas_internal_api.modules.postgres.constants import MY_CLUSTER_TYPE as postgres_cluster_type
from dbaas_internal_api.modules.postgres.traits import PostgresqlRoles
from dbaas_internal_api.modules.redis.constants import MY_CLUSTER_TYPE as redis_cluster_type
from dbaas_internal_api.modules.redis.traits import RedisRoles
from dbaas_internal_api.utils.validation import (
    DynamicDefaultSchema,
    GrpcTimestamp,
    QuotaValidator,
    RestartSchema,
    _validate_hosts_count,
    validate_repl_source,
)

# pylint: disable=missing-docstring, invalid-name


class TestGrpcTimestamp:
    """
    GrpcTimestamp helper test
    """

    class SampleSchema(Schema):
        dt_with_tz = GrpcTimestamp(required=True)

    def test_return_datetime_with_timezone(self):
        res = self.SampleSchema().load(
            {
                'dt_with_tz': '2014-01-01T01:01:01+00',
            }
        )
        assert res.data['dt_with_tz'] == datetime(2014, 1, 1, 1, 1, 1, tzinfo=tzutc())

    def test_raise_validation_error_for_date_without_time_zone(self):
        with pytest.raises(ValidationError, match='Invalid timestamp format'):
            self.SampleSchema(strict=True).load(
                {
                    'dt_with_tz': '2014-01-01T01:01:01',
                }
            )


class SampleSchema2(RestartSchema):
    field_a = Int(restart=True)
    field_b = Int()


class SampleSchema(RestartSchema):
    field_c = Nested(SampleSchema2, many=True)
    field_d = Int(restart=True)
    field_e = Int()
    field_f = List(Int(restart=True))
    field_g = Nested(SampleSchema2)
    field_h = Int(restart=True, attribute='h')


def sample_variable_default_getter(instance_type, version, **_):
    return "{0}, {1}".format(instance_type, version)


class SampleDefaultSchema(DynamicDefaultSchema):
    class Level1Schema(DynamicDefaultSchema):
        class Level2Schema(DynamicDefaultSchema):
            class Level3Schema(DynamicDefaultSchema):
                sample_key = Str(variable_default=sample_variable_default_getter)

            level3 = Nested(Level3Schema)

        level2 = Nested(Level2Schema)

    containerized = List(Nested(Level1Schema))
    plain = Nested(Level1Schema)
    sample_key = Str(variable_default=sample_variable_default_getter)


class TestDynamicDefaultSchema:
    """
    Dynamic default schema helper test
    """

    def test_has_dynamic_default(self):
        expected = {
            'containerized': [
                {
                    'level2': {
                        'level3': {
                            'sample_key': 'instance_type, version',
                        }
                    }
                }
            ],
            'plain': {'level2': {'level3': {'sample_key': 'instance_type, version'}}},
            'sample_key': 'instance_type, version',
        }
        schema = SampleDefaultSchema(instance_type='instance_type', version='version')
        assert schema.dump({}).data == expected

    def test_valid_data_not_affected(self):
        data = {
            'containerized': [
                {
                    'level2': {
                        'level3': {
                            'sample_key': 'sample_data',
                        }
                    }
                }
            ],
            'plain': {'level2': {'level3': {'sample_key': 'sample_data'}}},
            'sample_key': 'sample_data',
        }
        data_mixed = {
            'containerized': [
                {
                    'level2': {
                        'level3': {
                            'sample_key': 'sample_data2',
                        }
                    }
                }
            ],
            'plain': {
                'level2': {
                    'level3': {
                        'sample_key': 'instance_type, version',
                    }
                }
            },
            'sample_key': 'sample_data2',
        }
        schema = SampleDefaultSchema(instance_type='instance_type', version='version')
        assert schema.dump(data).data == data
        assert schema.dump(data_mixed).data == data_mixed

    def test_dyn_defaults_not_present(self):
        data = {
            'containerized': [
                {
                    'level2': {
                        'level3': {},
                    }
                }
            ],
            'plain': {
                'level2': {
                    'level3': {},
                }
            },
            'sample_key': 'data',
        }
        schema = SampleDefaultSchema()
        assert schema.dump(data).data == data


class TestRestartSchema:
    """
    RestartSchema helper test
    """

    def test_non_restart_change(self):
        schema = SampleSchema()
        res = schema.load({'field_e': 1, 'field_c': [{'field_b': 2}]})
        assert not res[1]
        assert schema.context.get('restart') is None

    def test_top_restart_change(self):
        schema = SampleSchema()
        res = schema.load({'field_d': 1, 'field_c': [{'field_b': 2}]})
        assert not res[1]
        assert schema.context.get('restart') is True

    def test_top_restart_with_attribute_change(self):
        schema = SampleSchema()
        res = schema.load({'field_h': 1})
        assert not res[1]
        assert schema.context.get('restart') is True

    def test_deep_restart_change(self):
        schema = SampleSchema()
        res = schema.load({'field_e': 1, 'field_c': [{'field_a': 2}]})
        assert not res[1]
        assert schema.context.get('restart') is True

    def test_direct_nested_restart_change(self):
        schema = SampleSchema()
        res = schema.load({'field_e': 1, 'field_g': {'field_a': 2}})
        assert not res[1]
        assert schema.context.get('restart') is True

    def test_container_restart_change(self):
        schema = SampleSchema()
        res = schema.load({'field_e': 1, 'field_f': [2]})
        assert not res[1]
        assert schema.context.get('restart') is True


SampleObjectForLoad = NamedTuple('SampleObjectForLoad', [('req', int), ('opt', int)])


class SampleSchemaForLoad(Schema):
    req = Int(required=True)
    opt = Int(missing=0)

    @post_load
    def make_object(self, data):
        return SampleObjectForLoad(**data)


class TestQuotaValidator:
    cloud = {
        'cloud_ext_id': 'cloud1',
        'cpu_quota': 2,
        'cpu_used': 0,
        'gpu_quota': 2,
        'gpu_used': 0,
        'memory_quota': 2,
        'memory_used': 0,
        'ssd_space_quota': 2,
        'ssd_space_used': 0,
        'hdd_space_quota': 2,
        'hdd_space_used': 0,
        'clusters_quota': 1,
        'clusters_used': 0,
    }
    flavor = {
        'cpu_guarantee': 1,
        'gpu_limit': 1,
        'memory_guarantee': 1,
        'network_guarantee': 1,
        'io_limit': 1,
    }
    space_quota_map = {
        'test': 'ssd',
    }

    def test_validation_ok(self, mocker, monkeypatch):
        patched_ctx = mocker.Mock()
        monkeypatch.setattr('dbaas_internal_api.utils.validation.g', patched_ctx)
        patched_ctx.cloud = self.cloud.copy()
        patched_get_space_quota_map = mocker.patch('dbaas_internal_api.utils.validation.metadb.get_space_quota_map')
        patched_get_space_quota_map.return_value = self.space_quota_map.copy()
        validator = QuotaValidator()
        validator.add(self.flavor.copy(), node_count=1, volume_size=1, disk_type_id='test')
        validator.validate()

    def test_validation_clusters_quota_exceeded(self, mocker, monkeypatch):
        patched_ctx = mocker.Mock()
        monkeypatch.setattr('dbaas_internal_api.utils.validation.g', patched_ctx)
        non_empty_cloud = self.cloud.copy()
        mocker.patch('dbaas_internal_api.utils.validation.abort')
        patched_get_space_quota_map = mocker.patch('dbaas_internal_api.utils.validation.metadb.get_space_quota_map')
        patched_get_space_quota_map.return_value = self.space_quota_map.copy()
        for key in non_empty_cloud:
            if key.endswith('_used'):
                non_empty_cloud[key] = 1
        patched_ctx.cloud = non_empty_cloud
        validator = QuotaValidator()
        validator.add(self.flavor.copy(), node_count=1, volume_size=1, disk_type_id='test')
        with pytest.raises(QuotaViolationHttpError, match='Quota limits exceeded, not enough clusters: 1 clusters'):
            validator.validate()

    def test_validation_resources_quota_exceeded(self, mocker, monkeypatch):
        patched_ctx = mocker.Mock()
        monkeypatch.setattr('dbaas_internal_api.utils.validation.g', patched_ctx)
        non_empty_cloud = self.cloud.copy()
        mocker.patch('dbaas_internal_api.utils.validation.abort')
        patched_get_space_quota_map = mocker.patch('dbaas_internal_api.utils.validation.metadb.get_space_quota_map')
        patched_get_space_quota_map.return_value = self.space_quota_map.copy()
        for key in non_empty_cloud:
            if key.endswith('_used') and key != 'clusters_used':
                non_empty_cloud[key] = 2
        patched_ctx.cloud = non_empty_cloud
        validator = QuotaValidator()
        validator.add(self.flavor.copy(), node_count=1, volume_size=1, disk_type_id='test')
        with pytest.raises(
            QuotaViolationHttpError,
            match='Quota limits exceeded, not enough cpu: 1 cores, gpu: 1 cards, ' 'memory: 1 byte, ssd_space: 1 byte',
        ):
            validator.validate()

    def test_multiple_add(self, mocker, monkeypatch):
        patched_ctx = mocker.Mock()
        monkeypatch.setattr('dbaas_internal_api.utils.validation.g', patched_ctx)
        patched_ctx.cloud = self.cloud.copy()
        mocker.patch('dbaas_internal_api.utils.validation.abort')
        patched_get_space_quota_map = mocker.patch('dbaas_internal_api.utils.validation.metadb.get_space_quota_map')
        patched_get_space_quota_map.return_value = self.space_quota_map.copy()
        validator = QuotaValidator()
        for _ in range(3):
            validator.add(self.flavor.copy(), node_count=1, volume_size=1, disk_type_id='test')
        with pytest.raises(
            QuotaViolationHttpError,
            match='Quota limits exceeded, not enough cpu: 1 cores, gpu: 1 cards, ' 'memory: 1 byte, ssd_space: 1 byte',
        ):
            validator.validate()

    def test_add_with_incorrect_disk_type_id(self, mocker, monkeypatch):
        patched_ctx = mocker.Mock()
        monkeypatch.setattr('dbaas_internal_api.utils.validation.g', patched_ctx)
        patched_ctx.cloud = self.cloud.copy()
        patched_get_space_quota_map = mocker.patch('dbaas_internal_api.utils.validation.metadb.get_space_quota_map')
        patched_get_space_quota_map.return_value = self.space_quota_map.copy()
        validator = QuotaValidator()
        with pytest.raises(DiskTypeIdError, match=r"Disk type with 'test_not_in_map' not found in \['test'\]"):
            validator.add(self.flavor.copy(), node_count=1, volume_size=1, disk_type_id='test_not_in_map')


class TestValidateHostsCount:
    @pytest.mark.parametrize(
        'cluster_type,role',
        [
            (clickhouse_cluster_type, ClickhouseRoles.clickhouse),
            (clickhouse_cluster_type, ClickhouseRoles.zookeeper),
            (mongodb_cluster_type, MongoDBRoles.mongod),
            (mongodb_cluster_type, MongoDBRoles.mongos),
            (mongodb_cluster_type, MongoDBRoles.mongocfg),
            (postgres_cluster_type, PostgresqlRoles.postgresql),
            (redis_cluster_type, RedisRoles.redis),
            (mysql_cluster_type, MySQLRoles.mysql),
        ],
    )
    def test_valid(self, cluster_type, role):
        for i in range(1, 8):
            _validate_hosts_count(1, 7, cluster_type, role, 'any resourcePreset', 'any diskTypeId', i)

    @pytest.mark.parametrize(
        'cluster_type,role,human_role,entity',
        [
            (clickhouse_cluster_type, ClickhouseRoles.clickhouse.value, 'ClickHouse', 'shard'),
            (clickhouse_cluster_type, ClickhouseRoles.zookeeper.value, 'Zookeeper', 'subcluster'),
            (mongodb_cluster_type, MongoDBRoles.mongod.value, 'Mongod', 'shard'),
            (mongodb_cluster_type, MongoDBRoles.mongos.value, 'Mongos', 'subcluster'),
            (mongodb_cluster_type, MongoDBRoles.mongocfg.value, 'Mongocfg', 'subcluster'),
            (postgres_cluster_type, PostgresqlRoles.postgresql.value, 'PostgreSQL', 'cluster'),
            (redis_cluster_type, RedisRoles.redis.value, 'Redis', 'shard'),
            (mysql_cluster_type, MySQLRoles.mysql.value, 'MySQL', 'cluster'),
        ],
    )
    def test_invalid(self, cluster_type, role, human_role, entity):
        with pytest.raises(
            PreconditionFailedError,
            match="{role} {entity} with resource preset 's1.compute.1' and disk type 'network-ssd' "
            "allows at most 7 hosts".format(role=human_role, entity=entity),
        ):
            _validate_hosts_count(1, 7, cluster_type, role, 's1.compute.1', 'network-ssd', 8)
        with pytest.raises(
            PreconditionFailedError,
            match="{role} {entity} with resource preset 's1.compute.1' and disk type 'local-nvme' "
            "requires at least 3 hosts".format(role=human_role, entity=entity),
        ):
            _validate_hosts_count(3, 7, cluster_type, role, 's1.compute.1', 'local-nvme', 1)


class TestReplSourceValidation:
    @pytest.mark.parametrize(
        'hosts_repl_sources,host_config',
        [
            [{'a': None, 'b': None, 'c': None}, {'d': 'c'}],
            [{'a': None, 'b': None, 'c': None, 'd': 'c'}, {'d': 'a'}],
            [{'a': None, 'b': None, 'c': None}, {'d': 'c', 'e': 'd'}],
            [
                {
                    'a': None,
                },
                {
                    None: 'a',
                },
            ],
        ],
    )
    def test_normal_case(self, hosts_repl_sources, host_config):
        assert validate_repl_source(hosts_repl_sources, host_config)

    @pytest.mark.parametrize(
        'hosts_repl_sources,host_config',
        [
            [{'a': None, 'b': None, 'c': None}, {'c': None}],
            [{'a': None, 'b': None, 'c': None, 'd': 'c'}, {'d': 'c'}],
            [{'a': None, 'b': None, 'c': None}, {}],
        ],
    )
    def test_no_change(self, hosts_repl_sources, host_config):
        assert not validate_repl_source(hosts_repl_sources, host_config)

    @pytest.mark.parametrize(
        'hosts_repl_sources,host_config',
        [
            [{'a': None, 'b': None, 'c': None}, {'c': 'c'}],
            [{'a': None, 'b': None, 'c': None, 'd': 'c', 'e': 'd'}, {'c': 'e'}],
        ],
    )
    def test_repl_chain_loop(self, hosts_repl_sources, host_config):
        with pytest.raises(MalformedReplicationChain):
            validate_repl_source(hosts_repl_sources, host_config)

    @pytest.mark.parametrize(
        'hosts_repl_sources,host_config',
        [
            [{'a': None}, {'a': 'a'}],
        ],
    )
    def test_last_ha_host_in_cluster(self, hosts_repl_sources, host_config):
        with pytest.raises(LastHostInHaGroupError):
            validate_repl_source(hosts_repl_sources, host_config)

    @pytest.mark.parametrize(
        'hosts_repl_sources,host_config',
        [
            [{'a': None, 'b': None}, {'b': 'x'}],
            [
                {
                    'a': None,
                },
                {'b': 'b'},
            ],
        ],
    )
    def test_host_not_found(self, hosts_repl_sources, host_config):
        with pytest.raises(HostNotExistsError):
            validate_repl_source(hosts_repl_sources, host_config)
