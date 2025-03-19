from core.hard_numbered_tariffs import HardNumberedComputeTariffFactory


class SelectedLinuxComputeFactory(HardNumberedComputeTariffFactory):

    def __init__(self, company: str):
        super().__init__(company)

    def price_2_4_80_on_demand_usd_no_vat(self):
        rub_price = 2618.62
        return rub_price / self.USD_RUB_rate / (1 + self.VAT_rate)

    def price_2_8_160_on_demand_usd_no_vat(self):
        rub_price = 4418.09
        return rub_price / self.USD_RUB_rate / (1 + self.VAT_rate)
