from core.tariff import ComputeTariffMenu, OperSys, StorageTariffMenu
from core.utils import get_compute_tariffs
from core.tariff_factory_base import TariffFactoryBase
from core.page_scrapper_base import PageScrapperBase


class AzureTariffFactory(TariffFactoryBase):
    HOURS_IN_MONTH = 730

    def __init__(self, company: str, scrapper: PageScrapperBase):
        super().__init__(company, scrapper)
        self.ssd_price = 4.8 / 64  # 4.8$ for 64 Gb ssd. source: Azure pricing calculator

    def get_compute(self) -> ComputeTariffMenu:
        with self.scrapper as scr:
            scr.scrap_all()
            a2v2 = scr['A2v2'] * self.HOURS_IN_MONTH
            d2v3_demand = scr['D2v3_demand'] * self.HOURS_IN_MONTH
            d2v3_one_year = scr['D2v3_one_year'] * self.HOURS_IN_MONTH
            d2v3_three_years = scr['D2v3_three_years'] * self.HOURS_IN_MONTH
            a2v2_storage_cost = (80 - scr['A2v2_incl_storage']) * self.ssd_price
            d2v3_storage_cost = (160 - scr['D2v3_incl_storage']) * self.ssd_price
        return ComputeTariffMenu(
            oper_system=OperSys.Linux,
            config_2_4_80=self.t('2-4-80', a2v2 + a2v2_storage_cost),
            config_2_4_80_1yr=self.empty('2-4-80-1yr'),
            config_2_4_80_3yr=self.empty('2-4-80-3yr'),
            config_2_8_160=self.t('2-8-160', d2v3_demand + d2v3_storage_cost),
            config_2_8_160_1yr=self.t('2-8-160-1yr', d2v3_one_year + d2v3_storage_cost),
            config_2_8_160_3yr=self.t('2-8-160-3yr', d2v3_three_years + d2v3_storage_cost)
        )

    def get_storage(self) -> StorageTariffMenu:
        raise Exception('Get storage method is not supported')


class AzureLinuxTariffFactory(TariffFactoryBase):

    def __init__(self, company: str, scrapper: PageScrapperBase):
        super().__init__(company, scrapper)
        self.azure_factory = AzureTariffFactory(company, scrapper)

    def get_compute(self) -> ComputeTariffMenu:
        menu = self.azure_factory.get_compute()
        for t in get_compute_tariffs(menu):
            t.oper_sys = OperSys.Linux
        return menu

    def get_storage(self) -> StorageTariffMenu:
        raise Exception('Not supported')


class AzureWindowsTariffFactory(TariffFactoryBase):
    def __init__(self, company: str, scrapper: PageScrapperBase):
        super().__init__(company, scrapper)
        self.azure_factory = AzureTariffFactory(company, scrapper)

    def get_compute(self) -> ComputeTariffMenu:
        menu = self.azure_factory.get_compute()
        menu.oper_system = OperSys.Windows
        for t in get_compute_tariffs(menu):
            t.oper_sys = OperSys.Windows
        return menu

    def get_storage(self) -> StorageTariffMenu:
        raise Exception('Not supported')
