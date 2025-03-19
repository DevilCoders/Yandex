from typing import List, Dict, Any

from pandas import DataFrame
from tqdm import tqdm

from core.tariff import *


class ComputeDashboard:

    def __init__(self):
        self.tariff_table = self._create_table()
        self.tariffs_flat_table = self._create_flat_tariffs_table()
        self._scrappers: Dict[str, Any] = {}

    def _create_flat_tariffs_table(self) -> DataFrame:
        df = DataFrame(columns=['company', 'service', 'commitment', 'price', 'currency', 'pricing_unit', 'comment'])
        return df

    def _create_table(self) -> DataFrame:
        conf_2_4_80_cols = self._service_columns(Service.CONFIG_2_4_80)
        conf_2_8_160_cols = self._service_columns(Service.CONFIG_2_8_160)

        return DataFrame(columns=conf_2_4_80_cols + conf_2_8_160_cols,
                         index=['Amazon', 'Azure', 'Google', 'Yandex', 'Selectel', 'VK']).fillna(0)

    def _service_columns(self, s: Service) -> List[str]:
        return [f"{s.value}_{oper_sys}_{c.value}" for oper_sys in ['linux', 'windows'] for c in Commitment]

    def add_tariff_factory(self, company_name: str, s):
        self._scrappers[company_name] = s

    def calculate(self):
        provider_index = 1
        with tqdm(total=len(self._scrappers)) as pbar:
            for company_name, scrapper in self._scrappers.items():
                pbar.set_description(f"Processing {company_name} ({provider_index} out of {len(self._scrappers)})")
                menu = scrapper.get_compute()
                self._append_tariffs(menu)
                pbar.update(1)
                provider_index += 1

    def _append_tariffs(self, menu: ComputeTariffMenu):
        for attr_name in dir(menu):
            t = getattr(menu, attr_name)
            if not isinstance(t, ComputeTariff):
                continue
            if t.company not in self.tariff_table.index:
                continue
            col_name = f"{t.service.value}_{t.oper_sys.value}_{t.commitment.value}"
            if col_name not in self.tariff_table.columns:
                raise ValueError(f'Fail to put tariff for service = {t.service} and commitment = {t.commitment}')
            if self.tariff_table.loc[t.company, col_name] > 0:
                raise Exception(f"Tariff table already contains value for company <{t.company}> and {col_name}")
            self.tariff_table.loc[t.company, col_name] = t.price
            self.tariffs_flat_table = self.tariffs_flat_table.append({
                'company': t.company,
                'service': self._display_service(t.service, t.oper_sys),
                'commitment': self._display_commitment(t.commitment),
                'price': round(t.price, 2) if t.price != -1 else 0,
                'currency': self._display_currency(t.currency),
                'pricing_unit': t.pricing_unit,
                'comment': t.comment
            }, ignore_index=True)

    def _display_currency(self, c: TariffCurrency) -> str:
        if c == TariffCurrency.USD:
            return 'USD'
        elif c == TariffCurrency.RUB:
            return 'RUB'
        else:
            raise ValueError(f'Unexpected currency: {c}')

    def _display_os(self, o: OperSys) -> str:
        if o == OperSys.Linux:
            return "linux"
        elif o == OperSys.Windows:
            return "windows"
        else:
            raise ValueError("Unexpected operation system value")

    def _display_service(self, s: Service, o: OperSys) -> str:
        if s == Service.CONFIG_2_4_80:
            return f"2_4_80_{self._display_os(o)}"
        elif s == Service.CONFIG_2_8_160:
            return f"2_8_160_{self._display_os(o)}"
        else:
            raise ValueError("Unexpected service")

    def _display_commitment(self, c: Commitment):
        if c == Commitment.ON_DEMAND:
            return "on_demand"
        elif c == Commitment.ONE_YEAR:
            return "1_year"
        elif c == Commitment.THREE_YEAR:
            return "3_year"
