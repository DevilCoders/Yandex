import os

import ydb.tests.library.common.yatest_common as yatest_common
from library.python.testing.recipe import declare_recipe, set_env

from cloud.disk_manager.test.recipe.image_file_server_launcher import ImageFileServerLauncher


def start(argv):
    image_file_path = yatest_common.source_path("cloud/disk_manager/test/images/image.img")
    image_file_server = ImageFileServerLauncher(image_file_path)
    image_file_server.start()
    set_env("DISK_MANAGER_RECIPE_IMAGE_FILE_SERVER_PORT", str(image_file_server.port))
    set_env("DISK_MANAGER_RECIPE_IMAGE_FILE_SIZE", str(image_file_server.image_file_size))
    set_env("DISK_MANAGER_RECIPE_IMAGE_FILE_CRC32", str(image_file_server.image_file_crc32))

    invalid_image_file_path = yatest_common.source_path("cloud/disk_manager/test/images/invalid_image.img")
    invalid_image_file_server = ImageFileServerLauncher(invalid_image_file_path)
    invalid_image_file_server.start()
    set_env("DISK_MANAGER_RECIPE_INVALID_IMAGE_FILE_SERVER_PORT", str(invalid_image_file_server.port))

    ubuntu1804_image_file_path = yatest_common.work_path("qcow2_images/ubuntu-18.04-minimal-cloudimg-amd64.img")
    if os.path.exists(ubuntu1804_image_file_path):
        ubuntu1804_image_file_server = ImageFileServerLauncher(ubuntu1804_image_file_path)
        ubuntu1804_image_file_server.start()
        set_env("DISK_MANAGER_RECIPE_QCOW2_UBUNTU1804_IMAGE_FILE_SERVER_PORT", str(ubuntu1804_image_file_server.port))
        set_env("DISK_MANAGER_RECIPE_QCOW2_UBUNTU1804_IMAGE_FILE_SIZE", "332595200")
        # size and crc32 after converting to raw image
        set_env("DISK_MANAGER_RECIPE_QCOW2_UBUNTU1804_IMAGE_SIZE", "2361393152")
        set_env("DISK_MANAGER_RECIPE_QCOW2_UBUNTU1804_IMAGE_CRC32", "2577917554")

    ubuntu1604_image_file_path = yatest_common.work_path("qcow2_images/ubuntu1604-ci-stable")
    if os.path.exists(ubuntu1604_image_file_path):
        ubuntu1604_image_file_server = ImageFileServerLauncher(ubuntu1604_image_file_path)
        ubuntu1604_image_file_server.start()
        set_env("DISK_MANAGER_RECIPE_QCOW2_UBUNTU1604_IMAGE_FILE_SERVER_PORT", str(ubuntu1604_image_file_server.port))
        set_env("DISK_MANAGER_RECIPE_QCOW2_UBUNTU1604_IMAGE_FILE_SIZE", "332595200")
        # size and crc32 after converting to raw image
        set_env("DISK_MANAGER_RECIPE_QCOW2_UBUNTU1604_IMAGE_SIZE", "15246295040")
        set_env("DISK_MANAGER_RECIPE_QCOW2_UBUNTU1604_IMAGE_CRC32", "1189208160")


def stop(argv):
    ImageFileServerLauncher.stop()


if __name__ == "__main__":
    declare_recipe(start, stop)
