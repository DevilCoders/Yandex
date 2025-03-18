import test.tests.common as tests_common


class PackageType(object):
    Debian = 'debian'
    Tar = 'tar'


class RunTests(object):
    No = None
    Small = 't'
    SmallMedium = 'tt'
    SmallMediumLarge = 'ttt'


class YaPackageRunException(Exception):

    def __init__(self, result):
        self.result = result


def run_ya_package(packages, version=None, run_tests=RunTests.No, package_type=PackageType.Tar, check_call=True):
    assert run_tests in [RunTests.No, RunTests.Small, RunTests.SmallMedium, RunTests.SmallMediumLarge], "Use RunTests enum for run_tests param"
    assert package_type in [PackageType.Tar, PackageType.Debian], "Use PackageType enum for package_type param"
    args = []

    if version is not None:
        args += ["--custom-version", str(version)]
    if run_tests is not None:
        assert run_tests in []
        args += ["-{}".format(run_tests)]
    args += ["--{}".format(package_type)]
    if package_type == PackageType.Debian:
        args += ["--not-sign-debian"]
    res = tests_common.run_ya_package(packages, args=args, use_ya_bin=False, use_test_tool_bin=False, use_ymake_bin=False)
    if check_call and res.exit_code != 0:
        raise YaPackageRunException(res)
    return res
