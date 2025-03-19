"""Setup.py for rotate_s3_backups"""
import os
from setuptools import setup
os.environ["NOSE_LOGCAPTURE"] = "True"

setup(
    name="rotate-s3-backups",
    version="1",
    author="Sergey Kacheev",
    author_email="skacheev@yandex-team.ru",
    description=("S3 backup rotating script"),
    license="GPL",
    url="https://github.yandex-team.ru/admins/rotate-s3-backups",
    packages=["s3_rotate"],
    scripts=['rotate-s3-backups'],
    install_requires=[
        'requests',
        'pyyaml',
        'kazoo',
        'dotmap',
        ],
    test_suite='nose.collector',
    tests_require=['nose'],
)
