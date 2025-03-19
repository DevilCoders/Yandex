""" Hardware models """
import os
import socket
import logging
import psutil
from icecream_agent.models.storage import detect_storage


class Dom0(object):  # pylint: disable=too-few-public-methods
    """ dom0 object model """

    def __init__(self, pool):
        """
        Dom0 attributes:
          cpu: number of cpu, int
          memory: memory size in bytes, int
          hostname: machine hostname, str
          storage: Storage object
          pool: host machine pool, str
        """
        self.cpu = os.cpu_count()
        self.memory = psutil.virtual_memory().total
        self.hostname = socket.gethostname()
        self.storage = detect_storage()
        self.pool = pool
        self._log = logging.getLogger()
        self._log.debug('Initialized dom0 object')

    def to_dict(self):
        """
        Represent object in python dict
        :return: Dom0 object, json
        """
        data = dict()
        for key, value in vars(self).items():
            if not key.startswith('_') and value is not None:
                if hasattr(value, 'to_dict'):
                    data[key] = value.to_dict()
                else:
                    data[key] = value
        return data


if __name__ == "__main__":
    # machine = Dom0(pool="music", storage_type="raid", storage_name="raid")
    machine = Dom0(pool="music")
    print(machine.to_dict())
