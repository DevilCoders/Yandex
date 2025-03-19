from typing import Union
from datetime import datetime, timedelta

from cloud.dwh.lms.exceptions import UnknownIncrementTypeError

from cloud.dwh.lms.models.metadata import LMSDBLoadMetadataInc, LMSIncrementType

from cloud.dwh.lms.repositories.db_load_metadata_repo import DBLoadMetadataRepository
from cloud.dwh.lms.utils import get_cursor
from cloud.dwh.lms import config


class IncrementManager:
    """
    Class for saving / loading increment value for an object_id
    """
    def __init__(self, metadata: LMSDBLoadMetadataInc):
        self._metadata = metadata

    def get_increment_value(self):
        inc_value = DBLoadMetadataRepository.get_increment_value(get_cursor(config.METADATA_CONN_ID),
                                                                 self._metadata.object_id)
        inc_value, step_back = self.get_py_increment_value_and_step_back(inc_value)
        return inc_value - step_back

    def update_increment_value(self, increment_value: Union[int, datetime]):
        increment_value = self.increment_value_to_string(increment_value)
        cursor = get_cursor(config.METADATA_CONN_ID)
        DBLoadMetadataRepository.update_increment_value(cursor, self._metadata.object_id, increment_value)
        cursor.execute("commit")

    def increment_value_to_string(self, increment_value: Union[int, datetime]):
        """
        Convert increment value to string using appropriate inc type
        :param increment_value: Union[int, datetime]
        :return: str
        """
        if self._metadata.increment_type == LMSIncrementType.Integer:
            return str(increment_value)
        if self._metadata.increment_type == LMSIncrementType.Date:
            return increment_value.strftime("%Y%m%d")
        if self._metadata.increment_type == LMSIncrementType.Datetime:
            return increment_value.strftime("%Y%m%d%H%M%S")
        raise UnknownIncrementTypeError(f"Unknown increment_type passed: {self._metadata.increment_type}")

    def get_py_increment_value_and_step_back(self, increment_value: str):
        """
        Return increment and step back values of appropriate python types
        :param increment_value: str
        :return: (increment_value, step_back_value)
        """
        if self._metadata.increment_type == LMSIncrementType.Integer:
            return int(increment_value), int(self._metadata.step_back_value)
        if self._metadata.increment_type == LMSIncrementType.Date:
            return datetime.strptime(increment_value, "%Y%m%d"), timedelta(days=int(self._metadata.step_back_value))
        if self._metadata.increment_type == LMSIncrementType.Datetime:
            return datetime.strptime(increment_value, "%Y%m%d%H%M%S"), timedelta(seconds=int(self._metadata.step_back_value))
        raise UnknownIncrementTypeError(f"Unknown increment_type passed: {self._metadata.increment_type}")
