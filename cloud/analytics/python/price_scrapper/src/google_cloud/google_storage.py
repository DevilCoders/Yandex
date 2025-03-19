from core.tariff import ComputeTariffMenu, StorageTariffMenu
from core.page_scrapper_base import PageScrapperBase
from core.tariff_factory_base import TariffFactoryBase


class GoogleStorageTariffFactory(TariffFactoryBase):

    def __init__(self, company: str, scrapper: PageScrapperBase):
        super().__init__(company, scrapper)

    def get_storage(self) -> StorageTariffMenu:
        with self.scrapper as scr:
            scr.scrap_all()
            comment = 'Region Europe, location Frankfurt'
            return StorageTariffMenu(
                std_per_space=self.s('std_per_gb', scr['STD_PER_GB'], comment),
                std_read=self.s('std_read', scr['STD_READ_PER_10000'], comment),
                std_write=self.s('std_write', float(scr['STD_WRITE_PER_10000']) / 10, comment),
                cold_per_space=self.s('cold_per_gb', scr['COLD_PER_GB'], comment),
                cold_read=self.s('cold_read', scr['COLD_READ_PER_10000'], comment),
                cold_write=self.s('cold_write', float(scr['COLD_WRITE_PER_10000']) / 10, comment)
            )

    def get_compute(self) -> ComputeTariffMenu:
        pass
