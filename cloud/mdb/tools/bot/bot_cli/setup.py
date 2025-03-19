#!python
import setuptools

with open("README.md", "r") as f:
    descr = f.read()

setuptools.setup(
    name="bh",
    version='1.0',
    author="asalimonov",
    scripts=['bh.py'],
    author_email="asalimonov@yandex-team.ru",
    description="CLI for REST API for bot.yandex-team.ru",
    long_description=descr,
    url="https://a.yandex-team.ru/arc/trunk/arcadia/cloud/mdb/tools/bot_cli",
    # packages=['bh'],
    install_requires=['bot_lib', 'pandas', 'tabulate', 'openpyxl', 'click'],
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
    ],
)
