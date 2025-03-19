# pylint: disable=ungrouped-imports
"""
Worker interaction module
"""
from dbaas_worker.providers.compute import ComputeApi as BaseComputeApi
from dbaas_worker.providers.conductor import ConductorApi as BaseConductorApi

from .utils.mock import FAKE_TASK, DoNothing


class ConductorApi(BaseConductorApi):
    """
    ConductorApi wrapper for bootstrap
    """
    def __init__(self, config):
        super().__init__(config, FAKE_TASK, DoNothing())
        self.report = DoNothing()

    # pylint: disable=no-self-use
    def conductor_group_name_from_id(self, group_id):
        """
        Override parent method to generate group name
        """
        return str(group_id).replace('-', '_')


class ComputeApi(BaseComputeApi):
    """
    ComputeApi wrapper for bootstrap
    """
    def __init__(self, config):
        super().__init__(config, FAKE_TASK, DoNothing())
        self.metadb_host = DoNothing()
        self.report = DoNothing()
