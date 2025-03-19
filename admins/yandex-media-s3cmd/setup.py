from setuptools import setup

setup(
    name="yandex-media-s3cmd",
    version="1",
    author_email="media-admin@yandex-team.ru",
    description=("Debianization for s3cmd"),
    license="GPL",
    url="https://github.yandex-team.ru/admins/yandex-media-s3cmd",
    install_requires=['s3cmd'],
)
