"""
Redis pillar wrapper
"""
from copy import deepcopy
from typing import List, Optional

from ...core.base_pillar import BackupAndAccessPillar
from ...core.crypto import encrypt, gen_encrypted_password
from ...utils.version import Version
from .constants import MAX_SHARDS_COUNT
from .traits import PersistenceModes
from .utils import (
    calculate_client_output_buffer_limit,
    calculate_maxmemory,
    calculate_repl_backlog_size,
    combine_client_output_buffer_limit,
    split_buffer_limits,
)


class RedisPillar(BackupAndAccessPillar):
    """
    Redis pillar
    """

    BLOCKED_COMMANDS = [
        'BGREWRITEAOF',
        'BGSAVE',
        'CONFIG',
        'DEBUG',
        'LASTSAVE',
        'MIGRATE',
        'MODULE',
        'MONITOR',
        'MOVE',
        'OBJECT',
        'REPLICAOF',
        'SAVE',
        'SHUTDOWN',
        'SLAVEOF',
    ]

    BLOCKED_COMMANDS_60 = BLOCKED_COMMANDS + ['ACL']

    BLOCKED_SENTINEL_COMMANDS = [
        'FAILOVER',
        'RESET',
    ]

    BLOCKED_SENTINEL_COMMANDS_62 = BLOCKED_SENTINEL_COMMANDS + ['CONFIG-SET']

    BLOCKED_SENTINEL_COMMANDS_70 = BLOCKED_SENTINEL_COMMANDS_62 + ['CONFIG-GET']

    BLOCKED_SENTINEL_DIRECT_COMMANDS = [
        'ACL',
    ]

    BLOCKED_CLUSTER_COMMANDS = [
        'ADDSLOTS',
        'DELSLOTS',
        'FAILOVER',
        'FORGET',
        'MEET',
        'REPLICATE',
        'RESET',
        'SAVECONFIG',
        'SET-CONFIG-EPOCH',
        'SETSLOT',
        'FLUSHSLOTS',
        'BUMPEPOCH',
    ]

    def __init__(self, pillar: dict = None) -> None:
        self._pillar = (
            pillar
            if pillar is not None
            else {
                'data': {
                    'redis': {
                        'config': {},
                    },
                },
            }
        )

    def update_config(self, config: dict) -> None:
        """
        Update Redis config.
        Incoming 'config' is expected to be validated by
        Redis config schema.
        """
        password = config.pop('password', None)
        if password:
            config.update(
                {
                    'requirepass': encrypt(password),
                    'masterauth': encrypt(password),
                }
            )
        self._redis['config'].update(config)

    def update_client_buffers(self, client_output_limit_buffer_normal, client_output_limit_buffer_pubsub) -> bool:
        """
        Update Redis config with client-output-buffer-limits.
        :return:
        """
        config = self._redis['config']
        changed = False

        if client_output_limit_buffer_normal:
            normal_before = config['client-output-buffer-limit normal']
            normal_str = combine_client_output_buffer_limit(normal_before, client_output_limit_buffer_normal)
            if normal_before != normal_str:
                config['client-output-buffer-limit normal'] = normal_str
                changed = True
        if client_output_limit_buffer_pubsub:
            pubsub_before = config['client-output-buffer-limit pubsub']
            pubsub_str = combine_client_output_buffer_limit(pubsub_before, client_output_limit_buffer_pubsub)
            if pubsub_before != pubsub_str:
                config['client-output-buffer-limit pubsub'] = pubsub_str
                changed = True
        return changed

    def set_replica_priority(self, priority: int) -> None:
        """
        Set replica priority.
        """
        self._redis['config']['replica-priority'] = priority

    def get_replica_priority(self) -> Optional[int]:
        """
        Get replica priority.
        """
        return self._redis.get('config', {}).get('replica-priority')

    def set_maxmemory(self, maxmemory: int) -> None:
        """
        Set maxmemory value.
        """
        self._redis['config']['maxmemory'] = maxmemory

    def set_cluster_mode(self, sharded: bool) -> None:
        """
        Enable/disable cluster mode.
        """
        self._redis['config']['cluster-enabled'] = 'yes' if sharded else 'no'

    def is_cluster_enabled(self) -> bool:
        """
        Returns True if cluster mode is enabled.
        """
        return self._redis['config'].get('cluster-enabled') == 'yes'

    def set_repl_backlog_size(self, size: int) -> None:
        """
        Set repl-backlog-size value.
        """
        self._redis['config']['repl-backlog-size'] = size

    def set_client_output_buffer_limits(self, soft_limit: int, repl_backlog_size: int, soft_timeout: int = 60) -> None:
        """
        Set client-output-buffer-limit value.
        """
        buffer_types = ['normal', 'pubsub']
        for buffer_type in buffer_types:
            self._redis['config'][
                f'client-output-buffer-limit {buffer_type}'
            ] = f'{soft_limit * 2} {soft_limit} {soft_timeout}'
        repl_soft_limit = min(repl_backlog_size, max(soft_limit, repl_backlog_size // 2))
        self._redis['config'][
            'client-output-buffer-limit replica'
        ] = f'{repl_backlog_size} {repl_soft_limit} {soft_timeout}'

    def set_flavor_dependent_options(self, flavor: dict) -> None:
        """
        Set all the options dependent on machine's resources.
        """
        self.set_maxmemory(calculate_maxmemory(flavor))
        backlog_size = calculate_repl_backlog_size(flavor)
        self.set_repl_backlog_size(backlog_size)
        soft_limit = calculate_client_output_buffer_limit(flavor)
        self.set_client_output_buffer_limits(soft_limit, backlog_size)

    def generate_renames(self, sharded: bool = False, version: Version = None) -> None:
        """
        Add command renames.
        """

        def renames(commands, hashlen=40):
            return {cmd: gen_encrypted_password(hashlen) for cmd in commands}

        secrets = {
            'renames': renames(self.get_blocked_commands(version)),
        }
        if sharded:
            secrets.update({'cluster_renames': renames(self.BLOCKED_CLUSTER_COMMANDS)})
        else:
            sentinel_commands = []
            if version == Version(5, 0) or version == Version(6, 0):
                sentinel_commands = self.BLOCKED_SENTINEL_COMMANDS
            elif version == Version(6, 2):
                sentinel_commands = self.BLOCKED_SENTINEL_COMMANDS_62
            elif version == Version(7, 0):
                sentinel_commands = self.BLOCKED_SENTINEL_COMMANDS_70
            secrets.update({'sentinel_renames': renames(sentinel_commands)})
            if not (version == Version(5, 0) or version == Version(6, 0)):
                secrets.update({'sentinel_direct_renames': renames(self.BLOCKED_SENTINEL_DIRECT_COMMANDS)})
        self._redis['secrets'] = secrets

    def get_blocked_commands(self, version):
        return self.BLOCKED_COMMANDS if version == Version(5, 0) else self.BLOCKED_COMMANDS_60

    @property
    def config(self) -> dict:
        """
        Get Redis config.
        """
        return deepcopy(self._redis['config'])

    def get_info_config(self):
        def backport_limits(limits):
            hard_limit, soft_limit, soft_seconds = split_buffer_limits(limits)
            res = {
                "hard_limit": hard_limit,
                "hard_limit_unit": "b",
                "soft_limit": soft_limit,
                "soft_limit_unit": "b",
                "soft_seconds": soft_seconds,
            }
            return res

        config = self.config
        config['client-output-buffer-limit-normal'] = backport_limits(config['client-output-buffer-limit normal'])
        config['client-output-buffer-limit-pubsub'] = backport_limits(config['client-output-buffer-limit pubsub'])
        return config

    @property
    def zk_hosts(self) -> List[str]:
        """
        List of zookeeper hosts fqdns.
        """
        return self._redis['zk_hosts']

    @zk_hosts.setter
    def zk_hosts(self, hosts: List[str]) -> None:
        self._redis['zk_hosts'] = hosts

    @property
    def tls_enabled(self) -> bool:
        """
        Get tls mode.
        """
        return self._redis.get('tls', {}).get('enabled', False)

    @tls_enabled.setter
    def tls_enabled(self, enabled: bool) -> None:
        """
        Set Redis tls mode.
        """
        if 'tls' not in self._redis:
            self._redis['tls'] = {}
        self._redis['tls']['enabled'] = enabled

    @property
    def _redis(self):
        if not self._pillar:
            return {}
        return self._pillar.get('data', {}).get('redis', {})

    @property
    def databases(self):
        """
        Get databases num.
        """
        return self._redis['config'].get('databases', None)

    @property
    def maxmemory_policy(self):
        """
        Get maxmemory-policy.
        """
        return self._redis['config'].get('maxmemory-policy', None)

    @property
    def appendonly(self):
        """
        Get aof setting.
        """
        return self._redis['config'].get('appendonly', None)

    @appendonly.setter
    def appendonly(self, appendonly: str) -> None:
        """
        Set appendonly setting.
        """
        self._redis['config']['appendonly'] = appendonly

    @property
    def save(self):
        """
        Get rdb setting.
        """
        return self._redis['config'].get('save', None)

    @save.setter
    def save(self, save: str) -> None:
        """
        Set rdb setting.
        """
        self._redis['config']['save'] = save

    @property
    def max_shards_count(self) -> int:
        """
        Custom max shards count for a cluster.
        """
        return int(self._redis.get('max_shards_count', MAX_SHARDS_COUNT))

    @property
    def password(self) -> Optional[str]:
        """
        Get password.
        """
        return self._redis['config'].get('requirepass', None)

    def _process_persistence_mode(self, mode: str) -> dict:
        def_options = {'appendonly': self.appendonly or 'yes', 'save': self.save or ''}

        if mode == PersistenceModes.off.value:
            return {'appendonly': 'no', 'save': ''}

        if mode == PersistenceModes.on.value:
            if not self.save and self.appendonly == 'no':
                return {'appendonly': 'yes', 'save': ''}
            if self.save and self.appendonly in (None, 'no'):
                return {'appendonly': 'no'}

        return def_options

    def process_persistence_mode(self, mode: str) -> bool:
        def apply(options: dict) -> bool:
            changed = False
            for name, new_value in options.items():
                old_value = getattr(self, name)
                if old_value != new_value:
                    changed = True
                    setattr(self, name, new_value)
            return changed

        def_options = self._process_persistence_mode(mode)
        return apply(def_options)

    def get_persistence_mode_for_info(self) -> str:
        """
        Get persistence mode for info in get/list clusters
        :return:
        """
        result = "ON"
        save = self.save
        appendonly = self.appendonly
        off_tuples = [
            ("", "no"),
            (None, "no"),
        ]

        if (save, appendonly) in off_tuples:
            result = "OFF"
        return result
