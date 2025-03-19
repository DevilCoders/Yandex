"""
Schemas for Redis configs.
"""

from marshmallow.fields import Int, Nested
from marshmallow.validate import Range

from ....apis.schemas.cluster import ClusterConfigSchemaV1, ClusterConfigSpecSchemaV1, ConfigSchemaV1
from ....apis.schemas.fields import MappedEnum, Str
from ....apis.schemas.objects import AccessSchemaV1
from ....utils.register import register_config_schema
from ....utils.validation import Schema, TimeOfDay
from ..constants import MY_CLUSTER_TYPE
from ..traits import MemoryUnits, NotifyKeyspaceEvents, PersistenceModes, RedisClusterTraits
from .resources import RedisResourcesSchemaV1, RedisResourcesUpdateSchemaV1


INT_MAX = 2147483647
LONG_MAX = 9223372036854775807
SLOWLOG_MIN_TIME = 10  # ms, temporary fix, see MDB-14070
DATABASES_MIN = 1
DATABASES_MAX = 1024


class MaxMemoryPolicy(MappedEnum):
    """
    maxmemory-policy map
    """

    def __init__(self, **kwargs):
        super().__init__(
            {
                'VOLATILE_LRU': 'volatile-lru',
                'ALLKEYS_LRU': 'allkeys-lru',
                'VOLATILE_LFU': 'volatile-lfu',
                'ALLKEYS_LFU': 'allkeys-lfu',
                'VOLATILE_RANDOM': 'volatile-random',
                'ALLKEYS_RANDOM': 'allkeys-random',
                'VOLATILE_TTL': 'volatile-ttl',
                'NOEVICTION': 'noeviction',
            },
            **kwargs
        )


class PersistenceMode(MappedEnum):
    """
    persistence mode map
    """

    def __init__(self, **kwargs):
        super().__init__(
            {
                'ON': PersistenceModes.on.value,
                'OFF': PersistenceModes.off.value,
            },
            **kwargs
        )


class MemoryUnit(MappedEnum):
    """
    memory unit map
    """

    MAPPING = {
        'BYTES': MemoryUnits.bytes.value,
        'KILOBYTES': MemoryUnits.kilobytes.value,
        'MEGABYTES': MemoryUnits.megabytes.value,
        'GIGABYTES': MemoryUnits.gigabytes.value,
    }

    def __init__(self, **kwargs):
        super().__init__(self.MAPPING, **kwargs)


class ClientOutputBufferLimit(ConfigSchemaV1):
    """
    Redis client-output-buffer-limit schema.
    """

    hardLimit = Int(validate=Range(min=0, max=LONG_MAX), attribute='hard_limit')
    hardLimitUnit = MemoryUnit(attribute='hard_limit_unit', missing='BYTES')
    softLimit = Int(validate=Range(min=0, max=LONG_MAX), attribute='soft_limit')
    softLimitUnit = MemoryUnit(attribute='soft_limit_unit', missing='BYTES')
    softSeconds = Int(validate=Range(min=0, max=LONG_MAX), attribute='soft_seconds')


class BaseConfigSchemaV1(ConfigSchemaV1):
    """
    Redis common config schema.
    """

    databases = Int(validate=Range(min=DATABASES_MIN, max=DATABASES_MAX), restart=True)
    maxmemoryPolicy = MaxMemoryPolicy(attribute='maxmemory-policy')
    slowlogLogSlowerThan = Int(validate=Range(min=SLOWLOG_MIN_TIME, max=LONG_MAX), attribute='slowlog-log-slower-than')
    slowlogMaxLen = Int(validate=Range(min=0, max=LONG_MAX), attribute='slowlog-max-len')
    timeout = Int(validate=Range(min=0, max=INT_MAX))
    clientOutputBufferLimitNormal = Nested(ClientOutputBufferLimit, attribute='client-output-buffer-limit-normal')
    clientOutputBufferLimitPubsub = Nested(ClientOutputBufferLimit, attribute='client-output-buffer-limit-pubsub')


class RedisConfigSchemaV1(BaseConfigSchemaV1):
    """
    Redis create config schema.
    """

    password = Str(validate=RedisClusterTraits.password.validate, load_only=True, required=True)


class RedisConfigUpdateSchemaV1(BaseConfigSchemaV1):
    """
    Redis update config schema.
    """

    password = Str(validate=RedisClusterTraits.password.validate, load_only=True)


# version 5.0
class Redis50Mixin:
    """
    Redis config 5.0 specific mixin
    """

    notifyKeyspaceEvents = Str(
        validate=NotifyKeyspaceEvents(allowed_symbols="KEg$lshzxeAt").validate, attribute='notify-keyspace-events'
    )


@register_config_schema(MY_CLUSTER_TYPE, '5.0')
class Redis50ConfigSchemaV1(Redis50Mixin, RedisConfigSchemaV1):
    """
    Redis config schema (version 5.0)
    """


class Redis50ConfigUpdateSchemaV1(Redis50Mixin, RedisConfigUpdateSchemaV1):
    """
    Redis config update schema (version 5.0)
    """


class Redis50ConfigSetSchemaV1(Schema):
    """
    Redis config set schema.
    """

    effectiveConfig = Nested(Redis50ConfigSchemaV1, attribute='effective_config', required=True)
    userConfig = Nested(Redis50ConfigSchemaV1, attribute='user_config')
    defaultConfig = Nested(Redis50ConfigSchemaV1, attribute='default_config')


# version 6.0
class Redis60Mixin:
    """
    Redis config 6.0 specific mixin
    """

    notifyKeyspaceEvents = Str(
        validate=NotifyKeyspaceEvents(allowed_symbols="KEg$lshzxeAtm").validate, attribute='notify-keyspace-events'
    )


@register_config_schema(MY_CLUSTER_TYPE, '6.0')
class Redis60ConfigSchemaV1(Redis60Mixin, RedisConfigSchemaV1):
    """
    Redis config schema (version 6.0)
    """


class Redis60ConfigUpdateSchemaV1(Redis60Mixin, RedisConfigUpdateSchemaV1):
    """
    Redis config update schema (version 6.0)
    """


class Redis60ConfigSetSchemaV1(Schema):
    """
    Redis config set schema.
    """

    effectiveConfig = Nested(Redis60ConfigSchemaV1, attribute='effective_config', required=True)
    userConfig = Nested(Redis60ConfigSchemaV1, attribute='user_config')
    defaultConfig = Nested(Redis60ConfigSchemaV1, attribute='default_config')


# version 6.2
class Redis62Mixin:
    """
    Redis config 6.2 specific mixin
    """

    notifyKeyspaceEvents = Str(
        validate=NotifyKeyspaceEvents(allowed_symbols="KEg$lshzxeAtmd").validate, attribute='notify-keyspace-events'
    )


@register_config_schema(MY_CLUSTER_TYPE, '6.2', feature_flag='MDB_REDIS_62')
class Redis62ConfigSchemaV1(Redis62Mixin, RedisConfigSchemaV1):
    """
    Redis config schema (version 6.2)
    """


class Redis62ConfigUpdateSchemaV1(Redis62Mixin, RedisConfigUpdateSchemaV1):
    """
    Redis config update schema (version 6.2)
    """


class Redis62ConfigSetSchemaV1(Schema):
    """
    Redis config set schema.
    """

    effectiveConfig = Nested(Redis62ConfigSchemaV1, attribute='effective_config', required=True)
    userConfig = Nested(Redis62ConfigSchemaV1, attribute='user_config')
    defaultConfig = Nested(Redis62ConfigSchemaV1, attribute='default_config')


# version 7.0
class Redis70Mixin:
    """
    Redis config 7.0 specific mixin
    """

    notifyKeyspaceEvents = Str(
        validate=NotifyKeyspaceEvents(allowed_symbols="KEg$lshzxeAtmnd").validate, attribute='notify-keyspace-events'
    )


@register_config_schema(MY_CLUSTER_TYPE, '7.0', feature_flag='MDB_REDIS_70')
class Redis70ConfigSchemaV1(Redis70Mixin, RedisConfigSchemaV1):
    """
    Redis config schema (version 7.0)
    """


class Redis70ConfigUpdateSchemaV1(Redis70Mixin, RedisConfigUpdateSchemaV1):
    """
    Redis config update schema (version 7.0)
    """


class Redis70ConfigSetSchemaV1(Schema):
    """
    Redis config set schema.
    """

    effectiveConfig = Nested(Redis70ConfigSchemaV1, attribute='effective_config', required=True)
    userConfig = Nested(Redis70ConfigSchemaV1, attribute='user_config')
    defaultConfig = Nested(Redis70ConfigSchemaV1, attribute='default_config')


# common
class ClusterConfigMixin:
    backupWindowStart = Nested(TimeOfDay, attribute='backup_window_start')
    access = Nested(AccessSchemaV1, attribute='access')


class RedisClusterConfigSchemaV1(ClusterConfigMixin, ClusterConfigSchemaV1):
    """
    Redis cluster config schema (response).
    """

    redisConfig_5_0 = Nested(Redis50ConfigSetSchemaV1, attribute='redis_config_5_0')
    redisConfig_6_0 = Nested(Redis60ConfigSetSchemaV1, attribute='redis_config_6_0')
    redisConfig_6_2 = Nested(Redis62ConfigSetSchemaV1, attribute='redis_config_6_2')
    redisConfig_7_0 = Nested(Redis70ConfigSetSchemaV1, attribute='redis_config_7_0')
    resources = Nested(RedisResourcesSchemaV1)


class RedisClusterConfigSpecCreateSchemaV1(ClusterConfigMixin, ClusterConfigSpecSchemaV1):
    """
    Redis cluster config spec schema (request).
    """

    redisConfig_5_0 = Nested(Redis50ConfigSchemaV1, attribute='redis_config_5_0')
    redisConfig_6_0 = Nested(Redis60ConfigSchemaV1, attribute='redis_config_6_0')
    redisConfig_6_2 = Nested(Redis62ConfigSchemaV1, attribute='redis_config_6_2')
    redisConfig_7_0 = Nested(Redis70ConfigSchemaV1, attribute='redis_config_7_0')
    resources = Nested(RedisResourcesSchemaV1)


class RedisClusterConfigSpecUpdateSchemaV1(ClusterConfigMixin, ClusterConfigSpecSchemaV1):
    """
    Redis cluster config spec schema (request).
    """

    redisConfig_5_0 = Nested(Redis50ConfigUpdateSchemaV1, attribute='redis_config_5_0')
    redisConfig_6_0 = Nested(Redis60ConfigUpdateSchemaV1, attribute='redis_config_6_0')
    redisConfig_6_2 = Nested(Redis62ConfigUpdateSchemaV1, attribute='redis_config_6_2')
    redisConfig_7_0 = Nested(Redis70ConfigUpdateSchemaV1, attribute='redis_config_7_0')
    resources = Nested(RedisResourcesUpdateSchemaV1)
