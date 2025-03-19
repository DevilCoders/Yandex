"""
OpenSearch host create executor.
"""
from copy import deepcopy

from ...common.host.create import HostCreateExecutor
from ...utils import register_executor
from ..utils import classify_host_map


@register_executor('opensearch_host_create')
class OpensearchHostCreate(HostCreateExecutor):
    """
    Create OpenSearch host in dbm or compute.
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.properties = deepcopy(self.config.opensearch_data)
        _, data_hosts = classify_host_map(self.args['hosts'])
        self.properties.conductor_group_id = next(iter(data_hosts.values()))['subcid']

    def run(self):
        host = self.args['host']
        self.args['hosts'][host]['service_account_id'] = self.args.get('service_account_id')

        super().run()
