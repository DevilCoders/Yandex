from dataclasses import dataclass
from dateutil import parser as dateparser
import json
import subprocess
import sys
import tempfile
import uuid

import jinja2

from library.python import resource


class YcpCmdEngine:

    def exec(self, command, input, err):
        return subprocess.check_output(
            command,
            input=bytes(input if input else "{}", "utf8"),
            stderr=err
        )

    def generate_id(self):
        return uuid.uuid1()


class YcpTestEngine:

    def __init__(self):
        self._id = 0

    def exec(self, command, input, err):
        sys.stdout.write('Command=%s\n' % command)
        sys.stdout.write('Input=%s\n' % input)

        if len(command) > 3 and command[3] == 'create-for-service-account':
            return resource.find('fake-iam-token.json').decode('utf8')

        if len(command) > 3 and command[3] == 'get':
            if command[2] == 'instance':
                return resource.find('fake-instance.json').decode('utf8')
            raise Exception('get: unexpected entity: %s' % command[2])

        if len(command) > 3 and command[3] == 'create':
            if command[2] == 'instance':
                return resource.find('fake-instance.json').decode('utf8')
            elif command[2] == 'disk':
                return resource.find('fake-disk.json').decode('utf8')
            elif command[2] == 'disk-placement-group':
                return resource.find('fake-disk-placement-group.json').decode('utf8')
            elif command[2] == 'placement-group':
                return resource.find('fake-placement-group.json').decode('utf8')
            elif command[2] == 'filesystem':
                return resource.find('fake-filesystem.json').decode('utf8')

            raise Exception('create: unexpected entity: %s' % command[2])

        if len(command) > 3 and command[3] == 'list':
            if command[2] == 'subnet':
                return resource.find('fake-subnet-list.json').decode('utf8')
            elif command[2] == 'image':
                return resource.find('fake-image-list.json').decode('utf8')
            elif command[2] == 'instance':
                return resource.find('fake-instance-list.json').decode('utf8')
            elif command[2] == 'disk':
                return resource.find('fake-disk-list.json').decode('utf8')
            elif command[2] == 'disk-placement-group':
                return resource.find('fake-disk-placement-group-list.json').decode('utf8')
            elif command[2] == 'placement-group':
                return resource.find('fake-placement-group-list.json').decode('utf8')
            elif command[2] == 'filesystem':
                return resource.find('fake-filesystem-list.json').decode('utf8')

            raise Exception('list: unexpected entity: %s' % command[2])

        if len(command) > 3 and command[3] == 'start':
            if command[2] == 'instance':
                return resource.find('fake-instance.json').decode('utf8')

    def generate_id(self):
        self._id += 1
        return str(self._id)


def make_ycp_engine(dry_run):
    return YcpTestEngine() if dry_run else YcpCmdEngine()


class Ycp:

    class Error(Exception):
        pass

    @dataclass
    class CreateInstanceConfig:
        name: str
        cores: int
        memory: int
        folder_id: str
        image_name: str
        zone_id: str
        subnet_name: str
        subnet_id: str
        compute_node: str
        placement_group_name: str
        host_group: str
        filesystem_id: str
        platform_id: str

    @dataclass
    class CreateDiskConfig:
        block_size: int
        name: str
        size: int
        folder_id: str
        type_id: str
        placement_group_name: str
        zone_id: str
        image_name: str

    @dataclass
    class CreateFsConfig:
        block_size: int
        name: str
        size: int
        folder_id: str
        type_id: str
        zone_id: str

    class Instance:

        def __init__(self, info):
            self.id = info['id']
            network_interface = info['network_interfaces'][0]
            address_key = "primary_v6_address"
            if address_key not in network_interface:
                address_key = "primary_v4_address"
            self.ip = network_interface[address_key]['address']
            self.compute_node = info.get('compute_node')
            self.name = info['name']
            self.created_at = dateparser.parse(info['created_at'])
            self.folder_id = info.get('folder_id', '')
            self.boot_disk = info.get('boot_disk', {}).get("disk_id", '')
            self.secondary_disks = [item.get('disk_id', '') for item in info.get('secondary_disks', [])]

    class Disk:

        def __init__(self, info):
            self.id = info['id']
            self.name = info.get('name', '')
            self.created_at = dateparser.parse(info['created_at'])
            self.instance_ids = info.get('instance_ids', [])
            self.folder_id = info.get('folder_id', '')
            self.size = info.get('size', 0)
            self.block_size = info.get('block_size', 0)

    class Image:

        def __init__(self, info):
            self.id = info['id']
            self.name = info.get('name', '')
            self.created_at = dateparser.parse(info['created_at'])

    class Snapshot:

        def __init__(self, info):
            self.id = info['id']
            self.name = info.get('name', '')
            self.created_at = dateparser.parse(info['created_at'])

    class Filesystem:

        def __init__(self, info):
            self.id = info['id']
            self.name = info['name']
            self.created_at = dateparser.parse(info['created_at'])
            self.instance_ids = info.get('instance_ids', [])

    class IamToken:

        def __init__(self, info):
            self.iam_token = info['iam_token']
            self.service_account_id = info['subject']['service_account']['id']

    def __init__(self, profile, logger, engine):
        self._profile = profile
        self._logger = logger
        self._engine = engine

    def _resolve_entity_name(self, subcommand, folder_id, name, stderr, fail_if_not_found=True):
        request = ['ycp'] + subcommand + ['list', '--format', 'json', '-r', '-', '--folder-id', folder_id, '--profile',
                                          self._profile]

        self._logger.warn("sent ycp request: %s" % " ".join([str(x) for x in request]))

        response = self._engine.exec(request, str(), stderr)

        self._logger.warn("got ycp response: %s" % response)

        j = json.loads(response)
        for x in j:
            if x.get('name') == name:
                return x['id']

        if fail_if_not_found:
            raise self.Error('entity not found, subcommand=%s, folder_id=%s, name=%s'
                             % (subcommand, folder_id, name))

        return None

    def _resolve_image_name(self, folder_id, image_name, stderr):
        return self._resolve_entity_name(
            ['compute', 'image'],
            folder_id,
            image_name,
            stderr)

    def _resolve_subnet_name(self, folder_id, subnet_name, stderr):
        return self._resolve_entity_name(
            ['vpc', 'subnet'],
            folder_id,
            subnet_name,
            stderr)

    def _resolve_placement_group_name(self, folder_id, placement_name, stderr):
        return self._resolve_entity_name(
            ['compute', 'placement-group'],
            folder_id,
            placement_name,
            stderr,
            fail_if_not_found=False)

    def _resolve_disk_placement_group_name(self, folder_id, placement_name, stderr):
        return self._resolve_entity_name(
            ['compute', 'disk-placement-group'],
            folder_id,
            placement_name,
            stderr,
            fail_if_not_found=False)

    def _create_entity(self, entity_name: str, name: str, folder_id: str, zone_id: str, stderr) -> str:
        create = resource.find(f'create-{entity_name}.yaml').decode('utf8')
        rendered_create = jinja2.Template(create).render(name=name, folder_id=folder_id, zone_id=zone_id)

        try:
            entity = json.loads(self._engine.exec(
                ['ycp', 'compute', entity_name, 'create', '-r', '-', '--format', 'json',
                 '--profile', self._profile],
                rendered_create,
                stderr))

            return entity['id']
        except subprocess.CalledProcessError as e:
            stderr.seek(0)
            self._logger.error(
                f'Ycp {entity_name} create failed with exit code {e.returncode}:\n'
                f'{b"".join(stderr.readlines()).decode("utf-8")}')
            raise self.Error(f'ycp {entity_name} create failed: {e}')

    def create_instance(self, config: CreateInstanceConfig) -> Instance:
        create_instance_yaml_template = resource.find('create-instance.yaml').decode('utf8')
        with tempfile.TemporaryFile() as stderr:
            try:
                subnet_id = config.subnet_id
                if subnet_id is None:
                    subnet_id = self._resolve_subnet_name(
                        config.folder_id,
                        config.subnet_name, stderr)

                placement_group_id = None
                if config.placement_group_name is not None:
                    placement_group_id = self._resolve_placement_group_name(
                        config.folder_id,
                        config.placement_group_name,
                        stderr)

                    if placement_group_id is None:
                        placement_group_id = self._create_entity(
                            'placement-group',
                            config.placement_group_name,
                            config.folder_id,
                            None,
                            stderr)

                platform_id = 'standard-v2'
                if config.platform_id is not None:
                    platform_id = config.platform_id

                rendered_create_instance_yaml = jinja2.Template(create_instance_yaml_template).render(
                    name=config.name,
                    cores=config.cores,
                    memory=config.memory,
                    image_id=self._resolve_image_name(config.folder_id, config.image_name, stderr),
                    zone_id=config.zone_id,
                    subnet_id=subnet_id,
                    folder_id=config.folder_id,
                    compute_node=config.compute_node,
                    placement_group_id=placement_group_id,
                    host_group=config.host_group,
                    filesystem_id=config.filesystem_id,
                    platform_id=platform_id,
                )

                response = self._engine.exec(
                    ['ycp', 'compute', 'instance', 'create', '-r', '-', '--format', 'json', '--profile', self._profile],
                    rendered_create_instance_yaml,
                    stderr)
            except subprocess.CalledProcessError as e:
                stderr.seek(0)
                self._logger.error(f'Ycp instance create failed with exit code {e.returncode}:\n'
                                   f'{b"".join(stderr.readlines()).decode("utf-8")}')
                raise self.Error(f'ycp instance create failed: {e}')

        return Ycp.Instance(json.loads(response))

    def create_disk(self, config: CreateDiskConfig) -> Disk:
        placement_group_id = None
        if config.placement_group_name is not None:
            with tempfile.TemporaryFile() as stderr:
                placement_group_id = self._resolve_disk_placement_group_name(
                    config.folder_id,
                    config.placement_group_name,
                    stderr
                )

                if placement_group_id is None:
                    placement_group_id = self._create_entity(
                        'disk-placement-group',
                        config.placement_group_name,
                        config.folder_id,
                        config.zone_id,
                        stderr)

        create_disk_yaml_template = resource.find('create-disk.yaml').decode('utf8')
        with tempfile.TemporaryFile() as stderr:
            image_id = None
            if config.image_name is not None:
                image_id=self._resolve_image_name(
                    config.folder_id,
                    config.image_name,
                    stderr)
            rendered_create_disk_yaml = jinja2.Template(create_disk_yaml_template).render(
                block_size=config.block_size,
                name=config.name,
                size=config.size,
                type_id=config.type_id,
                placement_group_id=placement_group_id,
                image_id=image_id,
                zone_id=config.zone_id,
                folder_id=config.folder_id)
            try:
                response = self._engine.exec(
                    ['ycp', 'compute', 'disk', 'create', '-r', '-', '--format', 'json', '--profile', self._profile],
                    rendered_create_disk_yaml,
                    stderr)
            except subprocess.CalledProcessError as e:
                stderr.seek(0)
                self._logger.error(f'Ycp disk create failed with exit code {e.returncode}:\n'
                                   f'{b"".join(stderr.readlines()).decode("utf-8")}')
                raise self.Error(f'ycp disk create failed: {e}')

        return Ycp.Disk(json.loads(response))

    def attach_disk(self, instance: Instance, disk: Disk) -> None:
        attach_disk_yaml_template = resource.find('attach-disk.yaml').decode('utf8')
        rendered_attach_disk_yaml = jinja2.Template(attach_disk_yaml_template).render(
            instance_id=instance.id,
            disk_id=disk.id)
        with tempfile.TemporaryFile() as stderr:
            try:
                self._engine.exec(
                    ['ycp', 'compute', 'instance', 'attach-disk', '-r', '-', '--format', 'json',
                     '--profile', self._profile],
                    rendered_attach_disk_yaml,
                    stderr)
            except subprocess.CalledProcessError as e:
                stderr.seek(0)
                self._logger.error(f'Ycp disk attach failed with exit code {e.returncode}:\n'
                                   f'{b"".join(stderr.readlines()).decode("utf-8")}')
                raise self.Error(f'ycp attach-disk failed: {e}')

    def create_fs(self, config: CreateFsConfig) -> Filesystem:
        create_fs_yaml_template = resource.find('create-fs.yaml').decode('utf8')
        rendered_create_fs_yaml = jinja2.Template(create_fs_yaml_template).render(
            block_size=config.block_size,
            name=config.name,
            size=config.size,
            type_id=config.type_id,
            zone_id=config.zone_id,
            folder_id=config.folder_id)
        with tempfile.TemporaryFile() as stderr:
            try:
                response = self._engine.exec(
                    ['ycp', 'compute', 'filesystem', 'create', '-r', '-', '--format', 'json', '--profile',
                     self._profile],
                    rendered_create_fs_yaml,
                    stderr)
            except subprocess.CalledProcessError as e:
                stderr.seek(0)
                self._logger.error(f'Ycp filesystem create failed with exit code {e.returncode}:\n'
                                   f'{b"".join(stderr.readlines()).decode("utf-8")}')
                raise self.Error(f'ycp filesystem create failed: {e}')

        return Ycp.Filesystem(json.loads(response))

    def attach_fs(self, instance: Instance, fs: Filesystem, device_name: str) -> None:
        attach_fs_yaml_template = resource.find('attach-fs.yaml').decode('utf8')
        rendered_attach_fs_yaml = jinja2.Template(attach_fs_yaml_template).render(
            instance_id=instance.id,
            filesystem_id=fs.id,
            device_name=device_name)
        with tempfile.TemporaryFile() as stderr:
            try:
                self._engine.exec(
                    ['ycp', 'compute', 'instance', 'attach-filesystem', '-r', '-', '--format', 'json',
                     '--profile', self._profile],
                    rendered_attach_fs_yaml,
                    stderr)
            except subprocess.CalledProcessError as e:
                stderr.seek(0)
                self._logger.error(f'Ycp filesystem attach failed with exit code {e.returncode}:\n'
                                   f'{b"".join(stderr.readlines()).decode("utf-8")}')
                raise self.Error(f'ycp attach-filesystem failed: {e}')

    def delete_instance(self, instance: Instance) -> None:
        delete_instance_yaml_template = resource.find('delete-instance.yaml').decode('utf8')
        rendered_delete_instance_yaml = jinja2.Template(delete_instance_yaml_template).render(instance_id=instance.id)
        with tempfile.TemporaryFile() as stderr:
            try:
                self._engine.exec(
                    ['ycp', 'compute', 'instance', 'delete', '-r', '-', '--format', 'json', '--profile', self._profile],
                    rendered_delete_instance_yaml,
                    stderr)
            except subprocess.CalledProcessError as e:
                stderr.seek(0)
                self._logger.error(f'Ycp instance delete failed with exit code {e.returncode}:\n'
                                   f'{b"".join(stderr.readlines()).decode("utf-8")}')
                raise self.Error(f'ycp instance delete failed: {e}')

    def delete_disk(self, disk: Disk) -> None:
        delete_disk_yaml_template = resource.find('delete-disk.yaml').decode('utf8')
        rendered_delete_disk_yaml = jinja2.Template(delete_disk_yaml_template).render(disk_id=disk.id)
        with tempfile.TemporaryFile() as stderr:
            try:
                self._engine.exec(
                    ['ycp', 'compute', 'disk', 'delete', '-r', '-', '--format', 'json', '--profile', self._profile],
                    rendered_delete_disk_yaml,
                    stderr)
            except subprocess.CalledProcessError as e:
                stderr.seek(0)
                self._logger.error(f'Ycp disk delete failed with exit code {e.returncode}:\n'
                                   f'{b"".join(stderr.readlines()).decode("utf-8")}')
                raise self.Error(f'ycp disk delete failed: {e}')

    def delete_image(self, image: Image) -> None:
        delete_image_yaml_template = resource.find('delete-image.yaml').decode('utf8')
        rendered_delete_image_yaml = jinja2.Template(delete_image_yaml_template).render(image_id=image.id)
        with tempfile.TemporaryFile() as stderr:
            try:
                self._engine.exec(
                    ['ycp', 'compute', 'image', 'delete', '-r', '-', '--format', 'json', '--profile', self._profile],
                    rendered_delete_image_yaml,
                    stderr)
            except subprocess.CalledProcessError as e:
                stderr.seek(0)
                self._logger.error(f'Ycp image delete failed with exit code {e.returncode}:\n'
                                   f'{b"".join(stderr.readlines()).decode("utf-8")}')
                raise self.Error(f'ycp image delete failed: {e}')

    def delete_snapshot(self, snapshot: Snapshot) -> None:
        delete_snapshot_yaml_template = resource.find('delete-snapshot.yaml').decode('utf8')
        rendered_delete_snapshot_yaml = jinja2.Template(delete_snapshot_yaml_template).render(snapshot_id=snapshot.id)
        with tempfile.TemporaryFile() as stderr:
            try:
                self._engine.exec(
                    ['ycp', 'compute', 'snapshot', 'delete', '-r', '-', '--format', 'json', '--profile', self._profile],
                    rendered_delete_snapshot_yaml,
                    stderr)
            except subprocess.CalledProcessError as e:
                stderr.seek(0)
                self._logger.error(f'Ycp snapshot delete failed with exit code {e.returncode}:\n'
                                   f'{b"".join(stderr.readlines()).decode("utf-8")}')
                raise self.Error(f'ycp snapshot delete failed: {e}')

    def detach_disk(self, instance: Instance, disk: Disk) -> None:
        detach_disk_yaml_template = resource.find('detach-disk.yaml').decode('utf8')
        rendered_detach_disk_yaml = jinja2.Template(detach_disk_yaml_template).render(
            instance_id=instance.id,
            disk_id=disk.id)
        with tempfile.TemporaryFile() as stderr:
            try:
                self._engine.exec(
                    ['ycp', 'compute', 'instance', 'detach-disk', '-r', '-', '--format', 'json',
                     '--profile', self._profile],
                    rendered_detach_disk_yaml,
                    stderr)
            except subprocess.CalledProcessError as e:
                stderr.seek(0)
                self._logger.error(f'Ycp disk detach failed with exit code {e.returncode}:\n'
                                   f'{b"".join(stderr.readlines()).decode("utf-8")}')
                raise self.Error(f'ycp detach-disk failed: {e}')

    def delete_fs(self, fs: Filesystem) -> None:
        delete_fs_yaml_template = resource.find('delete-fs.yaml').decode('utf8')
        rendered_delete_fs_yaml = jinja2.Template(delete_fs_yaml_template).render(filesystem_id=fs.id)
        with tempfile.TemporaryFile() as stderr:
            try:
                self._engine.exec(
                    ['ycp', 'compute', 'filesystem', 'delete', '-r', '-', '--format', 'json', '--profile',
                     self._profile],
                    rendered_delete_fs_yaml,
                    stderr)
            except subprocess.CalledProcessError as e:
                stderr.seek(0)
                self._logger.error(f'Ycp filesystem delete failed with exit code {e.returncode}:\n'
                                   f'{b"".join(stderr.readlines()).decode("utf-8")}')
                raise self.Error(f'ycp filesystem delete failed: {e}')

    def detach_fs(self, instance: Instance, fs: Filesystem) -> None:
        detach_fs_yaml_template = resource.find('detach-fs.yaml').decode('utf8')
        rendered_detach_fs_yaml = jinja2.Template(detach_fs_yaml_template).render(
            instance_id=instance.id,
            filesystem_id=fs.id)
        with tempfile.TemporaryFile() as stderr:
            try:
                self._engine.exec(
                    ['ycp', 'compute', 'instance', 'detach-filesystem', '-r', '-', '--format', 'json',
                     '--profile', self._profile],
                    rendered_detach_fs_yaml,
                    stderr)
            except subprocess.CalledProcessError as e:
                stderr.seek(0)
                self._logger.error(f'Ycp filesystem detach failed with exit code {e.returncode}:\n'
                                   f'{b"".join(stderr.readlines()).decode("utf-8")}')
                raise self.Error(f'ycp detach-filesystem failed: {e}')

    def create_iam_token(self, service_account_id: str) -> IamToken:
        create_iam_token_yaml_template = resource.find('create-iam-token.yaml').decode('utf8')
        rendered_create_iam_token_yaml = jinja2.Template(create_iam_token_yaml_template).render(
            account_id=service_account_id)
        with tempfile.TemporaryFile() as stderr:
            try:
                response = self._engine.exec(
                    ['ycp', 'iam', 'iam-token', 'create-for-service-account', '-r', '-', '--format', 'json',
                     '--profile', self._profile],
                    rendered_create_iam_token_yaml,
                    stderr)
            except subprocess.CalledProcessError as e:
                stderr.seek(0)
                self._logger.error(f'Ycp iam-token create failed with exit code {e.returncode}:\n'
                                   f'{b"".join(stderr.readlines()).decode("utf-8")}')
                raise self.Error(f'ycp iam-token create failed: {e}')
        return Ycp.IamToken(json.loads(response))

    def list_instances(self) -> [Instance]:
        with tempfile.TemporaryFile() as stderr:
            try:
                response = self._engine.exec(
                    ['ycp', 'compute', 'instance', 'list', '--format', 'json',
                     '--profile', self._profile],
                    str(),
                    stderr)
            except subprocess.CalledProcessError as e:
                stderr.seek(0)
                self._logger.error(f'Ycp instance list failed with exit code {e.returncode}:\n'
                                   f'{b"".join(stderr.readlines()).decode("utf-8")}')
                raise self.Error(f'ycp instance list failed: {e}')

        res = []
        for instance in json.loads(response):
            try:
                res.append(Ycp.Instance(instance))
            except KeyError:
                continue
        return res

    def list_disks(self) -> [Disk]:
        with tempfile.TemporaryFile() as stderr:
            try:
                response = self._engine.exec(
                    ['ycp', 'compute', 'disk', 'list', '--format', 'json',
                     '--profile', self._profile],
                    str(),
                    stderr)
            except subprocess.CalledProcessError as e:
                stderr.seek(0)
                self._logger.error(f'Ycp disk list failed with exit code {e.returncode}:\n'
                                   f'{b"".join(stderr.readlines()).decode("utf-8")}')
                raise self.Error(f'ycp disk list failed: {e}')

        res = []
        for disk in json.loads(response):
            try:
                res.append(Ycp.Disk(disk))
            except KeyError:
                continue
        return res

    def list_images(self) -> [Image]:
        with tempfile.TemporaryFile() as stderr:
            try:
                response = self._engine.exec(
                    ['ycp', 'compute', 'image', 'list', '--format', 'json',
                     '--profile', self._profile],
                    str(),
                    stderr)
            except subprocess.CalledProcessError as e:
                stderr.seek(0)
                self._logger.error(f'Ycp image list failed with exit code {e.returncode}:\n'
                                   f'{b"".join(stderr.readlines()).decode("utf-8")}')
                raise self.Error(f'ycp image list failed: {e}')

        res = []
        for image in json.loads(response):
            try:
                res.append(Ycp.Image(image))
            except KeyError:
                continue
        return res

    def list_snapshots(self) -> [Snapshot]:
        with tempfile.TemporaryFile() as stderr:
            try:
                response = self._engine.exec(
                    ['ycp', 'compute', 'snapshot', 'list', '--format', 'json',
                     '--profile', self._profile],
                    str(),
                    stderr)
            except subprocess.CalledProcessError as e:
                stderr.seek(0)
                self._logger.error(f'Ycp snapshot list failed with exit code {e.returncode}:\n'
                                   f'{b"".join(stderr.readlines()).decode("utf-8")}')
                raise self.Error(f'ycp snapshot list failed: {e}')

        res = []
        for snapshot in json.loads(response):
            try:
                res.append(Ycp.Snapshot(snapshot))
            except KeyError:
                continue
        return res

    def stop_instance(self, instance: Instance):
        with tempfile.TemporaryFile() as stderr:
            try:
                self._engine.exec(
                    ['ycp', 'compute', 'instance', 'stop', instance.id, '--format', 'json',
                     '--profile', self._profile],
                    str(),
                    stderr)
            except subprocess.CalledProcessError as e:
                stderr.seek(0)
                self._logger.error(f'Ycp instance stop failed with exit code {e.returncode}:\n'
                                   f'{b"".join(stderr.readlines()).decode("utf-8")}')
                raise self.Error(f'ycp instance stop failed: {e}')

    def start_instance(self, instance: Instance) -> str:
        with tempfile.TemporaryFile() as stderr:
            try:
                response = self._engine.exec(
                    ['ycp', 'compute', 'instance', 'start', instance.id, '--format', 'json',
                     '--profile', self._profile],
                    str(),
                    stderr)
            except subprocess.CalledProcessError as e:
                stderr.seek(0)
                self._logger.error(f'Ycp instance start failed with exit code {e.returncode}:\n'
                                   f'{b"".join(stderr.readlines()).decode("utf-8")}')
                raise self.Error(f'ycp instance start failed: {e}')

        instance = Ycp.Instance(json.loads(response))
        return instance.compute_node

    def list_filesystems(self) -> [Disk]:
        with tempfile.TemporaryFile() as stderr:
            try:
                response = self._engine.exec(
                    ['ycp', 'compute', 'filesystem', 'list', '--format', 'json',
                     '--profile', self._profile],
                    str(),
                    stderr)
            except subprocess.CalledProcessError as e:
                stderr.seek(0)
                self._logger.error(f'Ycp filesystem list failed with exit code {e.returncode}:\n'
                                   f'{b"".join(stderr.readlines()).decode("utf-8")}')
                raise self.Error(f'ycp filesystem list failed: {e}')

        res = []
        for fs in json.loads(response):
            try:
                res.append(Ycp.Filesystem(fs))
            except KeyError:
                continue
        return res

    def get_instance(self, instance_id) -> Instance:
        with tempfile.TemporaryFile() as stderr:
            try:
                response = self._engine.exec(
                    ['ycp', 'compute', 'instance', 'get', instance_id, '--format', 'json',
                     '--profile', self._profile],
                    str(),
                    stderr)
            except subprocess.CalledProcessError as e:
                stderr.seek(0)
                self._logger.error(f'Ycp instance get failed with exit code {e.returncode}:\n'
                                   f'{b"".join(stderr.readlines()).decode("utf-8")}')
                raise self.Error(f'ycp instance get failed: {e}')

        return Ycp.Instance(json.loads(response))

    def get_disk(self, disk_id) -> Disk:
        with tempfile.TemporaryFile() as stderr:
            try:
                response = self._engine.exec(
                    ['ycp', 'compute', 'disk', 'get', disk_id, '--format', 'json',
                     '--profile', self._profile],
                    str(),
                    stderr)
            except subprocess.CalledProcessError as e:
                stderr.seek(0)
                self._logger.error(f'Ycp disk get failed with exit code {e.returncode}:\n'
                                   f'{b"".join(stderr.readlines()).decode("utf-8")}')
                raise self.Error(f'ycp disk get failed: {e}')

        return Ycp.Disk(json.loads(response))

    def get_cloud_id(self, folder_id) -> str:
        with tempfile.TemporaryFile() as stderr:
            try:
                response = self._engine.exec(
                    ['ycp', 'resource-manager', 'folder', 'get', folder_id, '--format', 'json',
                     '--profile', self._profile],
                    str(),
                    stderr)
            except subprocess.CalledProcessError as e:
                stderr.seek(0)
                self._logger.error(f'Ycp cloud id get failed with exit code {e.returncode}:\n'
                                   f'{b"".join(stderr.readlines()).decode("utf-8")}')
                raise self.Error(f'ycp cloud id get failed: {e}')

        return json.loads(response).get("cloud_id")
