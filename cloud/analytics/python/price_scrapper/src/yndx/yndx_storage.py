from typing import Optional

from core.tariff import StorageTariffMenu, ComputeTariffMenu
from core.tariff_factory_base import TariffFactoryBase
from core.page_scrapper_base import PageScrapperBase
from yndx.yndx_storage_scrapper import YndxStorageScrapper


class YandexStorageTariffFactory(TariffFactoryBase):

    def __init__(self, company: str, scrapper: PageScrapperBase):
        super().__init__(company, scrapper)
        if not isinstance(scrapper, YndxStorageScrapper):
            raise TypeError('YndxStorageScrapper is expected')
        self._tariff: Optional[StorageTariffMenu] = None

    def get_storage(self) -> StorageTariffMenu:
        if self._tariff is not None:
            return self._tariff

        with self.scrapper as scr:
            scr.scrap_all()
            self._tariff = StorageTariffMenu(std_per_space=self.s('std_per_gb', self.scrapper['STD_PER_GB']),
                                             std_read=self.s('std_read', self.scrapper['STD_READ_PER_10000']),
                                             std_write=self.s('std_write', self.scrapper['STD_WRITE_PER_1000']),
                                             cold_per_space=self.s('cold_per_gb', self.scrapper['COLD_PER_GB']),
                                             cold_read=self.s('cold_read', self.scrapper['COLD_READ_PER_10000']),
                                             cold_write=self.s('cold_write', self.scrapper['COLD_WRITE_PER_1000']))

        return self._tariff

    def get_compute(self) -> ComputeTariffMenu:
        raise Exception('Get compute method is not supported')
