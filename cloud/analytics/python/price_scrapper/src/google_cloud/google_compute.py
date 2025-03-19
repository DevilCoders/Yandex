from core.ConfigCalculator import ConfigCalculator
from core.tariff import StorageTariffMenu, ComputeTariffMenu, Service, OperSys
from core.utils import get_compute_tariffs
from core.page_scrapper_base import PageScrapperBase
from core.tariff_factory_base import TariffFactoryBase


class GoogleLinuxComputeTariffFactory(TariffFactoryBase):

    def __init__(self, company: str, scrapper: PageScrapperBase):
        super().__init__(company, scrapper)

    def get_compute(self) -> ComputeTariffMenu:
        with self.scrapper as scr:
            scr.scrap_all()

            # 2-4-80
            p_2_4_80_dem = ConfigCalculator(Service.CONFIG_2_4_80).get_price(scr["PER_CORE_PER_HOUR"],
                                                                             scr["PER_RAM_PER_GB_PER_HOUR"],
                                                                             scr["SSD_MONTHLY_PRICE_PER_GB"])
            p_2_4_80_1yr = ConfigCalculator(Service.CONFIG_2_4_80).get_price(scr["PER_CORE_PER_HOUR_1YR"],
                                                                             scr["PER_RAM_PER_GB_PER_HOUR_1YR"],
                                                                             scr["SSD_MONTHLY_PRICE_PER_GB"])
            p_2_4_80_3yr = ConfigCalculator(Service.CONFIG_2_4_80).get_price(scr["PER_CORE_PER_HOUR_3YR"],
                                                                             scr["PER_RAM_PER_GB_PER_HOUR_3YR"],
                                                                             scr["SSD_MONTHLY_PRICE_PER_GB"])

            # 2-8-160
            p_2_8_160_dem = ConfigCalculator(Service.CONFIG_2_8_160).get_price(scr["PER_CORE_PER_HOUR"],
                                                                               scr["PER_RAM_PER_GB_PER_HOUR"],
                                                                               scr["SSD_MONTHLY_PRICE_PER_GB"])
            p_2_8_160_1yr = ConfigCalculator(Service.CONFIG_2_8_160).get_price(scr["PER_CORE_PER_HOUR_1YR"],
                                                                               scr["PER_RAM_PER_GB_PER_HOUR_1YR"],
                                                                               scr["SSD_MONTHLY_PRICE_PER_GB"])
            p_2_8_160_3yr = ConfigCalculator(Service.CONFIG_2_8_160).get_price(scr["PER_CORE_PER_HOUR_3YR"],
                                                                               scr["PER_RAM_PER_GB_PER_HOUR_3YR"],
                                                                               scr["SSD_MONTHLY_PRICE_PER_GB"])
        return ComputeTariffMenu(
            oper_system=OperSys.Linux,
            config_2_4_80=self.t('2-4-80', p_2_4_80_dem, 'Region Frankfurt'),
            config_2_4_80_1yr=self.t('2-4-80-1yr', p_2_4_80_1yr, 'Region Frankfurt'),
            config_2_4_80_3yr=self.t('2-4-80-3yr', p_2_4_80_3yr, 'Region Frankfurt'),
            config_2_8_160=self.t('2-8-160', p_2_8_160_dem, 'Region Frankfurt'),
            config_2_8_160_1yr=self.t('2-8-160-1yr', p_2_8_160_1yr, 'Region Frankfurt'),
            config_2_8_160_3yr=self.t('2-8-160-3yr', p_2_8_160_3yr, 'Region Frankfurt')
        )

    def get_storage(self) -> StorageTariffMenu:
        raise Exception('Not supported')


class GoogleWindowsComputeTariffFactory(TariffFactoryBase):
    WinLicenceCost: float = 67.16

    def __init__(self, company: str, scrapper: PageScrapperBase):
        super().__init__(company, scrapper)
        self._lin_factory = GoogleLinuxComputeTariffFactory(company, scrapper)

    def get_compute(self) -> ComputeTariffMenu:
        menu = self._lin_factory.get_compute()
        self._set_windows(menu)
        return menu.clone_and_inc_price(self.WinLicenceCost)

    def _set_windows(self, menu: ComputeTariffMenu):
        menu.oper_system = OperSys.Windows
        for t in get_compute_tariffs(menu):
            t.oper_sys = OperSys.Windows

    def get_storage(self) -> StorageTariffMenu:
        raise Exception("Not supported")
