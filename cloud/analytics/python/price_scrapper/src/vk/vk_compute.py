from core.hard_numbered_tariffs import HardNumberedComputeTariffFactory
from core.tariff import StorageTariffMenu, ComputeTariffMenu, OperSys
from core.utils import get_compute_tariffs


class VkLinuxComputeTariffFactory(HardNumberedComputeTariffFactory):

    def __init__(self, company: str):
        super().__init__(company)

    def price_2_4_80_on_demand_usd_no_vat(self):
        # https://mcs.mail.ru/pricing/
        rub_price_vat = 2580
        return rub_price_vat / self.USD_RUB_rate / (1 + self.VAT_rate)

    def price_2_8_160_on_demand_usd_no_vat(self):
        rub_price_vat = 4140
        return rub_price_vat / self.USD_RUB_rate / (1 + self.VAT_rate)

    def get_storage(self) -> StorageTariffMenu:
        raise Exception("Storage is not supported")


class VkWindowsComputeTariffFactory(HardNumberedComputeTariffFactory):

    def __init__(self, company: str):
        super().__init__(company)

    def price_2_4_80_on_demand_usd_no_vat(self):
        # https://mcs.mail.ru/pricing/
        rub_price_vat = 2944.98
        return rub_price_vat / self.USD_RUB_rate / (1 + self.VAT_rate)

    def price_2_8_160_on_demand_usd_no_vat(self):
        rub_price_vat = 4504.98
        return rub_price_vat / self.USD_RUB_rate / (1 + self.VAT_rate)

    def get_compute(self) -> ComputeTariffMenu:
        menu = super().get_compute()
        self._set_windows(menu)
        return menu

    def get_storage(self) -> StorageTariffMenu:
        raise Exception("Storage is not supported")

    def _set_windows(self, menu: ComputeTariffMenu):
        menu.oper_system = OperSys.Windows
        for t in get_compute_tariffs(menu):
            t.oper_sys = OperSys.Windows
