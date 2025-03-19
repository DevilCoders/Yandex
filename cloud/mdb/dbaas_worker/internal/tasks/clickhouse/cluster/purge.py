"""
ClickHouse purge cluster executor
"""

from ....providers.billingdb.billingdb import BillingDBProvider, BillType
from ...common.cluster.purge import ClusterPurgeExecutor
from ...utils import register_executor


@register_executor('clickhouse_cluster_purge')
class ClickHouseClusterPurge(ClusterPurgeExecutor):
    """
    Purge ClickHouse cluster
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.billingdb = BillingDBProvider(self.config, self.task, self.queue, self.args)

    def run(self):
        """
        Delete backups and Cloud Storage buckets, remove billing info.
        """
        super().run()
        self.billingdb.disable_billing(cid=self.task['cid'], bill_type=BillType.CH_CLOUD_STORAGE)
