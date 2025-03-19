#!/usr/bin/env python
# coding: utf-8

import distutils.command.clean
import os
import shutil
import setuptools


DEFAULT_DESTINATION = None
PUBLISH_DIR = "distribute/"


def get_arcadia_root():
    arcadia_root_candidate = os.path.dirname(os.path.abspath(__file__))
    retries_count = 10
    for _ in range(retries_count):
        if os.path.exists(os.path.join(arcadia_root_candidate, ".arcadia.root")):
            return arcadia_root_candidate
        arcadia_root_candidate = os.path.dirname(arcadia_root_candidate)
    raise RuntimeError("Unable to find .arcadia.root!")


class CopyModules(setuptools.Command):
    description = "copy proto files into the root directory"
    user_options = []

    # do not remove these two methods
    # removing them causes a runtime error

    def initialize_options(self):
        pass

    def finalize_options(self):
        pass

    def _create_module_dir(self, path):
        full_dest_path = PUBLISH_DIR + os.path.join(*path)
        if not os.path.exists(full_dest_path):
            os.mkdir(full_dest_path)
        if not os.path.exists(os.path.join(full_dest_path, "__init__.py")):
            open(os.path.join(full_dest_path, "__init__.py"), "a").close()

    def run(self):
        arcadia_root = get_arcadia_root()

        os.mkdir(PUBLISH_DIR)

        yc_constants_dest = "yc_constants"
        relpaths = {
            "cloud/iam/codegen/python/lib": yc_constants_dest,
        }

        for relpath in relpaths:
            source_folder = os.path.join(arcadia_root, relpath)
            destination_folder = relpaths[relpath]
            if destination_folder == DEFAULT_DESTINATION:
                destination_folder = relpath

            parts = destination_folder.split("/")
            for p_size in range(1, len(parts) + 1):
                self._create_module_dir(parts[:p_size])

            for file_name in os.listdir(source_folder):
                if file_name == "setup.py":
                    continue
                if file_name.endswith(".proto") or file_name.endswith(".py"):
                    shutil.copyfile(
                        os.path.join(source_folder, file_name),
                        os.path.join(PUBLISH_DIR + destination_folder, file_name),
                    )

        shutil.copyfile(
            arcadia_root + "/cloud/iam/codegen/python/lib/setup.py",
            PUBLISH_DIR + "setup.py"
        )


class Clean(distutils.command.clean.clean):
    def run(self):
        from distutils.dir_util import remove_tree
        if os.path.exists(PUBLISH_DIR):
            remove_tree(PUBLISH_DIR)
        distutils.command.clean.clean.run(self)


setuptools.setup(
    name="yc_constants",
    version="0.1.0",
    description="Python library for YC constants (quotas, resources, permissions)",
    author="cloud-iam@",
    author_email="cloud-iam@yandex-team.ru",
    license="Yandex NDA",

    package_dir={"": "."},
    packages=setuptools.find_packages("."),

    install_requires=[
    ],

    classifiers=[
        "Programming Language :: Python",
        "Programming Language :: Python :: 2",
        "Programming Language :: Python :: 2.7",
        "Programming Language :: Python :: 3",
    ],

    cmdclass={
        "preprocess": CopyModules,
        "clean": Clean,
    }
)
