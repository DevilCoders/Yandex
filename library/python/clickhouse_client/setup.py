#!/usr/bin/env python
# coding: utf8

from setuptools import setup, find_packages


def debian_version():
    try:
        import debian.changelog
    except Exception:
        # Fallback
        import re
        data = open("debian/changelog").read(65535)
        version = re.search(r"^[^ ]+ \(([^)]+)\) .*urgency=", data).group(1)
        return version
    else:
        changelog = debian.changelog.Changelog(open('debian/changelog'))
        return changelog.version.full_version


setup_kwargs = dict(
    name='python-clickhouse-client',
    version=debian_version(),
    description=("Python client for Clickhouse. "
                 "See PEP-249 for details."),
    author="Dmitriy Fedorov",
    author_email="dmifedorov@yandex-team.ru",
    packages=find_packages(),
)


if __name__ == '__main__':
    setup(**setup_kwargs)
