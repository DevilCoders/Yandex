#!python
import setuptools

with open("README.md", "r") as f:
    descr = f.read()

setuptools.setup(
    name="bot_lib",
    version='1.0',
    author="asalimonov",
    author_email="asalimonov@yandex-team.ru",
    description="A client for REST API for bot.yandex-team.ru",
    long_description=descr,
    url="https://a.yandex-team.ru/arc/trunk/arcadia/cloud/mdb/tools/bot",
    packages=['bot_lib'],
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
    ],
)
