import atexit
import fileinput
import sys
import os

from setuptools import setup, find_packages
from setuptools.command.test import test


class PyTest(test):
    def finalize_options(self):
        test.finalize_options(self)
        self.test_args = ["tests"]
        self.test_suite = True

    def run_tests(self):
        import pytest

        try:
            sys.exit(pytest.main(self.test_args))
        finally:
            # setup.py somehow conflicts with atexit module (registered handlers are executed when most of modules are
            # already collected by GC). Use this ugly hack to make `python setup.py test` work.
            atexit._run_exitfuncs()


def get_requirements(file_name):
    # FIXME: use requirements-parser package
    return [str(line) for line in fileinput.input([file_name])]


if __name__ == "__main__":
    install_requirements = get_requirements("requirements.txt")

    # To install a package from pypi on older versions of linux and on macos,
    # you must disable linux-specific dependencies (for example, systemd)
    disable_linux_requirements = os.getenv('DISABLE_LINUX_REQUIREMENTS', False)

    if sys.platform == "linux" and not disable_linux_requirements:
        install_requirements += get_requirements("linux-requirements.txt")

    setup(
        name="yc_common",
        version="0.4.0",
        description="Yandex Cloud common modules",
        author_email="devel@yandex-team.ru",
        install_requires=install_requirements,
        packages=find_packages(exclude=("tests",)),
        cmdclass={"test": PyTest},
        tests_require=get_requirements("test-requirements.txt"),
    )
