from abc import ABCMeta, abstractmethod


class DataSource(object):
    """
        Abstract class for data sources.
        Encapsulates general fields: service_ids array, config, logger.
        Provides method get_check_result which should be implemented in child classes
    """

    __metaclass__ = ABCMeta

    def __init__(self, service_ids, config, logger):
        self.datetime_format = "%Y-%m-%dT%H:%M:%SZ"
        self.service_ids = service_ids
        self.args = config.pop("check_args")
        self.config = config
        self.logger = logger

    @abstractmethod
    def get_check_result(self):
        raise NotImplementedError
