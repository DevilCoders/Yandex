import logging
import os

from library.python.testing import recipe

logger = logging.getLogger("maps.pylibs.recipes.gdal")


def start(argv):
    root = os.getenv('ARCADIA_SOURCE_ROOT')

    gdal_data = root + '/contrib/libs/gdal/data'
    proj_data = root + '/contrib/libs/proj/data'

    recipe.set_env("GDAL_DATA", gdal_data)
    recipe.set_env("PROJ_LIB", proj_data)
    recipe.set_env("PROJ_DIR", proj_data)


def stop(argv):
    pass


def main():
    recipe.declare_recipe(start, stop)
