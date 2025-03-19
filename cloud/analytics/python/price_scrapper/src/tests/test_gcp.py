from unittest import TestCase

from core.utils import *

from google_cloud.google_compute import *
from google_cloud.google_compute_scrapper import GoogleComputeScrapper
from google_cloud.google_storage import *
from google_cloud.google_storage_scrapper import *


class TestGoogle(TestCase):

    def test_linux_compute_2_4_80(self):
        scr = GoogleComputeScrapper()
        google = GoogleLinuxComputeTariffFactory('Google', scr)
        menu = google.get_compute()

        self.assertAlmostEqual(menu.config_2_4_80.price, 67.63, delta=0.01)
        self.assertAlmostEqual(menu.config_2_4_80_1yr.price, 48.65, delta=0.01)
        self.assertAlmostEqual(menu.config_2_4_80_3yr.price, 39.41, delta=0.01)

        self.assertAlmostEqual(menu.config_2_8_160.price, 94.80, delta=0.01)
        self.assertAlmostEqual(menu.config_2_8_160_1yr.price, 71.80, delta=0.01)
        self.assertAlmostEqual(menu.config_2_8_160_3yr.price, 60.61, delta=0.01)

        assert_vat(menu, False)
        assert_company_name(menu, "Google")
        assert_operating_system(menu, OperSys.Linux)

    def test_win_compute(self):
        scr = GoogleComputeScrapper()
        win_factory = GoogleWindowsComputeTariffFactory('Google', scr)
        menu = win_factory.get_compute()

        self.assertAlmostEqual(menu.config_2_4_80.price, 67.63 + GoogleComputeScrapper.WinLicenceCost, delta=0.01)
        self.assertAlmostEqual(menu.config_2_4_80_1yr.price, 48.65 + GoogleComputeScrapper.WinLicenceCost, delta=0.01)
        self.assertAlmostEqual(menu.config_2_4_80_3yr.price, 39.41 + GoogleComputeScrapper.WinLicenceCost, delta=0.01)

        self.assertAlmostEqual(menu.config_2_8_160.price, 94.80 + GoogleComputeScrapper.WinLicenceCost, delta=0.01)
        self.assertAlmostEqual(menu.config_2_8_160_1yr.price, 71.80 + GoogleComputeScrapper.WinLicenceCost, delta=0.01)
        self.assertAlmostEqual(menu.config_2_8_160_3yr.price, 60.61 + GoogleComputeScrapper.WinLicenceCost, delta=0.01)
        assert_vat(menu, False)
        assert_company_name(menu, "Google")
        assert_operating_system(menu, OperSys.Windows)

    # Class A - object adds, bucket and object listings (write)
    # Class B - object gets, retrieving bucket and object metadata (read)
    def test_std_storage(self):
        scr = GoogleStorageScrapper('https://cloud.google.com/storage/pricing#europe')
        f = GoogleStorageTariffFactory('Google', scr)
        menu = f.get_storage()
        self.assertAlmostEqual(menu.std_per_space.price, 0.023, delta=0.001)
        self.assertAlmostEqual(menu.std_read.price, 0.004, delta=0.001)
        self.assertAlmostEqual(menu.std_write.price, 0.005, delta=0.001)
        self.assertAlmostEqual(menu.cold_per_space.price, 0.006, delta=0.001)
        self.assertAlmostEqual(menu.cold_read.price, 0.05, delta=0.001)
        self.assertAlmostEqual(menu.cold_write.price, 0.01, delta=0.001)
        assert_no_vat(menu)
        assert_company_name(menu, "Google")
        assert_storage_pricing_units(menu)
