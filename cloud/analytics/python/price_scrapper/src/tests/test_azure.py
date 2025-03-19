from unittest import TestCase

from azure.azure_compute import AzureLinuxTariffFactory, AzureWindowsTariffFactory
from azure.azure_compute_scrapper import AzureComputeScrapper
from azure.azure_storage import AzureStorageTariffFactory
from azure.azure_storage_scrapper import AzureStorageScrapperNew
from core.utils import *


class TestAzure(TestCase):

    def test_linux_compute(self):
        scr = AzureComputeScrapper()
        azure = AzureLinuxTariffFactory('Azure', scr)
        menu = azure.get_compute()
        self.assert_tariffs(menu)
        assert_vat(menu, False)
        assert_linux(menu)
        assert_company_name(menu, "Azure")

    def test_windows_compute(self):
        scr = AzureComputeScrapper()
        azure = AzureWindowsTariffFactory('Azure', scr)
        menu = azure.get_compute()
        self.assert_tariffs(menu)
        assert_vat(menu, False)
        assert_windows(menu)
        assert_company_name(menu, "Azure")

    def assert_tariffs(self, menu: ComputeTariffMenu, ):
        ssd_gb_price = 4.8 / 64  # from pricing calculator on website: 4.8$ for 64Gb
        ssd_included = {
            "A2 v2": 20,
            "D2 v3": 50
        }
        self.assertAlmostEqual(menu.config_2_4_80.price, 63.51 + ssd_gb_price * (80 - ssd_included['A2 v2']), delta=0.1)
        self.assertAlmostEqual(menu.config_2_8_160.price, 78.11 + ssd_gb_price * (160 - ssd_included['D2 v3']), delta=0.1)
        self.assertAlmostEqual(menu.config_2_8_160_1yr.price, 51.1657 + ssd_gb_price * (160 - ssd_included['D2 v3']), delta=0.1)
        self.assertAlmostEqual(menu.config_2_8_160_3yr.price, 33.7479 + ssd_gb_price * (160 - ssd_included['D2 v3']), delta=0.1)

    def test_storage(self):
        scr = AzureStorageScrapperNew()
        std = AzureStorageTariffFactory('Azure', scr)
        menu = std.get_storage()
        # test cold storage
        self.assertAlmostEqual(menu.std_per_space.price, 0.02, delta=0.0001)
        self.assertAlmostEqual(menu.std_read.price, 0.006, delta=0.00001)
        self.assertAlmostEqual(menu.std_write.price, 0.07 / 10, delta=0.00001)  # на сайте указано за 10 тыс
        # test std storage
        self.assertAlmostEqual(menu.cold_per_space.price, 0.01, delta=0.1)
        self.assertAlmostEqual(menu.cold_read.price, 0.013, delta=0.1)
        self.assertAlmostEqual(menu.cold_write.price, 0.013, delta=0.1)
        assert_company_name(menu, "Azure")
        assert_no_vat(menu)
        assert_storage_pricing_units(menu)
