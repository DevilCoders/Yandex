from unittest import TestCase

from amazon.aws import AmazonComputeUploader, AmazonStorageUploader
from core.tariff import OperSys
from core.utils import *


#
# Url for reserved instances
# https://aws.amazon.com/ru/ec2/pricing/reserved-instances/pricing/
#
class TestAws(TestCase):

    # https://calculator.aws/#/createCalculator/EC2?nc2=h_ql_pr_calc - to check on-demand configurations
    # https://aws.amazon.com/ru/ec2/pricing/reserved-instances/pricing/ - to check reserved configurations
    def test_linux_compute(self):
        aws = AmazonComputeUploader(OperSys.Linux)
        menu = aws.get_compute()
        self.assertAlmostEqual(menu.config_2_4_80.price, 0.097 * 720 + 0.119 * 80, delta=0.01)  # совпадает с слайдом
        self.assertAlmostEqual(menu.config_2_4_80_1yr.price, 500 / 12 + 0.119 * 80, delta=0.01)
        self.assertAlmostEqual(menu.config_2_4_80_3yr.price, 958 / 36 + 0.119 * 80, delta=0.01)

        self.assertAlmostEqual(menu.config_2_8_160.price, 0.141 * 720 + 160 * 0.119, delta=0.01)
        self.assertAlmostEqual(menu.config_2_8_160_1yr.price, 726 / 12 + 0.119 * 160, delta=0.01)
        self.assertAlmostEqual(menu.config_2_8_160_3yr.price, 1393 / 36 + 0.119 * 160, delta=0.01)
        assert_no_vat(menu)
        assert_linux(menu)
        assert_company_name(menu, "Amazon")

    def test_windows_compute(self):
        aws = AmazonComputeUploader(OperSys.Windows)
        menu = aws.get_compute()
        self.assertAlmostEqual(menu.config_2_4_80.price, 0.189 * 720 + 0.119 * 80, delta=0.01)
        self.assertAlmostEqual(menu.config_2_4_80_1yr.price, 1306 / 12 + 0.119 * 80, delta=0.01)
        self.assertAlmostEqual(menu.config_2_4_80_3yr.price, 3376 / 36 + 0.119 * 80, delta=0.01)

        self.assertAlmostEqual(menu.config_2_8_160.price, 0.233 * 720 + 160 * 0.119, delta=0.01)
        self.assertAlmostEqual(menu.config_2_8_160_1yr.price, 1532 / 12 + 0.119 * 160, delta=0.01)
        self.assertAlmostEqual(menu.config_2_8_160_3yr.price, 3811 / 36 + 0.119 * 160, delta=0.01)
        assert_no_vat(menu)
        assert_windows(menu)
        assert_company_name(menu, "Amazon")

    # https://aws.amazon.com/s3/pricing/
    def test_aws_storage(self):
        aws = AmazonStorageUploader(url="https://a0.p.awsstatic.com/pricing/1.0/s3/region/eu-central-1/index.json")
        menu = aws.get_storage()
        self.assertAlmostEqual(menu.std_per_space.price, 0.0245, delta=0.01)
        self.assertAlmostEqual(menu.cold_per_space.price, 0.0135, delta=0.01)
        self.assertAlmostEqual(menu.std_write.price, 0.0054, delta=0.001)
        self.assertAlmostEqual(menu.cold_write.price, 0.01, delta=0.005)
        self.assertAlmostEqual(menu.std_read.price, 0.00043 * 10, delta=0.001)  # multiply but 10 because on website price is per 1000 operations
        self.assertAlmostEqual(menu.cold_read.price, 0.001 * 10, delta=0.001)
        assert_company_name(menu, "Amazon")
