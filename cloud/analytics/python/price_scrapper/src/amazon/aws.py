from abc import ABC
from typing import Dict, Union, Any, Callable

import requests
import json
from selenium.webdriver.firefox.webdriver import WebDriver

from core.exceptions import NotSupportedException
from amazon.aws_helper import AwsHelper
from core.tariff import *
from core.utils import first, json_attr


class AmazonScrapperBase(AwsHelper, ABC):

    def __init__(self, urls):
        super().__init__("Amazon", urls)

    def _sku_price(self, sku_json, attr_name: str, attr_value: str, filter_res: Callable[[Any], bool] = None) -> float:
        return float(self.__sku_price_str(sku_json,
                                          attr_name,
                                          attr_value,
                                          filter_res))

    def __sku_price_str(self, sku_json, attr_name: str, attr_value: str, filter_res: Callable[[Any], bool] = None) -> str:
        if "prices" not in sku_json:
            raise Exception("Json object must have 'prices' attribute")
        found_sku = [sku for sku in sku_json["prices"] if json_attr(sku, f"attributes -> {attr_name}") == attr_value]
        if len(found_sku) > 1:
            if filter_res is None:
                raise ValueError(f"One value is expected for sku name = {attr_value}")
            filtered = [sku for sku in found_sku if filter_res(sku)]
            if len(filtered) == 1:
                return json_attr(first(filtered), "price -> USD")
            else:
                raise ValueError(f"One value is expected after filtering")
        elif len(found_sku) == 1:
            return json_attr(first(found_sku), "price -> USD")
        else:
            raise ValueError(f"No sku with name = {attr_value}")

    def _read_json(self, url: str):
        resp = requests.get(url)
        resp.raise_for_status()
        return json.loads(resp.text)


class AmazonComputeUploader(AmazonScrapperBase):
    HOURS_IN_MONTH = 720
    MONTHS_IN_YEAR = 12
    DEFAULL_SSD_PRICE = 0.119

    def __init__(self, oper_sys: OperSys):
        super().__init__(AmazonComputeUploader.url_for(oper_sys))
        self._oper_sys = oper_sys
        self.ssd_price: Union[float, None] = None

    @staticmethod
    def url_for(oper_system: OperSys) -> Dict[str, str]:
        if oper_system == OperSys.Linux:
            return {
                "on_demand": "https://a0.p.awsstatic.com/pricing/1.0/ec2/region/eu-central-1/ondemand/linux/index.json",
                "one_year_reserve": 'https://b0.p.awsstatic.com/pricing/2.0/meteredUnitMaps/ec2/USD/current/'
                                    'ec2-reservedinstance/1%20year/All%20Upfront/'
                                    'EU%20(Frankfurt)/Linux/Shared/index.json',
                "three_year_reserve": 'https://b0.p.awsstatic.com/pricing/2.0/meteredUnitMaps/ec2/'
                                      'USD/current/ec2-reservedinstance/3%20year/All%20Upfront/EU%20(Frankfurt)/'
                                      'Linux/Shared/index.json?timestamp=1624366372247',
                "storage": "https://calculator.aws/pricing/2.0/meteredUnitMaps/ec2/USD/current/ebs-calculator.json"
            }
        elif oper_system == OperSys.Windows:
            return {
                "on_demand": "https://a0.p.awsstatic.com/pricing/1.0/ec2/region/eu-central-1/ondemand/windows/index.json",
                "one_year_reserve": 'https://b0.p.awsstatic.com/pricing/2.0/meteredUnitMaps/ec2/USD/current/'
                                    'ec2-reservedinstance/1%20year/All%20Upfront/'
                                    'EU%20(Frankfurt)/Windows/Shared/index.json',
                "three_year_reserve": 'https://b0.p.awsstatic.com/pricing/2.0/meteredUnitMaps/ec2/'
                                      'USD/current/ec2-reservedinstance/3%20year/All%20Upfront/EU%20(Frankfurt)/'
                                      'Windows/Shared/index.json?timestamp=1624366372247',
                "storage": "https://calculator.aws/pricing/2.0/meteredUnitMaps/ec2/USD/current/ebs-calculator.json"
            }
        else:
            raise ValueError("Invalid operating system: Linux or Windows is expected")

    def get_compute(self) -> ComputeTariffMenu:
        menu = self._on_demand_compute()
        menu.config_2_4_80_1yr = self._one_year_compute_2_4()
        menu.config_2_4_80_3yr = self._three_year_compute_2_4()
        menu.config_2_8_160_1yr = self._one_year_compute_2_8()
        menu.config_2_8_160_3yr = self._three_year_compute_2_8()
        return menu

    def _scrap_compute_ssd(self, driver: WebDriver) -> SsdComputeTariff:
        # TODO: move ssd for compute uploading here
        pass

    def _three_year_compute_2_4(self) -> ComputeTariff:
        price_2_4 = self._three_years_reverve_price(self._url["three_year_reserve"], "c5.large")
        return self._create_tariff(service=Service.CONFIG_2_4_80,
                                   commitment=Commitment.THREE_YEAR,
                                   comment="Region Frankfurt, all upfront",
                                   price=price_2_4 + 80 * self._per_gb_ssd_monthly_price(),
                                   oper_sys=self._oper_sys)

    def _three_year_compute_2_8(self) -> ComputeTariff:
        price_2_8 = self._three_years_reverve_price(self._url["three_year_reserve"], "m5n.large")
        return self._create_tariff(service=Service.CONFIG_2_8_160,
                                   commitment=Commitment.THREE_YEAR,
                                   comment="Region Frankfurt, all upfront",
                                   price=price_2_8 + 160 * self._per_gb_ssd_monthly_price(),
                                   oper_sys=self._oper_sys)

    def _one_year_compute_2_4(self) -> ComputeTariff:
        price_2_4 = self._one_year_reserve_price(self._url["one_year_reserve"], "c5.large")

        return self._create_tariff(service=Service.CONFIG_2_4_80,
                                   commitment=Commitment.ONE_YEAR,
                                   comment="Region Frankfurt, all upfront",
                                   price=price_2_4 + 80 * self._per_gb_ssd_monthly_price(),
                                   oper_sys=self._oper_sys)

    def _one_year_compute_2_8(self) -> ComputeTariff:
        price_2_8 = self._one_year_reserve_price(self._url["one_year_reserve"], "m5n.large")
        return self._create_tariff(service=Service.CONFIG_2_8_160,
                                   commitment=Commitment.ONE_YEAR,
                                   comment="Region Frankfurt, all upfront",
                                   price=price_2_8 + 160 * self._per_gb_ssd_monthly_price(),
                                   oper_sys=self._oper_sys)

    def _on_demand_compute(self) -> ComputeTariffMenu:
        sku_json = self._read_json(self._url["on_demand"])
        hourly_2_4 = self._sku_price(sku_json, "aws:ec2:instanceType", "c5.large")
        hourly_2_8 = self._sku_price(sku_json, "aws:ec2:instanceType", "m5n.large")
        ssd_price = self._per_gb_ssd_monthly_price()
        monthly_price_2_4_80 = AmazonComputeUploader.HOURS_IN_MONTH * hourly_2_4 + 80 * ssd_price
        monthly_price_2_8_160 = AmazonComputeUploader.HOURS_IN_MONTH * hourly_2_8 + 160 * ssd_price
        return ComputeTariffMenu(oper_system="Linux",
                                 config_2_4_80=self._create_tariff(service=Service.CONFIG_2_4_80,
                                                                   commitment=Commitment.ON_DEMAND,
                                                                   price=monthly_price_2_4_80,
                                                                   oper_sys=self._oper_sys,
                                                                   comment="Config: c5.large, Region: Europe (Frankfurt), processor: Intel Xeon Platinum 8124M"),
                                 config_2_4_80_1yr=self.empty_compute_tariff(Service.CONFIG_2_4_80, Commitment.ONE_YEAR),
                                 config_2_4_80_3yr=self.empty_compute_tariff(Service.CONFIG_2_4_80, Commitment.THREE_YEAR),
                                 config_2_8_160=self._create_tariff(service=Service.CONFIG_2_8_160,
                                                                    commitment=Commitment.ON_DEMAND,
                                                                    price=monthly_price_2_8_160,
                                                                    oper_sys=self._oper_sys,
                                                                    comment="Config: m5n.large, Region: Europe (Frankfurt), processor: Intel Xeon Family p"),
                                 config_2_8_160_1yr=self.empty_compute_tariff(Service.CONFIG_2_8_160, Commitment.ONE_YEAR),
                                 config_2_8_160_3yr=self.empty_compute_tariff(Service.CONFIG_2_8_160, Commitment.THREE_YEAR))

    def _per_gb_ssd_monthly_price(self) -> float:
        if self.ssd_price is not None:
            return self.ssd_price
        sku_json = self._read_json(self._url["storage"])
        try:
            # TODO: replace with safe json
            self.ssd_price = float(sku_json["regions"]["EU (Frankfurt)"]["Storage General Purpose GB Mo"]["price"])
        except Exception as ex:
            print("Fail to find SSD tariff in regions->Frankfurt->Storage->price")
            print(str(ex))
            self.ssd_price = AmazonComputeUploader.DEFAULL_SSD_PRICE
        return self.ssd_price

    def _one_year_reserve_price(self, url, config_name: str):
        return self._compute_reserve_price(url, config_name, AmazonComputeUploader.MONTHS_IN_YEAR)

    def _three_years_reverve_price(self, url, config_name: str):
        return self._compute_reserve_price(url, config_name, 3 * AmazonComputeUploader.MONTHS_IN_YEAR)

    def _compute_reserve_price(self, url, config_name: str, months_in_period: int):
        sku_json = self._read_json(url)
        # TODO: replace with safe json
        ffurt_region = sku_json["regions"]["EU (Frankfurt)"]
        config = self._instance_by_name(ffurt_region, config_name)
        year_price = self.parse_price(config["riupfront:PricePerUnit"])
        return year_price / months_in_period

    def _instance_by_name(self, sku_list: Dict[str, Dict], inst_name: str) -> Dict[str, str]:
        found_sku = [sku for sku in sku_list.values() if sku["Instance Type"] == inst_name]
        if len(found_sku) == 1:
            return found_sku[0]
        else:
            raise ValueError(f"1 sku is expected for instance type [{inst_name}]")

    def _scrap_compute(self, driver: WebDriver) -> ComputeTariffMenu:
        raise NotSupportedException("Not supported since Amazon is loaded by API not by scrapping")

    def _scrap_storage(self, driver: WebDriver) -> StorageTariffMenu:
        raise NotSupportedException("Not supported since Amazon is loaded by API not by scrapping")


class AmazonStorageUploader(AmazonScrapperBase):

    def __init__(self, url):
        super().__init__(url)

    def get_storage(self) -> StorageTariffMenu:
        def filter_func(obj):
            return json_attr(obj, "attributes -> aws:s3:startRange") == "0"

        sku_json = self._read_json(self._url)
        cold_volume_price = self._sku_price(sku_json, "aws:s3:volumeType", "Standard - Infrequent Access")
        cold_write_one_operation = self._sku_price(sku_json, "aws:s3:groupDescription", "PUT/COPY/POST or LIST requests to Standard-Infrequent Access")
        cold_read_one_operation = self._sku_price(sku_json, "aws:s3:groupDescription", "GET and all other requests  to Standard-Infrequent Access")

        # std storage
        std_volume_price = self._sku_price(sku_json, "aws:s3:volumeType", "Standard", filter_func)
        std_write_one_operation = self._sku_price(sku_json, "aws:s3:groupDescription", "PUT/COPY/POST or LIST requests")
        std_read_one_operation = self._sku_price(sku_json, "aws:s3:groupDescription", "GET and all other requests")

        return StorageTariffMenu(
            std_per_space=self._storage_tariff(price=std_volume_price,
                                               service=StorageServiceType.PER_SPACE,
                                               storage_type=StorageType.STD,
                                               comment="S3 Intelligent - Tiering, Infrequent Access Tier, All Storage / Month"),
            std_read=self._storage_tariff(price=std_read_one_operation * 10000,
                                          service=StorageServiceType.READ,
                                          storage_type=StorageType.STD,
                                          comment="GET and all other requests"),
            std_write=self._storage_tariff(price=1000 * std_write_one_operation,
                                           service=StorageServiceType.WRITE,
                                           storage_type=StorageType.STD,
                                           comment="PUT/COPY/POST or LIST requests, price per 1000 operations"),
            cold_per_space=self._storage_tariff(price=cold_volume_price,
                                                service=StorageServiceType.PER_SPACE,
                                                storage_type=StorageType.COLD,
                                                comment="S3 Intelligent - Tiering, Infrequent Access Tier, All Storage / Month"),
            cold_read=self._storage_tariff(price=10000 * cold_read_one_operation,
                                           service=StorageServiceType.READ,
                                           storage_type=StorageType.COLD,
                                           comment="GET and all other requests, price per 10000 requests"),
            cold_write=self._storage_tariff(price=1000 * cold_write_one_operation,
                                            service=StorageServiceType.WRITE,
                                            storage_type=StorageType.COLD,
                                            comment="PUT/COPY/POST or LIST requests to Standard-Infrequent Access, price per 1000 operations")
        )

    def _scrap_compute(self, driver: WebDriver) -> ComputeTariffMenu:
        pass

    def _scrap_storage(self, driver: WebDriver) -> StorageTariffMenu:
        pass

    def _scrap_compute_ssd(self, driver: WebDriver) -> SsdComputeTariff:
        pass
