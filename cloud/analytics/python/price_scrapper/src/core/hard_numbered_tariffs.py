from abc import abstractmethod, ABCMeta
from typing import Dict

from core.page_scrapper_base import PageScrapperBase
from core.tariff import ComputeTariffMenu, OperSys, StorageTariffMenu
from core.tariff_factory_base import TariffFactoryBase


class EmptyPageScrapper(PageScrapperBase):

    def __init__(self):
        super().__init__("")

    def scrap_all(self):
        raise Exception("Not implemented since hard numbers are used")


class HardNumberedComputeTariffFactory(TariffFactoryBase, metaclass=ABCMeta):
    USD_RUB_rate = 74
    VAT_rate = 0.2

    def __init__(self, company: str):
        super().__init__(company, EmptyPageScrapper())

    @abstractmethod
    def price_2_4_80_on_demand_usd_no_vat(self):
        pass

    @abstractmethod
    def price_2_8_160_on_demand_usd_no_vat(self):
        pass

    def get_compute(self) -> ComputeTariffMenu:
        return ComputeTariffMenu(
            oper_system=OperSys.Linux,
            config_2_4_80=self.t('2-4-80', self.price_2_4_80_on_demand_usd_no_vat()),
            config_2_4_80_1yr=self.empty('2-4-80-1yr'),
            config_2_4_80_3yr=self.empty('2-4-80-3yr'),
            config_2_8_160=self.t('2-8-160', self.price_2_8_160_on_demand_usd_no_vat()),
            config_2_8_160_1yr=self.empty('2-8-160-1yr'),
            config_2_8_160_3yr=self.empty('2-8-160-3yr')
        )

    def get_storage(self) -> StorageTariffMenu:
        raise Exception("Not supported")


class HardNumberedStorageTariffFactory(TariffFactoryBase, metaclass=ABCMeta):
    USD_RUB_rate = 74
    VAT_rate = 0.2

    def __init__(self, company: str):
        super().__init__(company, EmptyPageScrapper())

    def get_compute(self) -> ComputeTariffMenu:
        raise Exception('Get compute method is not supported')

    @abstractmethod
    def get_storage_prices_usd_no_vat(self) -> Dict[str, float]:
        pass

    def get_storage(self) -> StorageTariffMenu:
        self._assert_tariff_dict()
        d = self.get_storage_prices_usd_no_vat()
        return StorageTariffMenu(
            std_per_space=self.s('std_per_gb', d['std_per_gb']),
            std_read=self.s('std_read', d['std_read']),
            std_write=self.s('std_write', d['std_write']),
            cold_per_space=self.s('cold_per_gb', d['cold_per_gb']),
            cold_read=self.s('cold_read', d['cold_read']),
            cold_write=self.s('cold_write', d['cold_write'])
        )

    def _assert_tariff_dict(self):
        required_keys = ['std_per_gb',
                         'std_read',
                         'std_write',
                         'cold_per_gb',
                         'cold_read',
                         'cold_write']
        for k in self.get_storage_prices_usd_no_vat().keys():
            if k not in required_keys:
                raise ValueError(f'Key {k} must be in tariff dictionary')
        for required_key in required_keys:
            if required_key not in required_keys:
                raise ValueError(f'Tariff for <{required_key}> must be tariff dictionary')
