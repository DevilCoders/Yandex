from typing import Dict

from core.hard_numbered_tariffs import HardNumberedStorageTariffFactory


class SelectelStorageTariffFactory(HardNumberedStorageTariffFactory):
    def __init__(self, company: str):
        super().__init__(company)

    def get_storage_prices_usd_no_vat(self) -> Dict[str, float]:
        usd_rate = self.USD_RUB_rate
        vat = self.VAT_rate
        return {
            'std_per_gb': 1.43 / usd_rate / (1 + vat),
            'std_read': 0.24 / usd_rate / (1 + vat),
            'std_write': 0.31 / usd_rate / (1 + vat),
            'cold_per_gb': 0.7 / usd_rate / (1 + vat),
            'cold_read': 0.62 / usd_rate / (1 + vat),
            'cold_write': 0.62 / usd_rate / (1 + vat)
        }
