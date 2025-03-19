from typing import Union

from cloud.marketplace.common.yc_marketplace_common.models.task import TaskResolution


class TaskProcessError(Exception):
    resolution_resolved = None
    default_message = "Error while process task."

    def __init__(self, message: str = None, resolution_resolved: Union[TaskResolution, None] = None):
        self.resolution_resolved = resolution_resolved

        if message is None:
            message = self.default_message

        super().__init__(message)


class TaskParamValidationError(TaskProcessError):
    default_message = "Error while validate task's params."


class TableNotFoundError(TaskProcessError):
    default_message = "Table not found."
