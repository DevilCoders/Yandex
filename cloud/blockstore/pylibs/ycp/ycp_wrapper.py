from contextlib import contextmanager
from datetime import datetime, timezone
from typing import Optional

from cloud.blockstore.pylibs.clusters import FolderDesc

from .ycp import Ycp


class YcpWrapper:
    TMP_INSTANCE_PREFIX = "tmpinstance-"

    class Error(Exception):
        pass

    def __init__(self, profile: str, folder_desc: FolderDesc, logger, engine):
        self._folder_desc = folder_desc
        self._logger = logger
        self._ycp = Ycp(profile, logger, engine)
        self._ycp_engine = engine
        self._auto_delete = True

    def turn_off_auto_deletion(self):
        self._auto_delete = False

    @contextmanager
    def create_instance(self,
                        cores: int,
                        memory: int,
                        compute_node: str = None,
                        placement_group_name: str = None,
                        host_group: str = None,
                        image_name: str = None,
                        name: str = None,
                        platform_id: str = None,
                        auto_delete: bool = True) -> Ycp.Instance:
        self._logger.info('Creating instance')
        create_instance_cfg = Ycp.CreateInstanceConfig(
            name=name or '{}{}'.format(self.TMP_INSTANCE_PREFIX, self._ycp_engine.generate_id()),
            cores=cores,
            memory=memory * 1024 ** 3,
            image_name=image_name or self._folder_desc.image_name,
            zone_id=self._folder_desc.zone_id,
            subnet_name=self._folder_desc.subnet_name,
            subnet_id=self._folder_desc.subnet_id,
            folder_id=self._folder_desc.folder_id,
            compute_node=compute_node,
            placement_group_name=placement_group_name,
            host_group=host_group,
            filesystem_id=self._folder_desc.filesystem_id,
            platform_id=platform_id,
        )
        self._logger.debug(f'create_instance_config: {create_instance_cfg}')
        try:
            instance = self._ycp.create_instance(create_instance_cfg)
        except Ycp.Error as e:
            raise self.Error(f'failed to create instance: {e}')
        self._logger.info(f'Created instance <id={instance.id}, host={instance.compute_node}, ip={instance.ip}>')
        try:
            yield instance
        finally:
            if auto_delete and self._auto_delete:
                self.delete_instance(instance)

    def delete_instance(self, instance: Ycp.Instance):
        self._logger.info(f'Deleting instance <id={instance.id}>')
        try:
            self._ycp.delete_instance(instance)
        except Ycp.Error as e:
            raise self.Error(f'failed to delete instance: {e}')
        self._logger.info(f'Deleted instance <id={instance.id}>')

    @contextmanager
    def create_disk(self,
                    size: int,
                    type_id: str,
                    bs: int = 4096,
                    placement_group_name: str = None,
                    name: str = None,
                    image_name: str = None,
                    auto_delete: bool = True) -> Ycp.Disk:
        self._logger.info('Creating disk')
        create_disk_config = Ycp.CreateDiskConfig(
            block_size=bs,
            name=name or f'tmpdisk-{self._ycp_engine.generate_id()}',
            size=size * 1024 ** 3,
            type_id=type_id,
            placement_group_name=placement_group_name,
            zone_id=self._folder_desc.zone_id,
            folder_id=self._folder_desc.folder_id,
            image_name=image_name)
        self._logger.debug(f'create_disk_config: {create_disk_config}')
        try:
            disk = self._ycp.create_disk(create_disk_config)
        except Ycp.Error as e:
            raise self.Error(f'failed to create disk: {e}')
        self._logger.info(f'Created disk <id={disk.id}>')
        try:
            yield disk
        finally:
            if auto_delete and self._auto_delete:
                self.delete_disk(disk)

    def delete_disk(self, disk: Ycp.Disk):
        self._logger.info(f'Deleting disk <id={disk.id}>')
        try:
            self._ycp.delete_disk(disk)
        except Ycp.Error as e:
            raise self.Error(f'failed to delete disk <id={disk.id}>: {e}')
        self._logger.info(f'Deleted disk <id={disk.id}>')

    @contextmanager
    def attach_disk(self, instance: Ycp.Instance, disk: Ycp.Disk, auto_detach: bool = True) -> None:
        self._logger.info(f'Attaching disk <id={disk.id}> to instance <id={instance.id}>')
        try:
            self._ycp.attach_disk(instance, disk)
        except Ycp.Error as e:
            raise self.Error(
                f'failed to attach disk <id={disk.id}> to instance <id={instance.id}>: {e}')
        try:
            yield
        finally:
            if auto_detach:
                self.detach_disk(instance, disk)

    def detach_disk(self, instance: Ycp.Instance, disk: Ycp.Disk):
        self._logger.info(f'Detaching disk <id={disk.id}> from instance <id={instance.id}>')
        try:
            self._ycp.detach_disk(instance, disk)
        except Ycp.Error as e:
            raise self.Error(
                f'failed to detach disk <id={disk.id}> from instance <id={instance.id}>: {e}')
        self._logger.info(f'Detached disk <id={disk.id}> from instance <id={instance.id}>')

    @contextmanager
    def create_fs(self,
                  size: int,
                  type_id: str,
                  bs: int = 4096,
                  name: str = None,
                  auto_delete: bool = True) -> Ycp.Filesystem:
        self._logger.info('Creating filesystem')
        create_fs_config = Ycp.CreateFsConfig(
            block_size=bs,
            name=name or f'tmpfs-{self._ycp_engine.generate_id()}',
            size=size * 1024 ** 3,
            type_id=type_id,
            zone_id=self._folder_desc.zone_id,
            folder_id=self._folder_desc.folder_id)
        self._logger.debug(f'create_fs_config: {create_fs_config}')
        try:
            fs = self._ycp.create_fs(create_fs_config)
        except Ycp.Error as e:
            raise self.Error(f'failed to create filesystem: {e}')
        self._logger.info(f'Created filesystem <id={fs.id}>')
        try:
            yield fs
        finally:
            if auto_delete and self._auto_delete:
                self.delete_fs(fs)

    def delete_fs(self, fs: Ycp.Filesystem):
        self._logger.info(f'Deleting filesystem <id={fs.id}>')
        try:
            self._ycp.delete_fs(fs)
        except Ycp.Error as e:
            raise self.Error(f'failed to delete filesystem <id={fs.id}>: {e}')
        self._logger.info(f'Deleted filesystem <id={fs.id}>')

    @contextmanager
    def attach_fs(self, instance: Ycp.Instance, fs: Ycp.Filesystem, device_name: str, auto_detach: bool = True) -> None:
        self._logger.info(f'Attaching filesystem <id={fs.id}> to instance <id={instance.id}>')
        try:
            self._ycp.stop_instance(instance)
            self._ycp.attach_fs(instance, fs, device_name)
            self._ycp.start_instance(instance)
        except Ycp.Error as e:
            raise self.Error(
                f'failed to attach filesystem <id={fs.id}> to instance <id={instance.id}>: {e}')
        try:
            yield
        finally:
            if auto_detach:
                self.detach_fs(instance, fs)

    def detach_fs(self, instance: Ycp.Instance, fs: Ycp.Filesystem):
        self._logger.info(f'Detaching filesystem <id={fs.id}> from instance <id={instance.id}>')
        try:
            self._ycp.stop_instance(instance)
            self._ycp.detach_fs(instance, fs)
            self._ycp.start_instance(instance)
        except Ycp.Error as e:
            raise self.Error(
                f'failed to detach filesystem <id={fs.id}> from instance <id={instance.id}>: {e}')
        self._logger.info(f'Detached filesystem <id={fs.id}> from instance <id={instance.id}>')

    def delete_image(self, image: Ycp.Image):
        self._logger.info(f'Deleting image <id={image.id}>')
        try:
            self._ycp.delete_image(image)
        except Ycp.Error as e:
            raise self.Error(f'failed to delete image <id={image.id}>: {e}')
        self._logger.info(f'Deleted image <id={image.id}>')

    def delete_snapshot(self, snapshot: Ycp.Snapshot):
        self._logger.info(f'Deleting snapshot <id={snapshot.id}>')
        try:
            self._ycp.delete_snapshot(snapshot)
        except Ycp.Error as e:
            raise self.Error(f'failed to delete snapshot <id={snapshot.id}>: {e}')
        self._logger.info(f'Deleted snapshot <id={snapshot.id}>')

    def create_iam_token(self, service_account_id: str) -> Ycp.IamToken:
        self._logger.info(f'Creating iam token for service_account <id={service_account_id}>')
        try:
            return self._ycp.create_iam_token(service_account_id)
        except Ycp.Error as e:
            raise self.Error(
                f'failed to create iam token for service_account <id={service_account_id}>: {e}')

    def list_instances(self) -> [Ycp.Instance]:
        self._logger.info('Listing all instances')
        try:
            return self._ycp.list_instances()
        except Ycp.Error as e:
            raise self.Error(
                f'failed to list all instance: {e}')

    def list_disks(self) -> [Ycp.Disk]:
        self._logger.info('Listing all disks')
        try:
            return self._ycp.list_disks()
        except Ycp.Error as e:
            raise self.Error(
                f'failed to list all disks: {e}')

    def list_images(self) -> [Ycp.Image]:
        self._logger.info('Listing all images')
        try:
            return self._ycp.list_images()
        except Ycp.Error as e:
            raise self.Error(
                f'failed to list all images: {e}')

    def list_snapshots(self) -> [Ycp.Snapshot]:
        self._logger.info('Listing all snapshots')
        try:
            return self._ycp.list_snapshots()
        except Ycp.Error as e:
            raise self.Error(
                f'failed to list all snapshots: {e}')

    def list_filesystems(self) -> [Ycp.Filesystem]:
        self._logger.info('Listing all filesystems')
        try:
            return self._ycp.list_filesystems()
        except Ycp.Error as e:
            raise self.Error(
                f'failed to list all filesystems: {e}')

    def find_instance(self, name: str) -> Optional[Ycp.Instance]:
        instances = self.list_instances()
        for instance in instances:
            if instance.name == name:
                self._logger.info(f'Found instance with <id={instance.id}>')
                return instance
        self._logger.info('Instance not found')
        return None

    def get_instance(self, instance_id) -> Ycp.Instance:
        try:
            return self._ycp.get_instance(instance_id)
        except Ycp.Error as e:
            raise self.Error(f'failed to get instance: {e}')

    def get_disk(self, disk_id) -> Ycp.Disk:
        try:
            return self._ycp.get_disk(disk_id)
        except Ycp.Error as e:
            raise self.Error(f'failed to get disk: {e}')

    def delete_tmp_instances(self, ttl_days: int):
        now = datetime.now(timezone.utc)

        for instance in self.list_instances():
            time_delta = now - instance.created_at

            if instance.name.startswith(self.TMP_INSTANCE_PREFIX) and time_delta.days > ttl_days:
                self._logger.info(f'Delete old instance with <id={instance.id}>, created at "{instance.created_at}"')
                self.delete_instance(instance)
