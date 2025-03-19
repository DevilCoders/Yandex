from ...base import BaseExecutor
from cloud.mdb.dbaas_worker.internal.providers.solomon_service_alerts import SolomonServiceAlerts


class AlertGroupExecutor(BaseExecutor):
    """
    Base class for alert-management executors
    """

    def run(self):
        self.mlock.lock_cluster(['noop-host'])
        alerts_provider = SolomonServiceAlerts(self.config, self.task, self.queue)
        alerts_provider.state_to_desired(self.task['cid'])
        self.mlock.unlock_cluster()
