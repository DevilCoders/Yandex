from cloud.mdb.dbaas_worker.internal.tasks.common.modify import BaseModifyExecutor


class MoveContainerExecutor(BaseModifyExecutor):
    """
    Special executor to expose required methods
    """

    def run_operation_sync(self, *args, **kwargs):
        """
        Syncronous operation run on host
        """
        return self.deploy_api.wait([self._run_operation_host(*args, **kwargs)])

    def update_other_hosts_metadata(self, exclude_host):
        """
        Update metadata on all hosts with exclusion
        """
        return self._update_other_hosts_metadata(exclude_host)
