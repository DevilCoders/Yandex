"""
MongoDB enable sharding executor.
"""

from ....utils import get_first_key, get_first_value
from ...common.create import BaseCreateExecutor
from ...utils import build_host_group, register_executor
from ..utils import (
    MONGOCFG_HOST_TYPE,
    MONGOD_HOST_TYPE,
    MONGOS_HOST_TYPE,
    MONGOINFRA_HOST_TYPE,
    ORDERED_HOST_TYPES,
    SHARDING_INFRA_HOST_TYPES,
    classify_host_map,
)


@register_executor('mongodb_cluster_enable_sharding')
class MongodbShardingEnableExecuter(BaseCreateExecutor):
    """
    Enable MongoDB sharding
    """

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        hosts = classify_host_map(self.args['hosts'])
        host_groups = {
            host_type: build_host_group(getattr(self.config, host_type), hosts[host_type]) for host_type in hosts
        }

        self._health_host_group(host_groups[MONGOD_HOST_TYPE])

        for host_type in SHARDING_INFRA_HOST_TYPES:
            if host_type not in host_groups or not host_groups[host_type].hosts:
                # Do not process subclusters if there is no hosts of such type
                continue
            hgroup = host_groups[host_type]
            subcid = get_first_value(hgroup.hosts)['subcid']
            hgroup.properties.conductor_group_id = subcid
            self._create_conductor_group(hgroup)
            self._create_host_secrets(hgroup)
            self._create_host_group(hgroup)
            self._issue_tls(hgroup)

        shard_id = self.args.get('shard_id')

        # remove backward comp: drop "mongodb-add-shards" after worker deploy st/MDB-3503
        if shard_id is None:
            shard_id = get_first_key(self.args['mongodb-add-shards'])

        pillar = {
            MONGOD_HOST_TYPE: {
                'primary': {
                    'service-restart': True,
                    'service-stepdown': True,
                },
                'secondary': {
                    'service-restart': True,
                    'service-stepdown': True,
                },
            },
            MONGOCFG_HOST_TYPE: {
                'primary': {
                    'do-backup': True,
                },
                'secondary': {
                    'replica': True,
                },
            },
            MONGOS_HOST_TYPE: {
                'primary': {
                    'mongodb-add-shards': [shard_id],
                },
            },
            MONGOINFRA_HOST_TYPE: {
                'primary': {
                    'do-backup': True,
                    'mongodb-add-shards': [shard_id],
                },
                'secondary': {
                    'replica': True,
                    'mongodb-add-shards': [shard_id],
                },
            },
        }

        for host_type in ORDERED_HOST_TYPES:
            if host_type not in host_groups or not host_groups[host_type].hosts:
                # Do not process subclusters if there is no hosts of such type
                continue
            hgroup = host_groups[host_type]
            first_creating = get_first_key(hgroup.hosts)
            if host_type == MONGOINFRA_HOST_TYPE and MONGOCFG_HOST_TYPE in host_groups:
                first_creating = None
                # If we have mixed configuration
                # Then there will be no Primary MongoInfra
            host_type_pillar = pillar.get(host_type, {})
            for host, opts in hgroup.hosts.items():
                deploy_pillar = (
                    host_type_pillar.get('primary', {})
                    if host == first_creating
                    else host_type_pillar.get('secondary', {})
                )
                opts['deploy'] = {'pillar': deploy_pillar}

            self._highstate_host_group(hgroup)

        for host_type in ORDERED_HOST_TYPES:
            if host_type not in host_groups or not host_groups[host_type].hosts:
                # Do not check subclusters if there is no hosts of such type
                continue
            hgroup = host_groups[host_type]
            self._health_host_group(hgroup, '-post-highstate')
            if host_type in SHARDING_INFRA_HOST_TYPES:
                self._create_public_records(hgroup)
                self._enable_monitoring(hgroup)
        self.mlock.unlock_cluster()
