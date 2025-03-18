# coding: utf-8


from abc import ABCMeta, abstractmethod


class IVendorInstrumenter(object):
    __metaclass__ = ABCMeta

    @abstractmethod
    def instrument(self):
        """
        Инструментировать вендора.
        """
