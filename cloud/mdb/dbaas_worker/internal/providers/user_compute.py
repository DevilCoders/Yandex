"""
YC.Compute for working with user's compute API
"""

from copy import deepcopy

from .compute import ComputeApi


class UserComputeApi(ComputeApi):
    """
    This class performs actions on compute resources in the user's folder.
    However, all these actions are implemented in the ComputeApi provider,
    which is intended for managing resources of managed databases.
    If you need to extend the behavior of UserComputeApi provider,
    we recommend implementing the corresponding logic directly in the ComputeApi provider.
    """

    def __init__(self, config, task, queue):
        new_config = deepcopy(config)
        new_config.compute.folder_id = task['folder_id']
        new_config.compute.managed_network_id = None

        super().__init__(new_config, task, queue)
        self.should_retry_operations_limit_exceeded = True
