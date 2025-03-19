import unittest

from tests.test_aws import TestAws
from tests.test_azure import TestAzure
from tests.test_gcp import TestGoogle
from tests.test_selectel import TestSelectel
from tests.test_vk import TestVk
from tests.test_yndx import TestYandex

providers_suite = unittest.TestSuite()
providers_suite.addTest(unittest.makeSuite(TestAws))
providers_suite.addTest(unittest.makeSuite(TestAzure))
providers_suite.addTest(unittest.makeSuite(TestYandex))
providers_suite.addTest(unittest.makeSuite(TestGoogle))
providers_suite.addTest(unittest.makeSuite(TestSelectel))
providers_suite.addTest(unittest.makeSuite(TestVk))

runner = unittest.TextTestRunner(verbosity=2)
runner.run(providers_suite)
