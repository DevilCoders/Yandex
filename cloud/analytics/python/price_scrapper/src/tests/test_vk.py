from unittest import TestCase

from core.hard_numbered_tariffs import HardNumberedComputeTariffFactory
from core.utils import *
from vk.vk_compute import VkLinuxComputeTariffFactory, VkWindowsComputeTariffFactory


class TestVk(TestCase):

    def test_linux_compute(self):
        ex_rate = HardNumberedComputeTariffFactory.USD_RUB_rate
        vat = 0.2
        s = VkLinuxComputeTariffFactory("VK")
        menu = s.get_compute()
        self.assertAlmostEqual(2580 / ex_rate / (1 + vat), menu.config_2_4_80.price)
        self.assertAlmostEqual(4140 / ex_rate / (1 + vat), menu.config_2_8_160.price)
        assert_no_vat(menu)
        assert_linux(menu)
        assert_company_name(menu, "VK")

    def test_windows_compute(self):
        ex_rate = HardNumberedComputeTariffFactory.USD_RUB_rate
        vat = 0.2
        s = VkWindowsComputeTariffFactory("VK")
        menu = s.get_compute()
        self.assertAlmostEqual(2944.98 / ex_rate / (1 + vat), menu.config_2_4_80.price)
        self.assertAlmostEqual(4504.98 / ex_rate / (1 + vat), menu.config_2_8_160.price)
        assert_no_vat(menu)
        assert_windows(menu)
        assert_company_name(menu, "VK")


