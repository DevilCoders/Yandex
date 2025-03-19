from typing import Optional

from core.ConfigCalculator import ConfigCalculator
from core.tariff import ComputeTariffMenu, Service, OperSys, StorageTariffMenu
from core.tariff_factory_base import TariffFactoryBase
from core.page_scrapper_base import PageScrapperBase
from yndx.yndx_compute_scrapper import YndxComputeScrapper


class YndxLinuxComputeTariffFactory(TariffFactoryBase):

    def __init__(self, scrapper: PageScrapperBase):
        super().__init__("Yandex", scrapper)
        if not isinstance(scrapper, YndxComputeScrapper):
            raise TypeError('YndxComputeScrapper class is expected')
        self._tariff: Optional[ComputeTariffMenu] = None

    def get_compute(self) -> ComputeTariffMenu:
        if self._tariff is not None:
            return self._tariff

        with self.scrapper as scr:
            scr.scrap_all()
            p_2_4_80 = ConfigCalculator(Service.CONFIG_2_4_80).get_price(cpu_per_hour=scr["CPU_HOURLY_PRICE"],
                                                                         ram_per_gb_per_hour=scr["RAM_HOURLY_PRICE"],
                                                                         monthly_ssd_per_gb=scr["SSD_PRICE_PER_GB"])
            p_2_8_160 = ConfigCalculator(Service.CONFIG_2_8_160).get_price(cpu_per_hour=scr["CPU_HOURLY_PRICE"],
                                                                           ram_per_gb_per_hour=scr["RAM_HOURLY_PRICE"],
                                                                           monthly_ssd_per_gb=scr["SSD_PRICE_PER_GB"])
            self._tariff = ComputeTariffMenu(oper_system=OperSys.Linux,
                                             config_2_4_80=self.t('2-4-80', p_2_4_80),
                                             config_2_4_80_1yr=self.t('2-4-80-1yr', p_2_4_80 * YndxLinuxComputeTariffFactory.MULT_ONE_YEAR),
                                             config_2_4_80_3yr=self.t('2-4-80-3yr', p_2_4_80 * YndxLinuxComputeTariffFactory.MUL_THREE_YEARS),
                                             config_2_8_160=self.t('2-8-160', p_2_8_160),
                                             config_2_8_160_1yr=self.t('2-8-160-1yr', p_2_8_160 * YndxLinuxComputeTariffFactory.MULT_ONE_YEAR),
                                             config_2_8_160_3yr=self.t('2-8-160-3yr', p_2_8_160 * YndxLinuxComputeTariffFactory.MUL_THREE_YEARS)
                                             )
            return self._tariff

    def get_storage(self) -> StorageTariffMenu:
        raise Exception('Get storage method is not supported')


class YndxWinComputeTariffFactory(TariffFactoryBase):

    def __init__(self, linux_compute: YndxLinuxComputeTariffFactory):
        super().__init__("Yandex", linux_compute.scrapper)
        self._lin_compute = linux_compute

    def get_compute(self) -> ComputeTariffMenu:
        linux_tariffs = self._lin_compute.get_compute()
        if not self.scrapper.has_scrapped_data:
            raise Exception("Page scrapper hasn't scrapped data yet.")
        p_2_4_80 = linux_tariffs.config_2_4_80.price + self.scrapper["WINDOWS_PER_CORE_PRICE"] * 720 * 2
        p_2_8_160 = linux_tariffs.config_2_8_160.price + self.scrapper["WINDOWS_PER_CORE_PRICE"] * 720 * 2

        return ComputeTariffMenu(
            oper_system=OperSys.Windows,
            config_2_4_80=self.win_t('2-4-80', p_2_4_80),
            config_2_4_80_1yr=self.win_t('2-4-80-1yr', p_2_4_80 * self.MULT_ONE_YEAR),
            config_2_4_80_3yr=self.win_t('2-4-80-3yr', p_2_4_80 * self.MUL_THREE_YEARS),
            config_2_8_160=self.win_t('2-8-160', p_2_8_160),
            config_2_8_160_1yr=self.win_t('2-8-160-1yr', p_2_8_160 * self.MULT_ONE_YEAR),
            config_2_8_160_3yr=self.win_t('2-8-160-3yr', p_2_8_160 * self.MUL_THREE_YEARS)
        )

    def get_storage(self) -> StorageTariffMenu:
        raise Exception('Get storage method is not supported')
