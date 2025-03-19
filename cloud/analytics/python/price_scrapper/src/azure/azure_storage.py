from core.tariff import ComputeTariffMenu, StorageTariffMenu
from core.page_scrapper_base import PageScrapperBase
from core.tariff_factory_base import TariffFactoryBase


class AzureStorageTariffFactory(TariffFactoryBase):

    def __init__(self, company: str, scrapper: PageScrapperBase):
        super().__init__(company, scrapper)

    def get_compute(self) -> ComputeTariffMenu:
        raise Exception('Not supported')

    def get_storage(self) -> StorageTariffMenu:
        with self.scrapper as scr:
            scr.scrap_all()
        return StorageTariffMenu(
            std_per_space=self.s('std_per_gb', scr['STD_PER_GB']),
            std_read=self.s('std_read', scr['STD_READ_PER_10000']),
            std_write=self.s('std_write', float(scr['STD_WRITE_PER_10000']) / 10),
            cold_per_space=self.s('cold_per_gb', scr['COLD_PER_GB']),
            cold_read=self.s('cold_read', scr['COLD_READ_PER_10000']),
            cold_write=self.s('cold_write', float(scr['STD_READ_PER_10000']) / 10)
        )
