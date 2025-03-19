from unittest import TestCase

from core.utils import *
from selectel.selectel_compute import SelectedLinuxComputeFactory
from selectel.selectel_storage import SelectelStorageTariffFactory


class TestSelectel(TestCase):

    def test_linux_compute(self):
        ex_rate = SelectedLinuxComputeFactory.USD_RUB_rate
        vat = 0.2
        s = SelectedLinuxComputeFactory('Selectel')
        menu = s.get_compute()
        self.assertAlmostEqual(2618.62 / ex_rate / (1 + vat), menu.config_2_4_80.price)
        self.assertAlmostEqual(4418.09 / ex_rate / (1 + vat), menu.config_2_8_160.price)
        assert_no_vat(menu)
        assert_linux(menu)
        assert_company_name(menu, "Selectel")

    def test_storage(self):
        ex_rate = SelectedLinuxComputeFactory.USD_RUB_rate
        vat = 0.2
        sel = SelectelStorageTariffFactory('Selectel')
        menu = sel.get_storage()
        self.assertAlmostEqual(1.43 / (1 + vat) / ex_rate, menu.std_per_space.price)
        self.assertAlmostEqual(0.31 / (1 + vat) / ex_rate, menu.std_write.price)
        self.assertAlmostEqual(0.24 / (1 + vat) / ex_rate, menu.std_read.price)

        self.assertAlmostEqual(0.7 / (1 + vat) / ex_rate, menu.cold_per_space.price)
        self.assertAlmostEqual(0.62 / (1 + vat) / ex_rate, menu.cold_read.price)
        self.assertAlmostEqual(0.62 / (1 + vat) / ex_rate, menu.cold_write.price)
