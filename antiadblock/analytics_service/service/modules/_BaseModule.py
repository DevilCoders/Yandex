# encoding=utf8
from abc import ABCMeta, abstractmethod

# Ключи проверки, чтоб знать какие результаты выгружать из YT и проверять что два разных метода не используют один ключ
CHECK_KEYS = []


class _BaseModule(object):
    """
        Abstract class for modules.
        Encapsulates general fields: logger.
        Provides methods get_check_result and filter_results which should be implemented in child classes
    """

    __metaclass__ = ABCMeta

    def __init__(self, logger):
        self.logger = logger
        self.logger.info('Executing {} module'.format(self.__class__.__name__))

    @abstractmethod
    def get_check_result(self):
        raise NotImplementedError

    @abstractmethod
    def filter_results(self, current_results, previous_results):
        """
        Сравнение двух массивов результатов, возвращает только те что изменились или новые
        :param current_results: текущие результаты в формате [(ключ_проверки: {version: xx, description: уу, url: nn, check_date: 'дата проверки'}), (ключ2: {...})]
        :param previous_results: предыдущие результаты в формате {ключ: [версия, дата], ключ2: [версия, дата], ...}
        :return: фильтрованные текущие результаты
        """
        raise NotImplementedError
