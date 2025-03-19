import os

import ydb.tests.library.common.yatest_common as yatest_common
from ydb.tests.library.harness.param_constants import kikimr_driver_path
from library.python.testing.recipe import declare_recipe, set_env

from cloud.disk_manager.test.recipe.access_service_launcher import AccessServiceLauncher
from cloud.disk_manager.test.recipe.disk_manager_launcher import DiskManagerLauncher
from cloud.disk_manager.test.recipe.image_file_server_launcher import ImageFileServerLauncher
from cloud.disk_manager.test.recipe.kikimr_launcher import KikimrLauncher
from cloud.disk_manager.test.recipe.metadata_service_launcher import MetadataServiceLauncher
from cloud.disk_manager.test.recipe.nbs_launcher import NbsLauncher
from cloud.disk_manager.test.recipe.nfs_launcher import NfsLauncher
from cloud.disk_manager.test.recipe.snapshot_launcher import SnapshotLauncher


def get_controlplane_disk_manager_count(argv):
    return 2 if 'multiple-disk-managers' in argv else 1


def get_dataplane_disk_manager_count():
    return 1


def get_disk_manager_count(argv):
    return get_controlplane_disk_manager_count(argv) + get_dataplane_disk_manager_count()


def start(argv):
    certs_dir = yatest_common.source_path("cloud/blockstore/tests/certs")
    root_certs_file = os.path.join(certs_dir, "server.crt")
    cert_file = os.path.join(certs_dir, "server.crt")
    cert_key_file = os.path.join(certs_dir, "server.key")
    set_env("DISK_MANAGER_RECIPE_ROOT_CERTS_FILE", root_certs_file)

    images_dir = yatest_common.source_path("cloud/disk_manager/test/images")
    image_file = os.path.join(images_dir, "image.img")
    set_env("DISK_MANAGER_RECIPE_IMAGE_FILE", image_file)

    kikimr_binary_path = kikimr_driver_path()
    nbs_binary_path = yatest_common.binary_path("cloud/blockstore/daemon/blockstore-server")
    nfs_binary_path = yatest_common.binary_path("cloud/filestore/server/filestore-server")

    if 'stable' in argv:
        kikimr_binary_path = yatest_common.build_path(
            "kikimr/public/tools/package/stable/Berkanavt/kikimr/bin/kikimr")
        nbs_binary_path = yatest_common.build_path(
            "cloud/blockstore/tests/recipes/local-kikimr/stable-package-nbs/usr/bin/blockstore-server")

    with_nemesis = 'nemesis' in argv

    kikimr = KikimrLauncher(kikimr_binary_path=kikimr_binary_path)
    kikimr.start()
    set_env("DISK_MANAGER_RECIPE_KIKIMR_PORT", str(kikimr.port))

    if 'kikimr' in argv:
        return

    nbs = NbsLauncher(
        kikimr.port,
        kikimr.domains_txt,
        kikimr.dynamic_storage_pools,
        root_certs_file,
        cert_file,
        cert_key_file,
        kikimr_binary_path=kikimr_binary_path,
        nbs_binary_path=nbs_binary_path)
    nbs.start()
    set_env("DISK_MANAGER_RECIPE_NBS_PORT", str(nbs.port))

    if 'nbs' in argv:
        return

    nfs = NfsLauncher(
        kikimr_port=kikimr.port,
        domains_txt=kikimr.domains_txt,
        names_txt=kikimr.names_txt,
        nfs_binary_path=nfs_binary_path)
    nfs.start()
    set_env("DISK_MANAGER_RECIPE_NFS_PORT", str(nfs.port))

    if 'nfs' in argv:
        return

    image_file_path = yatest_common.source_path("cloud/disk_manager/test/images/image.img")
    image_file_server = ImageFileServerLauncher(image_file_path)
    image_file_server.start()
    set_env("DISK_MANAGER_RECIPE_IMAGE_FILE_SERVER_PORT", str(image_file_server.port))
    set_env("DISK_MANAGER_RECIPE_IMAGE_FILE_SIZE", str(image_file_server.image_file_size))
    set_env("DISK_MANAGER_RECIPE_IMAGE_FILE_CRC32", str(image_file_server.image_file_crc32))

    qcow2_image_file_path = yatest_common.work_path("qcow2_images/ubuntu-18.04-minimal-cloudimg-amd64.img")
    if os.path.exists(qcow2_image_file_path):
        qcow2_image_file_server = ImageFileServerLauncher(qcow2_image_file_path)
        qcow2_image_file_server.start()
        set_env("DISK_MANAGER_RECIPE_QCOW2_IMAGE_FILE_SERVER_PORT", str(qcow2_image_file_server.port))
        set_env("DISK_MANAGER_RECIPE_QCOW2_IMAGE_FILE_SIZE", "332595200")
        # size and crc32 after converting to raw image
        set_env("DISK_MANAGER_RECIPE_QCOW2_IMAGE_SIZE", "2361393152")
        set_env("DISK_MANAGER_RECIPE_QCOW2_IMAGE_CRC32", "2577917554")

    invalid_image_file_path = yatest_common.source_path("cloud/disk_manager/test/images/invalid_image.img")
    invalid_image_file_server = ImageFileServerLauncher(invalid_image_file_path)
    invalid_image_file_server.start()
    set_env("DISK_MANAGER_RECIPE_INVALID_IMAGE_FILE_SERVER_PORT", str(invalid_image_file_server.port))

    snapshot = SnapshotLauncher(
        kikimr.port,
        nbs.port,
        root_certs_file,
        cert_file,
        cert_key_file)
    snapshot.start()
    set_env("DISK_MANAGER_RECIPE_SNAPSHOT_PORT", str(snapshot.port))

    if 'snapshot' in argv:
        return

    metadata_service = MetadataServiceLauncher()
    metadata_service.start()

    access_service = AccessServiceLauncher(cert_file, cert_key_file)
    access_service.start()
    set_env("DISK_MANAGER_RECIPE_ACCESS_SERVICE_PORT", str(access_service.port))

    disk_managers = []

    for i in range(0, get_controlplane_disk_manager_count(argv)):
        idx = len(disk_managers)

        disk_managers.append(DiskManagerLauncher(
            hostname="localhost{}".format(idx),
            kikimr_port=kikimr.port,
            nbs_port=nbs.port,
            metadata_url=metadata_service.url,
            root_certs_file=root_certs_file,
            idx=idx,
            is_dataplane=False,
            with_nemesis=with_nemesis,
            nfs_port=nfs.port,
            snapshot_port=snapshot.port,
            access_service_port=access_service.port,
            cert_file=cert_file,
            cert_key_file=cert_key_file,
        ))
        disk_managers[idx].start()

    for i in range(0, get_dataplane_disk_manager_count()):
        idx = len(disk_managers)

        disk_managers.append(DiskManagerLauncher(
            hostname="localhost{}".format(idx),
            kikimr_port=kikimr.port,
            nbs_port=nbs.port,
            metadata_url=metadata_service.url,
            root_certs_file=root_certs_file,
            idx=idx,
            is_dataplane=True,
            with_nemesis=with_nemesis
        ))
        disk_managers[idx].start()

    # First node is always control plane.
    set_env("DISK_MANAGER_RECIPE_DISK_MANAGER_PORT", str(disk_managers[0].port))
    set_env("DISK_MANAGER_RECIPE_SERVER_CONFIG", disk_managers[0].server_config)


def stop(argv):
    for i in range(0, get_disk_manager_count(argv)):
        DiskManagerLauncher.stop(i)
    AccessServiceLauncher.stop()
    MetadataServiceLauncher.stop()
    SnapshotLauncher.stop()
    ImageFileServerLauncher.stop()
    NfsLauncher.stop()
    NbsLauncher.stop()
    KikimrLauncher.stop()


if __name__ == "__main__":
    declare_recipe(start, stop)
