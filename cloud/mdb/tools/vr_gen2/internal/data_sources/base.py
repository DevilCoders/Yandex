from abc import abstractmethod, ABC
from typing import List


class FlavorSourceBase(ABC):
    @abstractmethod
    def filter_ids_by_type(self, pattern) -> List[str]:
        pass

    @abstractmethod
    def filter_ids_by_name(self, pattern) -> List[str]:
        pass


class DiskTypeIdSourceBase(ABC):
    @abstractmethod
    def get_by_ext_id(self, ext_id) -> int:
        pass


class GeoIdSourceBase(ABC):
    @abstractmethod
    def get_without_names(self, excluded_geos: List[str]) -> List[int]:
        pass
