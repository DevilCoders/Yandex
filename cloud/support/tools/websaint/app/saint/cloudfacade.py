
import app.saint.printers as printers
from app.saint.helpers import *

# saint parts
from app.saint.saint_cloud_components.st_auth import St_Auth
from app.saint.saint_cloud_components.st_s3 import St_S3
from app.saint.saint_cloud_components.st_billing import St_Billing
from app.saint.saint_cloud_components.st_cloud import St_Cloud
from app.saint.saint_cloud_components.st_user import St_User
from app.saint.saint_cloud_components.st_folder import St_Folder
from app.saint.saint_cloud_components.st_disk import St_Disk
from app.saint.saint_cloud_components.st_mdb import St_MDB
from app.saint.saint_cloud_components.st_operation import St_Operation
from app.saint.saint_cloud_components.st_k8s import St_K8s
from app.saint.saint_cloud_components.st_network import St_Network
from app.saint.saint_cloud_components.st_instance import St_Instance

# grpc channels
from app.saint.grpc_gw import GrpcChannels

home_dir = expanduser('~')
config = configparser.RawConfigParser()
config.read('./rei.cfg'.format(home_dir))


class CloudFacade(St_Disk, St_Network, St_MDB, St_Operation, St_K8s, St_Auth, St_Cloud, St_Folder, St_User, St_S3,
                  St_Billing, St_Instance, GrpcChannels):

    def __init__(self, profile: Profile):
        self.profile = profile
        self.endpoints = profile.endpoints

        self.FOLDER_STATUS = [
            f'STATUS_UNSPECIFIED',
            'ACTIVE',
            'DELETING',
            'DELETED',
            'PENDING_DELETION'
        ]

        self.cloud_stat = {
            0: f'STATUS_UNSPECIFIED',
            1: 'CREATING',
            2: 'ACTIVE',
            3: 'DELETING',
            4: f'BLOCKED',
            5: 'DELETED',
            6: 'PENDING DELETION'
        }
        self.DISK_MODE = {
            0: f'MODE_UNSPECIFIED',
            1: f'READ',
            2: f'READ_WRITE'
        }
        self.DISK_STATUS = {
            0: f'STATUS UNSPECIFIED',
            1: 'CREATING',
            2: f'READY',
            3: f'ERROR',
            4: 'DELETING'
        }
        self.DISK_A_STATUS = {
            0: f'STATUS_UNSPECIFIED',
            1: 'ATTACHING',
            2: 'ATTACHED',
            3: 'DETACHING',
            4: f'DETACH_ERROR'
        }
        self.OS_TYPE = {
            0: f'OS_UNSPECIFIED',
            1: 'LINUX',
            2: 'WINDOWS'
        }
        self.INSTANCE_STATUS = {
            0: f'STATUS_UNSPECIFIED',
            1: 'PROVISIONING',
            2: f'RUNNING',
            3: 'STOPPING',
            4: 'STOPPED',
            5: 'STARTING',
            6: 'RESTARTING',
            7: f'UPDATING',
            8: f'ERROR',
            9: f'CRASHED',
            10: f'DELETING'
        }
        self.IP_VERSION = {
            0: f'IP_VERSION_UNSPECIFIED',
            1: "IPV4",
            2: "IPV6"
        }
        self.IP_TYPE = {
            0: f'TYPE_UNSPECIFIED',
            1: 'INTERNAL',
            2: 'EXTERNAL'
        }
        self.SN_STATUS = {
            0: f'STATUS_UNSPECIFIED',
            1: 'CREATING',
            2: 'ACTIVE',
            3: 'DELETING'
        }


    def format_address(self, address_raw):
        lbl = {}
        address = None
        try:
            for label, value in address_raw.labels:
                lbl[label] = value

            address = {
                'address_id': address_raw.id,
                'ip_address': address_raw.address,
                'created_at': timestamp_resolve_grpc(address_raw.created_at),
                'ephemeral': 'dynamic' if address_raw.ephemeral else 'static',
                'folder_id': address_raw.folder_id,
                'ip_version': self.IP_VERSION[address_raw.ip_version],
                'labels': lbl,
                'name': address_raw.name,
                'type': self.IP_TYPE[address_raw.type],
                'used': address_raw.used,
                'zone': address_raw.zone_id,
                'ddos': address_raw.ddos_protection_provider or 'None',
                'smtp': 'Yes' if address_raw.outgoing_smtp_capability else 'No'
            }
        except AttributeError:
            logging.error(f"address {address_raw} not found")

        return address

    @staticmethod
    def format_operation(operation_raw):
        operation = None
        try:
            operation = {
                'operation_id': operation_raw.id,
                'description': operation_raw.description,
                'created_at': timestamp_resolve_grpc(operation_raw.created_at),
                'created_by': operation_raw.created_by,
                'modified_at': timestamp_resolve_grpc(operation_raw.modified_at),
                'done': 'Yes' if operation_raw.done else 'No',
                'metadata': f'{operation_raw.metadata.value}',
                'error': operation_raw.error.message if operation_raw.error.code > 0 else 'ok'
            }
        except AttributeError:
            logging.error(f'Operation {operation_raw} is empty corrupt or not found')

        return operation

    def format_subnet(self, subnet):
        subnet_formatted = None
        try:
            if subnet.dhcp_options:
                a = "\n".join(subnet.dhcp_options.domain_name_servers)
                b = subnet.dhcp_options.domain_name
                c = "\n".join(subnet.dhcp_options.ntp_servers)
                subnet_dhcp = f'ns:  {a} \n ' \
                              f'domain: {b} \n ' \
                              f'ntp: {c}'

            if not (subnet.dhcp_options.domain_name_servers and subnet.dhcp_options.domain_name
                    and subnet.dhcp_options.ntp_servers):
                subnet_dhcp = ''

            subnet_formatted = {
                'cidr_v4': ' '.join(subnet.v4_cidr_blocks or ['-', ]),
                'cidr_v6': ' '.join(subnet.v6_cidr_blocks or ['-', ]),
                'subnet_id': subnet.id,
                'name': subnet.name,
                'status': self.SN_STATUS[subnet.status],
                'egress_nat': 'Yes' if subnet.egress_nat_enable else 'No',
                'route_table': subnet.route_table_id,
                'network_id': subnet.network_id,
                'folder_id': subnet.folder_id,
                'zone': subnet.zone_id,
                'description': subnet.description,
                'created_at': timestamp_resolve_grpc(subnet.created_at),
                'dhcp': subnet_dhcp or ''
            }
        except AttributeError:
            logging.error(f'Subnet {subnet} not found or corrupt')
        return subnet_formatted

    def format_cloud(self, cloud):
        result = {}
        try:
            result = {
                'cloud_id': cloud.id,
                'cloud_name': cloud.name,
                'description': cloud.description,
                'status': self.cloud_stat[cloud.status],
                'created_at': timestamp_resolve_grpc(cloud.created_at),
                'organization_id': cloud.organization_id
            }
        except AttributeError:
            logging.error(f"Cloud {cloud} not found or corrupt")
        return result

    @staticmethod
    def format_access_bindings(acb):
        """
        :param acb: ListAccessBindingsResponse
        :return: user_roles, fa_roles: {'user_id' : [roles_list]}
        """

        def role_append(roles, subject, role):
            if subject in roles.keys() and role != 'resource-manager.clouds.owner':
                roles[subject].append(role)
            elif subject not in roles.keys():
                roles[subject] = [f"{role}"]
            elif subject in roles.keys() and role == 'resource-manager.clouds.owner':
                roles[subject].insert(0, role)
            return roles

        user_roles, fa_roles = {}, {}
        try:
            for role in acb.access_bindings:
                if role.subject.type == 'userAccount':
                    user_roles = role_append(user_roles, role.subject.id, role.role_id)
                elif role.subject.type == 'federatedUser':
                    fa_roles = role_append(fa_roles, role.subject.id, role.role_id)
        except AttributeError:
            logging.error("access bingings list empty")

        return user_roles, fa_roles

    def format_folder(self, folder):
        data = {}
        try:
            data = {
                'id': folder.id,
                'cloudId': folder.cloud_id,
                'name': folder.name,
                'status': self.FOLDER_STATUS[folder.status],
                'description': folder.description,
                'created_at': timestamp_resolve_grpc(folder.created_at),
                'link': get_nda_link(f'http://console.cloud.yandex.ru/folders/{folder.id}?section=dashboard')
            }
        except AttributeError:
            logging.error(f"Folder {folder} not found or corrupt")

        return data

    def format_folders(self, resp):
        final = []

        try:
            for resps in resp.folders:
                data = self.format_folder(resps)
                final.append(data)
        except AttributeError:
            logging.error(f"no folders found or folder list {resp} corrupt")
        return final

    def format_disk_image(self, img):
        image = None
        try:
            image = {
                'id': img.id,
                'status': self.DISK_STATUS[img.status],
                'folder_id': img.folder_id,
                'created_at': timestamp_resolve_grpc(img.created_at),
                'name': img.name,
                'description': img.description,
                'family': img.family,
                'storage_size': printers.get_bytes_size_string(int(img.storage_size)),
                'min_disk_size': printers.get_bytes_size_string(int(img.min_disk_size)),
                'product_ids': ' '.join(img.product_ids)
            }
        except AttributeError:
            logging.error('Image not found or corrupt')
        return image

    def format_images_list(self, img):
        images = []
        try:
            for i in img.images:
                next_image = self.format_disk_image(i)
                images.append(next_image)
        except AttributeError:
            print(f"Image list {img} not found or corrupt")
        return images

    def format_disk(self, disk):
        final = {}
        image = {}
        try:
            dash_link = get_nda_link(self.profile.prod_disk_dashbord.format(disk.id))

            if disk.source_image_id:
                image = self.format_disk_image(self.check_source_image(disk.source_image_id))
            elif disk.source_snapshot_id:
                image['name'] = 'from snapshot {id}'.format(id=disk.source_snapshot_id)
                image['id'] = disk.source_snapshot_id
            else:
                image = {}

            disk_size = disk.size // (1024 * 1024 * 1024)

            v = self.get_vcpu(disk.instance_ids[0]) if disk.instance_ids else 96

            iops = calc_iops(disk_size, disk.type_id, v)

            final = {
                'disk_id': disk.id,
                'type_id': disk.type_id,
                'image': image.get("name", "Not a boot disk"),
                'image_id': image.get("id", "Not a boot disk"),
                'instance_id': ' '.join(disk.instance_ids) if disk.instance_ids else None,
                'size': printers.get_bytes_size_string(disk.size),
                'folder_id': disk.folder_id,
                'created_at': timestamp_resolve_grpc(disk.created_at),
                'status': self.DISK_STATUS[disk.status],
                'disk_dashboard': dash_link,

            }

            final = dict(final, **{'IOPS R/W': f'R: {iops["RIOPS"]}/W: {iops["WIOPS"]}',
                                   'Bandwidth R/W': f'R: {iops["RBW"]} Mb/W: {iops["WBW"]} Mb'})

        except AttributeError as e:
            logging.error(e)
        return final

    def format_bucket(self, b):
        bucket_formatted = {
            'name': b.get('name'),
            'cloud_id': b.get('cloud_id'),
            'default_storage_class': b.get('default_storage_class'),
            'likely_publicly_accessible': b.get('likely_publicly_accessible'),
            'used_space': printers.get_bytes_size_string(b.get('used_space')),
            'max_size' : printers.get_bytes_size_string(b.get('max_size')),
            'std_objects_count': b.get('simple_objects_count') + b.get('multipart_objects_count'),
            'cold_objects_count': b.get('cold_simple_objects_count') + b.get('cold_multipart_objects_count'),
            'std_objects_size': printers.get_bytes_size_string(b.get('simple_objects_size') + b.get('multipart_objects_size')),
            'cold_objects_size': printers.get_bytes_size_string(b.get('cold_simple_objects_size') + b.get('cold_multipart_objects_size')),
            'created_at': b.get('created_ts'),
            'link': f'https://s3-cloud.chv.yandex-team.ru/?componentSettings=%7B%22projects%22%3A%7B%22query%22%3A%22{b.get("name")}%22%7D%7D'
        }

        return bucket_formatted

    def format_disk_list(self, disk_list):
        result = []
        for next_disk in disk_list:
            disk_attach_params = next_disk['disk']
            disk_attach_type = next_disk['type']
            disk = self.disk_get(disk_attach_params.disk_id)
            v = self.get_vcpu(disk.instance_ids[0]) if disk.instance_ids else 96
            iops = calc_iops(disk.size // (1024 * 1024 * 1024), disk.type_id, v)

            disk_dict = {
                'disk_id': disk_attach_params.disk_id,
                'size': printers.get_bytes_size_string(disk.size),
                'type': disk_attach_type,
                'type_id': disk.type_id,
                'mode': self.DISK_MODE[disk_attach_params.mode],
                'attach_status': self.DISK_A_STATUS[disk_attach_params.status],
                'status': self.DISK_STATUS[disk.status],
                'auto_delete': disk_attach_params.auto_delete,
                'disk_dashboard': get_nda_link(self.profile.prod_disk_dashbord.format(disk_attach_params.disk_id))
            }
            disk_dict = dict(disk_dict, **{'iops': f'R: {iops["RIOPS"]}/W: {iops["WIOPS"]}',
                                           'bandwidth': f'R: {iops["RBW"]} Mb/W: {iops["WBW"]} Mb'})
            result.append(disk_dict)
        return result

    @staticmethod
    def get_disk_image(disk):
        if disk.source_image_id:
            return disk.source_image_id
        elif disk.source_snapshot_id:
            return f'from snapshot {disk.source_snapshot_id}'
        elif disk.labels.get('hystax_backup_id', None):
            return f"hystax backup\n{disk.labels.get('hystax_backup_id', 'unknown')}"
        else:
            return '-'

    def format_instance_full(self, instance, view='BASIC'):

        boot_disk = self.disk_get(instance.boot_disk.disk_id)
        instance_formatted = {
            'instance_id': instance.id,
            'name': instance.name,
            'description': instance.description or '-',
            'cloud_id': self.get_folder_by_id(instance.folder_id).cloud_id,
            'folder_id': instance.folder_id,
            'status': self.INSTANCE_STATUS[instance.status],
            'os_type': self.OS_TYPE[instance.boot_disk.os.type],
            'image': f'{self.get_disk_image(boot_disk)}' if boot_disk else None,
            'boot_disk_id': instance.boot_disk.disk_id,
            'fqdn': instance.fqdn,
            'zone': instance.zone_id,
            'platform_id': instance.platform_id,
            'compute_node': self.get_compute_node(instance.id),
            'vnc_port': f'oops - look in state.json',
            'preemptible': instance.scheduling_policy.preemptible,
            'cores': instance.resources.cores,
            'core_type': "exclusive" if instance.resources.core_fraction == 100 else "shared",
            'core_fraction': f'{instance.resources.core_fraction}%',
            'memory': printers.get_bytes_size_string(int(instance.resources.memory)),
            'created_at': timestamp_resolve_grpc(instance.created_at)
        }

        if instance.resources.nvme_disks > 0:
            instance_formatted[
                'local_nvme'] = f'{instance.resources.nvme_disks}';

        if instance.resources.gpus > 0:
            instance_formatted['gpu'] = f'{instance.resources.gpus}'

        ifaces = self.format_instance_interfaces(self.format_instances_interfaces(instance, short=1))

        instance_formatted = dict(instance_formatted, **ifaces)
        instance_formatted['folder_id'] = instance.folder_id

        usage_link = f'{get_nda_link(self.profile.solomon_instance_usage_url.format(instance.id))}'
        instance_formatted['usage_link'] = usage_link

        if view == 'FULL':
            metadata = ''
            for k, v in instance.metadata.items():
                metadata = f'{metadata}\n {k}: {v}'
            instance_formatted['metadata'] = metadata

        return instance_formatted

    @staticmethod
    def format_instance_interfaces(ifaces):
        try:
            default_iface = ifaces[0]
            if len(ifaces) > 1:
                default_iface = dict(default_iface,
                                     **{'total ifaces': f'{len(ifaces)}'})
        except IndexError:
            default_iface = {'network ifaces': 'No'}
        return default_iface

    @staticmethod
    def parse_security_groups(def_sg, sgs):
        c_blue = ''
        c_end = ''
        security_groups = ''

        if sgs:
            security_groups = '\n'.join(sgs)
        elif not sgs and def_sg:
            security_groups = f'{def_sg}{c_blue}(default){c_end}'
        elif (not sgs) and (not def_sg):
            security_groups = '-'

        return security_groups

    @staticmethod
    def parse_superflows(ff):
        c_blue = ''
        c_end = ''
        ff_parsed = ''
        for fe_flag in ff:
            if fe_flag.find('super-flow') != -1:
                try:
                    ff_parsed = f'{ff_parsed}{fe_flag}: {c_blue}True{c_end}\n'
                except KeyError:
                    ff_parsed = f'{fe_flag}: {c_blue}True{c_end}'
        return ff_parsed

    def format_instances_interfaces(self, instance, short=0):
        c_blue = ''
        c_end = ''
        result = []

        folder_id = instance.folder_id
        iface_attachments = self.get_network_attachments(instance.id, folder_id).network_interface_attachments

        for i in range(0, len(instance.network_interfaces)):
            folder_id = instance.folder_id  # again
            try:
                common = iface_attachments[i]
                internal = iface_attachments[i].internal_info

                sgs = list(instance.network_interfaces[i].security_group_ids)
                def_sg = self.get_network(common.network_id).default_security_group_id
                security_groups = self.parse_security_groups(def_sg, sgs)

                if folder_id != internal.subnet_folder_id:
                    folder_id = f'{c_blue}{internal.subnet_folder_id}{c_end}'

                if common.primary_v6_address.address:
                    internal_ip = f'{c_blue}v4{c_end}: {common.primary_v4_address.address or None}\n' \
                                  f'{c_blue}v6{c_end}:{common.primary_v6_address.address}'
                else:
                    internal_ip = f'{c_blue}v4{c_end}: {common.primary_v4_address.address or None}'

                iface = {'#': common.interface_index,
                         'folder_id': folder_id,
                         'internal_ip': internal_ip,
                         'public_ip': common.primary_v4_address.one_to_one_nat.address or '-',
                         'subnet_id': common.subnet_id,
                         'network_id': common.network_id,
                         's_groups': security_groups,
                         'etc': f'hbf enabled: {internal.hbf_enabled}\n '
                                f'{c_blue}tap_iface{c_end}: tap{common.instance_id[3:12:]}-{common.interface_index}',
                         'tap_iface': f'tap{common.instance_id[3:12:]}-{common.interface_index}'
                         }

                ff = [i for i in internal.feature_flags]

                iface['super_flow'] = self.parse_superflows(ff)
                result.append(iface)
            except AttributeError:
                print('No iface attachments found or some key error')
                quit()

        return result

    @staticmethod
    def format_address_by_ip(address_raw):
        resource = {}

        try:
            resource['address_id'] = address_raw.id
            if address_raw.network_load_balancer_id:
                resource['resource'] = 'network_load_balancer'
                resource['resource_id'] = address_raw.network_load_balancer_id
            elif address_raw.instance_id:
                resource['resource'] = 'instance'
                resource['resource_id'] = address_raw.instance_id
            else:
                resource['resource'] = 'None'
                resource['resource_id'] = 'None'

        except AttributeError:
            logging.error('Ip resource record invalid or missing')

        return resource

    def format_address_list(self, address_list):
        final = []
        for addr in address_list:
            resource = self.format_address_by_ip(self.get_address_by_ip(addr.address))
            ip_addr = self.format_address(addr)
            final.append(dict(resource, **ip_addr))
        return final

    @staticmethod
    def format_user_roles(user_roles):
        result = []
        resources = []
        for role in user_roles:
            if role.resource.id in resources:
                result[resources.index(role.resource.id)][
                    'account_roles'] = f"{result[resources.index(role.resource.id)]['account_roles']}\n{role.role_id}"
            else:
                result.append({
                    'resource_id': role.resource.id,
                    'resource_type': role.resource.type,
                    'account_roles': role.role_id
                })
                resources.append(role.resource.id)
        return result
