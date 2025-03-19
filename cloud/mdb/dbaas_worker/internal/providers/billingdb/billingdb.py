"""
BillingDB manipulation module
"""

from enum import Enum

from dbaas_common import tracing, retry

from ...metadb import get_cursor, DatabaseConnectionError
from ...query import execute
from ..common import BaseProvider, Change


class BillType(Enum):
    BACKUP = "BACKUP"
    CH_CLOUD_STORAGE = "CH_CLOUD_STORAGE"


class BillingDBProvider(BaseProvider):
    """
    BillingDB provider
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue)
        self._cluster_type = next(iter(args['hosts'].values()))['cluster_type']

    @retry.on_exception(DatabaseConnectionError, factor=10, max_wait=60, max_tries=6)
    @tracing.trace('BillingDB enable billing')
    def enable_billing(self, cid: str, bill_type: BillType):
        """
        Add cluster to billingdb tracks with suplpied billing type.
        """
        if not self.config.billingdb.enabled:
            return

        def rollback(task, safe_revision):
            self.disable_billing(cid, bill_type)

        change = Change(f'billing.{bill_type}.{cid}', 'enabled', rollback=rollback)
        context_key = f'billing.{bill_type}.{cid}'
        if self.context_get(context_key):
            self.add_change(change)
            return

        tracing.set_tag('cluster.id', cid)
        with self.get_master_cursor() as cursor:
            execute(
                cursor,
                'enable_billing',
                fetch=False,
                cluster_id=cid,
                bill_type=bill_type.value,
                cluster_type=self._cluster_type,
            )

        self.add_change(
            Change(
                key=change.key,
                value=change.value,
                rollback=change.rollback,
                context={context_key: True},
            )
        )

    @retry.on_exception(DatabaseConnectionError, factor=10, max_wait=60, max_tries=6)
    @tracing.trace('BillingDB disable billing')
    def disable_billing(self, cid: str, bill_type: BillType):
        """
        Remove cluster from billingdb tracks with suplpied billing type.
        """
        if not self.config.billingdb.enabled:
            return

        def rollback(task, safe_revision):
            self.enable_billing(cid, bill_type)

        tracing.set_tag('cluster.id', cid)
        with self.get_master_cursor() as cursor:
            execute(
                cursor,
                'disable_billing',
                fetch=False,
                cluster_id=cid,
                bill_type=bill_type.value,
            )

        self.add_change(Change(f'billing.{bill_type}.{cid}', 'disabled', rollback=rollback))

    def get_master_cursor(self):
        """
        Get cursor with connection to master
        """
        return get_cursor(self.config.billingdb.billingdb_dsn, self.config.billingdb.billingdb_hosts, self.logger)
