import os
import tarfile

import library.python.testing.yapackage as yapackage


def test_tar():
    res = yapackage.run_ya_package(["devtools/ya/package/tests/fat/data/package.json"], version=1, package_type=yapackage.PackageType.Tar)
    expected_tar_paths = [
        "destination/hello_world",
    ]
    with tarfile.open(os.path.join(res.output_root, "yandex-package-test.1.tar.gz")) as tf:
        for expected in expected_tar_paths:
            assert tf.getmember(expected)


def test_debian():
    res = yapackage.run_ya_package(["devtools/ya/package/tests/fat/data/package.json"], version=1, package_type=yapackage.PackageType.Debian)
    expected_tar_paths = [
        "yandex-package-test_1_amd64.build", "yandex-package-test_1_amd64.changes", "yandex-package-test_1_amd64.deb"
    ]
    with tarfile.open(os.path.join(res.output_root, "yandex-package-test.1.tar.gz")) as tf:
        for expected in expected_tar_paths:
            assert tf.getmember(expected)
