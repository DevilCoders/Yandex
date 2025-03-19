from abc import ABC, abstractmethod

class CubeJob(ABC):
    def __init__(self, job, paths_dict, folder) -> None:
        self._job = job
        self._paths_dict = paths_dict
        self._folder = folder
    
    @abstractmethod
    def job(self):
        pass

    