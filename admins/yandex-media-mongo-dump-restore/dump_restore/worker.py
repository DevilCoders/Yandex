from . import MongoWorker
from . import ZLock
import logging


class Worker:
    def __init__(self, config, **kwargs):
        self.config = config
        self.log = logging.getLogger(self.__class__.__name__)
        if 'run_parts' in kwargs:
            self.run_parts = kwargs['run_parts']
            self.log.info("Partial run config. Run names: %s", self.run_parts)
        else:
            self.run_parts = None

    def run(self, name):
        if name not in self.config:
            self.log.info("%s not found in config names. Skip.", name)
            return
        config = self.config.get(name)
        zklock = ZLock(config)
        if not zklock.acquire():
            self.log.info("Failed to get zk lock for config name %s. Exit.", name)
            return
        worker = MongoWorker(config)
        if config.action == 'backup':
            worker.dump()
        elif config.action == 'restore':
            worker.restore()
        else:
            self.log.warning("Unknown action %s in config named %s", config.action, name)
        zklock.release()

    def execute(self):
        if self.run_parts is not None:
            for name in self.run_parts:
                self.run(name)
        else:
            for name in self.config.keys():
                self.run(name)
