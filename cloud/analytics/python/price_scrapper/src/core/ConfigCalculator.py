from core.tariff import Service, OperSys


class ConfigCalculator:
    HOURS_IN_MONTH = 720

    def __init__(self, service: Service):
        self._service = service

    def get_price(self,
                  cpu_per_hour: float,
                  ram_per_gb_per_hour: float,
                  monthly_ssd_per_gb: float) -> float:
        cpu_per_month = ConfigCalculator.HOURS_IN_MONTH * cpu_per_hour
        ram_per_gb = ConfigCalculator.HOURS_IN_MONTH * ram_per_gb_per_hour
        return cpu_per_month * self._cpu_number() + ram_per_gb * self._ram_gb() + monthly_ssd_per_gb * self._ssd_gb()

    def _cpu_number(self) -> int:
        if self._service == Service.CONFIG_2_4_80 or self._service == Service.CONFIG_2_8_160:
            return 2
        else:
            raise ValueError("Unknown service")

    def _ram_gb(self) -> int:
        if self._service == Service.CONFIG_2_4_80:
            return 4
        elif self._service == Service.CONFIG_2_8_160:
            return 8
        else:
            raise ValueError("Unknows service")

    def _ssd_gb(self) -> int:
        if self._service == Service.CONFIG_2_4_80:
            return 80
        elif self._service == Service.CONFIG_2_8_160:
            return 160
        else:
            raise ValueError("Unknows service")
