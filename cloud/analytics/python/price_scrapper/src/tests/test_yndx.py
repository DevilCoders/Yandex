from unittest import TestCase

from core.utils import *
from yndx.yndx_compute import *
from yndx.yndx_compute_scrapper import YndxComputeScrapper
from yndx.yndx_storage import YandexStorageTariffFactory
from yndx.yndx_storage_scrapper import YndxStorageScrapper


class TestYandex(TestCase):

    def test_compute_linux(self):
        scr = YndxComputeScrapper('https://cloud.yandex.com/en/docs/compute/pricing')
        yt = YndxLinuxComputeTariffFactory(scr)
        menu = yt.get_compute()
        # Test 2-4-80 Config
        self.assertAlmostEqual(menu.config_2_4_80.price, 28.75, delta=0.01)
        self.assertAlmostEqual(menu.config_2_4_80_1yr.price,
                               menu.config_2_4_80.price * YndxLinuxComputeTariffFactory.MULT_ONE_YEAR,
                               delta=0.01)
        self.assertAlmostEqual(menu.config_2_4_80_3yr.price,
                               menu.config_2_4_80.price * YndxLinuxComputeTariffFactory.MUL_THREE_YEARS,
                               delta=0.01)
        assert_no_vat(menu)
        assert_linux(menu)
        assert_company_name(menu, "Yandex")
        # TODO: Test that commitment and service is set according to enums

    def test_linux_2_8_160(self):
        scr = YndxComputeScrapper('https://cloud.yandex.com/en/docs/compute/pricing')
        yt = YndxLinuxComputeTariffFactory(scr)
        menu = yt.get_compute()
        self.assertAlmostEqual(menu.config_2_8_160.price, 43.69, delta=0.01)
        self.assertAlmostEqual(menu.config_2_8_160_1yr.price,
                               menu.config_2_8_160.price * YndxLinuxComputeTariffFactory.MULT_ONE_YEAR, delta=0.01)
        self.assertAlmostEqual(menu.config_2_8_160_3yr.price,
                               menu.config_2_8_160.price * YndxLinuxComputeTariffFactory.MUL_THREE_YEARS, delta=0.01)
        assert_no_vat(menu)
        assert_linux(menu)
        assert_company_name(menu, "Yandex")

    def test_compute_windows(self):
        scr = YndxComputeScrapper('https://cloud.yandex.com/en/docs/compute/pricing')
        lin_yndx = YndxLinuxComputeTariffFactory(scr)
        yndx = YndxWinComputeTariffFactory(lin_yndx)
        menu = yndx.get_compute()
        self.assertAlmostEqual(menu.config_2_4_80.price, 55.70, delta=0.01)
        self.assertAlmostEqual(menu.config_2_8_160.price, 70.65, delta=0.01)
        assert_no_vat(menu)
        assert_windows(menu)
        assert_company_name(menu, "Yandex")

    def test_storage(self):
        scr = YndxStorageScrapper('https://cloud.yandex.com/en/docs/storage/pricing')
        ynd_stor = YandexStorageTariffFactory('Yandex', scr)
        menu = ynd_stor.get_storage()
        # assert prices
        self.assertAlmostEqual(menu.std_per_space.price, 0.016167, delta=0.0001)
        self.assertAlmostEqual(menu.cold_per_space.price, 0.008605, delta=0.0001)
        self.assertAlmostEqual(menu.std_read.price, 0.003129, delta=0.0001)
        self.assertAlmostEqual(menu.std_write.price, 0.003912, delta=0.0001)
        self.assertAlmostEqual(menu.cold_read.price, 0.007823, delta=0.0001)
        self.assertAlmostEqual(menu.cold_write.price, 0.009518, delta=0.0001)

        # assert correct tariff params
        self.assert_storage_params('std_per_gb', menu.std_per_space)
        self.assert_storage_params('std_read', menu.std_read)
        self.assert_storage_params('std_write', menu.std_write)
        self.assert_storage_params('cold_per_gb', menu.cold_per_space)
        self.assert_storage_params('cold_read', menu.cold_read)
        self.assert_storage_params('cold_write', menu.cold_write)

    def assert_storage_params(self, expected_repr_str: str, actual: StorageTariff):
        TariffFactoryBase.assert_storage_repr_str(expected_repr_str)
        expected_service = TariffFactoryBase.get_stor_service_from(expected_repr_str)
        expected_stor_type = TariffFactoryBase.get_stor_type_from(expected_repr_str)
        if actual.storage_type != expected_stor_type:
            raise Exception(f"Actual <{actual.storage_type}> and expected <{expected_stor_type}> storage "
                            f"types don't match for representation string <{expected_repr_str}>")
        if actual.service != expected_service:
            raise Exception(f"Actual <{actual.service}> and expected <{expected_service}> storage "
                            f"types don't match for representation string <{expected_repr_str}>")
