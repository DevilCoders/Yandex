from unittest import mock, TestCase

from parameterized import parameterized

import ycinfra


class TestYCInfra(TestCase):

    @parameterized.expand([
        ("valid", "a=1\nb=2", {'a': '1', 'b': '2'}),
        ("wrong format in one string", "a\nb=2", {}),
        ("wrong format", "a\nb", {}),
        ("empty content", "", {}),
    ])
    def test_get_os_release(self, case_name, file_content, expected_result):
        with mock.patch("builtins.open", mock.mock_open(read_data=file_content)):
            self.assertEqual(ycinfra.get_os_release(), expected_result)

    @parameterized.expand([
        ("valid", "DISTRIB_RELEASE=20.04\nDISTRIB_CODENAME=focal", "focal"),
        ("wrong format", "DISTRIB_RELEASE\nDISTRIB_CODENAME=focal", None),
        ("missing key", "DISTRIB_RELEASE\nDISTRIB_ID=Ubuntu", None),
        ("empty content", "", None),
        ("None content", None, None),
    ])
    def test_get_os_codename(self, case_name, file_content, expected_result):
        with mock.patch("builtins.open", mock.mock_open(read_data=file_content)):
            self.assertEqual(ycinfra.get_os_codename(), expected_result)
