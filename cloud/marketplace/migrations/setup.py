from setuptools import find_packages
from setuptools import setup

if __name__ == "__main__":
    setup(
        name="yc_marketplace_migrations",
        version="0.1.9",
        description="Yandex Cloud Marketplace Migrations",
        author_email="devel@yandex-team.ru",
        install_requires=[],
        packages=find_packages(),
        include_package_data=True,
    )
