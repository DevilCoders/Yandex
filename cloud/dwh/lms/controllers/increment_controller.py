from typing import Union
from datetime import datetime

from cloud.dwh.lms.models.metadata import LMSDBLoadMetadataInc
from cloud.dwh.lms.services.increment_manager import IncrementManager


class IncrementController:
    def __init__(self, metadata: LMSDBLoadMetadataInc):
        self._increment_manager = IncrementManager(metadata=metadata)

    def get_increment_value(self):
        return self._increment_manager.get_increment_value()

    def update_increment_value(self, increment_value: Union[int, datetime]):
        return self._increment_manager.update_increment_value(increment_value)

    def increment_value_to_string(self, increment_value: Union[int, datetime]):
        return self._increment_manager.increment_value_to_string(increment_value)

    def string_to_increment_value(self, increment_value: str):
        inc_value, _ = self._increment_manager.get_py_increment_value_and_step_back(increment_value)
        return inc_value
