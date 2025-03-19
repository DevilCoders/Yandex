from setuptools import find_packages
from setuptools import setup

setup(
    name="yc_marketplace_common",
    version="0.2.0",
    description="",
    packages=find_packages(exclude="tests"),
    include_package_data=True,
    scripts=[
        "bin/yc-marketplace-manage",
    ],
)
