#!/usr/bin/env python3

from unittest import mock, TestCase

from parameterized import parameterized

from bin import manage_apt_repos


class TestRepoManager(TestCase):

    def mock_response(self, *args):
        return args

    def test_unnecessary_repos_removed(self):
        with mock.patch("os.remove"):
            self.assertTrue(manage_apt_repos.unnecessary_repos_removed())

    def test_expected_exception_unnecessary_repos_removed(self):
        with mock.patch("os.remove") as mock_remove:
            mock_remove.side_effect = FileNotFoundError
            self.assertTrue(manage_apt_repos.unnecessary_repos_removed())

    def test_unexpected_exception_unnecessary_repos_removed(self):
        with mock.patch("os.remove") as mock_remove:
            mock_remove.side_effect = PermissionError
            self.assertFalse(manage_apt_repos.unnecessary_repos_removed())

    def test_clean_default_repos(self):
        with mock.patch("builtins.open"):
            self.assertTrue(manage_apt_repos.default_repos_cleaned())

    def test_exception_clean_default_repos(self):
        with mock.patch("builtins.open") as mock_open:
            mock_open.side_effect = PermissionError
            self.assertFalse(manage_apt_repos.default_repos_cleaned())

    @parameterized.expand([
        ("valid",
         {
             "yandex-cloud-common-focal-secure": {"branch": ["stable"], "arch": ["amd64", "all"]},
             "yandex-cloud-focal-secure": {"branch": ["stable", "testing"], "arch": ["amd64"]},
         },
         {
             "/etc/apt/sources.list.d/yandex-cloud-common-focal-secure-amd64-stable.list":
                 "deb https://yandex-cloud-common-focal-secure.dist.yandex.ru/yandex-cloud-common-focal-secure stable/amd64/\n",
             "/etc/apt/sources.list.d/yandex-cloud-common-focal-secure-stable.list":
                 "deb https://yandex-cloud-common-focal-secure.dist.yandex.ru/yandex-cloud-common-focal-secure stable/all/\n",
             "/etc/apt/sources.list.d/yandex-cloud-focal-secure-amd64-stable.list":
                 "deb https://yandex-cloud-focal-secure.dist.yandex.ru/yandex-cloud-focal-secure stable/amd64/\n",
             "/etc/apt/sources.list.d/yandex-cloud-focal-secure-amd64-testing.list":
                 "deb https://yandex-cloud-focal-secure.dist.yandex.ru/yandex-cloud-focal-secure testing/amd64/\n"}
         ),
    ])
    def test_generate_repos(self, case_name, repos, expected_result):
        self.assertDictEqual(manage_apt_repos.generate_repos(repos), expected_result)

    @parameterized.expand([
        ("missing branch",
         {"yandex-cloud-common-focal-secure": {"arch": ["amd64", "all"]}, },
         None),
        ("missing arch",
         {"yandex-cloud-common-focal-secure": {"branch": ["stable"]}, },
         None),
        ("wrong type branch",
         {"yandex-cloud-common-focal-secure": {"branch": "stable", "arch": ["amd64", "all"]}, },
         None),
        ("wrong type arch",
         {"yandex-cloud-common-focal-secure": {"branch": ["stable"], "arch": "amd64"}, },
         None),
        ("empty argument", None, None)
    ])
    def test_fail_generate_repos(self, case_name, repos, expected_result):
        self.assertEqual(manage_apt_repos.generate_repos(repos), expected_result)
