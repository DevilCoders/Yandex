#!/usr/bin/env python

import os
import subprocess
import sys

from distutils import dir_util
from distutils import log
from distutils.command.clean import clean as _clean
from setuptools import setup

if sys.version_info[0] == 3:
    # Python 3
    from distutils.command.build_py import build_py_2to3 as _build_py
else:
    # Python 2
    from distutils.command.build_py import build_py as _build_py


class clean(_clean):
    def run(self):
        dirpath, dirnames, filenames = next(os.walk("."))
        for dirname in dirnames:
            if dirname == "build" or dirname == "dist" or dirname.endswith("egg-info"):
                dir_util.remove_tree(os.path.join(dirpath, dirname))
        _clean.run(self)


setup(
    name="yc_snapshot_client",
    version="0.2.34",
    author="Anton Tyurin (noxiouz)",
    author_email="devel@yandex-team.ru",
    maintainer="Maxim Perevedentsev (lantame)",
    maintainer_email="devel@yandex-team.ru",
    description="Python client for Snapshot service",
    long_description="Python client for Snapshot service",
    license="LGPLv3+",
    cmdclass={"clean": clean, "build_py": _build_py},
    platforms=["Linux", "BSD", "MacOS"],
    include_package_data=True,
    zip_safe=False,
    packages=[
        "yc_snapshot_client",
        "yc_snapshot_client.snapshot_rpc",
    ],
    install_requires=open("./requirements.txt").read(),
    tests_require=open("./tests/requirements.txt").read(),
    test_suite="nose.collector",
    classifiers=[
        "Programming Language :: Python",
        # "Programming Language :: Python :: 2.6",
        "Programming Language :: Python :: 2.7",
        # "Programming Language :: Python :: 3.2",
        # "Programming Language :: Python :: 3.3",
        "Programming Language :: Python :: 3.5",
        "Programming Language :: Python :: Implementation :: PyPy",
        "Programming Language :: Python :: Implementation :: CPython",
        # "Development Status :: 1 - Planning",
        # "Development Status :: 2 - Pre-Alpha",
        # "Development Status :: 3 - Alpha",
        # "Development Status :: 4 - Beta",
        "Development Status :: 5 - Production/Stable",
        # "Development Status :: 6 - Mature",
        # "Development Status :: 7 - Inactive",
    ],
)
