from typing import Dict, Any

from pandas import DataFrame

from core.tariff import StorageTariff, StorageServiceType, StorageType, TariffCurrency


class StorageDashboard:

    def __init__(self):
        self.storage_table = self._create_table()
        self._scrappers: Dict[str, Any] = {}

    def add_scrapper(self, company: str, s: Any):
        self._scrappers[company] = s

    def _create_table(self) -> DataFrame:
        df = DataFrame(columns=[
            'company',
            'service',
            'price',
            'pricing_unit',
            'currency'
        ])
        return df

    def calculate(self):
        for company, s in self._scrappers.items():
            menu = s.get_storage()
            for attr_name in dir(menu):
                t = getattr(menu, attr_name)
                if not isinstance(t, StorageTariff):
                    continue
                self.storage_table = self.storage_table.append({
                    'company': company,
                    'service': self._display_service(t),
                    'price': round(t.price, 4) if t.price != -1 else 0,
                    'pricing_unit': t.pricing_unit,
                    'currency': 'USD' if t.currency == TariffCurrency.USD else 'RUB'
                }, ignore_index=True)

    def _display_service(self, t: StorageTariff):
        if t.service == StorageServiceType.PER_SPACE:
            return "standard_storage" if t.storage_type == StorageType.STD else "cold_storage"
        elif t.service == StorageServiceType.READ:
            return "standard_read" if t.storage_type == StorageType.STD else "cold_read"
        elif t.service == StorageServiceType.WRITE:
            return "standard_write" if t.storage_type == StorageType.STD else "cold_write"
        else:
            raise ValueError(f"Unknown storage service: {t.service}")
