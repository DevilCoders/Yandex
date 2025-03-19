from distutils.cmd import Command
import os
from setuptools import setup
from setuptools.command.build_py import build_py


def get_version():
    return open('VERSION').read().strip()


class ProtoBuildCommand(Command):
    description = "build protos"
    user_options = []

    def initialize_options(self):
        pass

    def finalize_options(self):
        pass

    def run(self):
        from grpc_tools import command
        command.build_package_protos(".")


class MakeModulesCommand(Command):
    description = "add __init__.py where appropriate"
    user_options = []

    def initialize_options(self):
        pass

    def finalize_options(self):
        pass

    def run(self):
        for root, dirs, files in os.walk('cloud'):
            if not os.path.exists(os.path.join(root, '__init__.py')):
                open(os.path.join(root, '__init__.py'), 'a').close()
        for root, dirs, files in os.walk('library'):
            if not os.path.exists(os.path.join(root, '__init__.py')):
                open(os.path.join(root, '__init__.py'), 'a').close()


class BuildPyCommand(build_py):
    def run(self):
        self.run_command('proto')
        self.run_command('make_modules')
        build_py.run(self)


setup(
    name='nbsclient',
    version=get_version(),
    description='Yandex NBS Python Library',
    author='Cloud NBS',
    author_email='nbs-dev@yandex-team.ru',
    url="https://wiki.yandex-team.ru/kikimr/projects/nbs/",
    license="Yandex NDA",
    classifiers=[
        "Programming Language :: Python",
        "Programming Language :: Python :: 2",
        "Programming Language :: Python :: 2.7",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.6",
    ],
    packages=[
        'cloud',
        'cloud.blockstore',
        'cloud.blockstore.public',
        'cloud.blockstore.public.api.grpc',
        'cloud.blockstore.public.api.protos',
        'cloud.blockstore.public.sdk',
        'cloud.blockstore.public.sdk.python',
        'cloud.blockstore.public.sdk.python.client',
        'cloud.blockstore.public.sdk.python.protos',
        'cloud.storage.core.protos',
        'library',
        'library.cpp',
        'library.cpp.lwtrace',
        'library.cpp.lwtrace.protos'
    ],
    setup_requires=[
        'grpcio-tools>=1.8.1',
    ],
    install_requires=[
        'protobuf>=3.3.0',
        'grpcio>=1.5.0',
        'googleapis-common-protos>=1.51.0',
        'requests>=2.22.0',
    ],
    cmdclass={
        'build_py': BuildPyCommand,
        'proto': ProtoBuildCommand,
        'make_modules': MakeModulesCommand,
    },
    options={
        'bdist_wheel': {'universal': True}
    },
)
