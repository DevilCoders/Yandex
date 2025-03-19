import logging
import printers
from helpers import *

# saint parts
from saint_cloud_components.st_auth import St_Auth
from saint_cloud_components.st_s3 import St_S3
from saint_cloud_components.st_billing import St_Billing
from saint_cloud_components.st_cloud import St_Cloud
from saint_cloud_components.st_user import St_User
from saint_cloud_components.st_folder import St_Folder
from saint_cloud_components.st_disk import St_Disk
from saint_cloud_components.st_mdb import St_MDB
from saint_cloud_components.st_operation import St_Operation
from saint_cloud_components.st_k8s import St_K8s
from saint_cloud_components.st_network import St_Network
from saint_cloud_components.st_instance import St_Instance
from saint_cloud_components.st_compute_host import St_ComputeHost

# grpc channels
from grpc_gw import *
home_dir = expanduser('~')
config = configparser.RawConfigParser()
config.read('{}/.rei/rei.cfg'.format(home_dir))


class CloudFacade(St_Disk, St_Network, St_MDB, St_Operation, St_K8s, St_Auth, St_Cloud, St_Folder, St_User, St_S3,
                  St_Billing, St_Instance, St_ComputeHost, GrpcChannels):

    def __init__(self, profile: Profile, args):
        self.profile = profile
        self.endpoints = profile.endpoints

        self.FOLDER_STATUS = [
            f'{printers.Color.cyan}STATUS_UNSPECIFIED{printers.Color.END}',
            'ACTIVE',
            'DELETING',
            'DELETED',
            'PENDING_DELETION'
        ]

        self.cloud_stat = {
            0: f'{printers.Color.cyan}STATUS_UNSPECIFIED{printers.Color.END}',
            1: 'CREATING',
            2: 'ACTIVE',
            3: 'DELETING',
            4: f'{printers.Color.red}BLOCKED{printers.Color.END}',
            5: 'DELETED',
            6: 'PENDING DELETION'
        }
        self.DISK_MODE = {
            0: f'{printers.Color.cyan}MODE_UNSPECIFIED{printers.Color.END}',
            1: f'{printers.Color.yellow}READ{printers.Color.END}',
            2: f'{printers.Color.green}READ_WRITE{printers.Color.END}'
        }
        self.DISK_STATUS = {
            0: f'{printers.Color.cyan}STATUS UNSPECIFIED{printers.Color.END}',
            1: 'CREATING',
            2: f'{printers.Color.green}READY{printers.Color.END}',
            3: f'{printers.Color.red}ERROR{printers.Color.END}',
            4: 'DELETING'
        }
        self.DISK_A_STATUS = {
            0: f'{printers.Color.cyan}STATUS_UNSPECIFIED{printers.Color.END}',
            1: 'ATTACHING',
            2: 'ATTACHED',
            3: 'DETACHING',
            4: f'{printers.Color.red}DETACH_ERROR{printers.Color.END}'
        }
        self.OS_TYPE = {
            0: f'{printers.Color.cyan}OS_UNSPECIFIED{printers.Color.END}',
            1: 'LINUX',
            2: 'WINDOWS'
        }
        self.INSTANCE_STATUS = {
            0: f'{printers.Color.cyan}STATUS_UNSPECIFIED{printers.Color.END}',
            1: 'PROVISIONING',
            2: f'{printers.Color.green}RUNNING{printers.Color.END}',
            3: 'STOPPING',
            4: 'STOPPED',
            5: 'STARTING',
            6: 'RESTARTING',
            7: f'{printers.Color.yellow}UPDATING{printers.Color.END}',
            8: f'{printers.Color.red}ERROR{printers.Color.END}',
            9: f'{printers.Color.red}CRASHED{printers.Color.END}',
            10: f'{printers.Color.magenta}DELETING{printers.Color.END}'
        }
        self.IP_VERSION = {
            0: f'{printers.Color.cyan}IP_VERSION_UNSPECIFIED{printers.Color.END}',
            1: "IPV4",
            2: "IPV6"
        }
        self.IP_TYPE = {
            0: f'{printers.Color.cyan}TYPE_UNSPECIFIED{printers.Color.END}',
            1: 'INTERNAL',
            2: 'EXTERNAL'
        }
        self.SN_STATUS = {
            0: f'{printers.Color.cyan}STATUS_UNSPECIFIED{printers.Color.END}',
            1: 'CREATING',
            2: 'ACTIVE',
            3: 'DELETING'
        }
        self.MDB_ENVIRONMENT = {
            0: 'ENVIRONMENT_UNSPECIFIED',
            1: 'PRODUCTION',
            2: 'PRESTABLE'
        }
        self.KAFKA_HEALTH = {
            0: 'HEALTH_UNKNOWN',
            1: 'ALIVE',
            2: 'DEAD',
            3: 'DEGRADED'
        }
        self.KAFKA_STATUS = {
            0: 'STATUS_UNKNOWN',
            1: 'CREATING',
            2: 'RUNNING',
            3: 'ERROR',
            4: 'UPDATING',
            5: 'STOPPING',
            6: 'STOPPED',
            7: 'STARTING'
        }
        self.KAFKA_HOST_TYPE = {
            0: 'ROLE_UNSPECIFIED',
            1: 'KAFKA',
            2: 'ZOOKEEPER'
        }
        self.KAFKA_HOST_HEALTH = {
            0: 'UNKNOWN',
            1: 'ALIVE',
            2: 'DEAD',
            3: 'DEGRADED'
        }
        self.ELASTICSEARCH_HEALTH = {
            0: 'HEALTH_UNKNOWN',
            1: 'ALIVE',
            2: 'DEAD',
            3: 'DEGRADED'
        }
        self.ELASTICSEARCH_STATUS = {
            0: 'STATUS_UNKNOWN',
            1: 'CREATING',
            2: 'RUNNING',
            3: 'ERROR',
            4: 'UPDATING',
            5: 'STOPPING',
            6: 'STOPPED',
            7: 'STARTING'
        }
        self.ELASTICSEARCH_HOST_TYPE = {
            0: 'TYPE_UNSPECIFIED',
            1: 'DATA_NODE',
            2: 'MASTER_NODE'
        }
        self.ELASTICSEARCH_HOST_HEALTH = {
            0: 'UNKNOWN',
            1: 'ALIVE',
            2: 'DEAD',
            3: 'DEGRADED'
        }
        self.GREENPLUM_HEALTH = {
            0: 'HEALTH_UNKNOWN',
            1: 'ALIVE',
            2: 'DEAD',
            3: 'DEGRADED',
            4: 'UNBALANCED'
        }
        self.GREENPLUM_STATUS = {
            0: 'STATUS_UNKNOWN',
            1: 'CREATING',
            2: 'RUNNING',
            3: 'ERROR',
            4: 'UPDATING',
            5: 'STOPPING',
            6: 'STOPPED',
            7: 'STARTING'
        }
        self.GREENPLUM_HOST_HEALTH = {
            0: 'UNKNOWN',
            1: 'ALIVE',
            2: 'DEAD',
            3: 'DEGRADED',
            4: 'UNBALANCED'
        }
        self.GREENPLUM_HOST_TYPE = {
            0: 'TYPE_UNSPECIFIED',
            1: 'MASTER',
            2: 'REPLICA',
            3: 'SEGMENT'
        }
        self.SQLSERVER_HEALTH = {
            0: 'HEALTH_UNKNOWN',
            1: 'ALIVE',
            2: 'DEAD',
            3: 'DEGRADED'
        }
        self.SQLSERVER_STATUS = {
            0: 'STATUS_UNKNOWN',
            1: 'CREATING',
            2: 'RUNNING',
            3: 'ERROR',
            4: 'UPDATING',
            5: 'STOPPING',
            6: 'STOPPED',
            7: 'STARTING'
        }
        self.SQLSERVER_HOST_TYPE = {
            0: 'ROLE_UNKNOWN',
            1: 'MASTER',
            2: 'REPLICA'
        }
        self.SQLSERVER_HOST_HEALTH = {
            0: 'HEALTH_UNKNOWN',
            1: 'ALIVE',
            2: 'DEAD',
            3: 'DEGRADED'
        }
        self.args = args

    def format_address(self, address_raw):
        lbl = ''
        address = None
        try:
            for label, value in address_raw.labels.items():
                lbl = f"{lbl}{label}:{value}\n"
            lbl = lbl[:-2]  # Убрать лишнюю пустую строку в выводе
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
                'labels': folder.labels,
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

    def format_instances_list_full(self, instances):
        result = []
        for instance in instances:
            result.append(self.format_instance_full(instance))
        return result

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
            'vnc_port': f'{printers.Color.red}oops - look in state.json{printers.Color.END}',
            'preemptible': instance.scheduling_policy.preemptible,
            'cores': instance.resources.cores,
            'core_type': "exclusive" if instance.resources.core_fraction == 100 else "shared",
            'core_fraction': f'{instance.resources.core_fraction}%',
            'memory': printers.get_bytes_size_string(int(instance.resources.memory)),
            'created_at': timestamp_resolve_grpc(instance.created_at)
        }

        if instance.resources.nvme_disks > 0:
            instance_formatted[
                'local_nvme'] = f'{printers.Color.cyan}{instance.resources.nvme_disks}{printers.Color.END}';

        if instance.resources.gpus > 0:
            instance_formatted['gpu'] = f'{printers.Color.cyan}{instance.resources.gpus}{printers.Color.END}'

        ifaces = self.format_instance_interfaces(self.format_instances_interfaces(instance, short=1))

        instance_formatted = dict(instance_formatted, **ifaces)

        usage_link = f'{get_nda_link(self.profile.solomon_instance_usage_url.format(instance.id))}'
        instance_formatted['usage_link'] = usage_link

        if view == 'FULL':
            metadata = ''
            for k, v in instance.metadata.items():
                metadata = f'{metadata}\n {printers.Color.cyan}{k}{printers.Color.END}: {v}'
            instance_formatted['metadata'] = metadata

        return instance_formatted

    @staticmethod
    def format_instance_interfaces(ifaces):
        try:
            default_iface = ifaces[0]
            if len(ifaces) > 1:
                default_iface = dict(default_iface,
                                     **{'total ifaces': f'{printers.Color.blue}{len(ifaces)}{printers.Color.END}'})
        except IndexError:
            default_iface = {'network ifaces': 'No'}
        return default_iface

    @staticmethod
    def parse_security_groups(def_sg, sgs):
        c_blue = printers.Color.blue
        c_end = printers.Color.END
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
        c_blue = printers.Color.blue
        c_end = printers.Color.END
        ff_parsed = ''
        for fe_flag in ff:
            if fe_flag.find('super-flow') != -1:
                try:
                    ff_parsed = f'{ff_parsed}{fe_flag}: {c_blue}True{c_end}\n'
                except KeyError:
                    ff_parsed = f'{fe_flag}: {c_blue}True{c_end}'
        return ff_parsed

    def format_instances_interfaces(self, instance, short=0):
        c_blue = printers.Color.blue
        c_end = printers.Color.END
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
            resource['address_id'] = address_raw.address_id
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


    @staticmethod
    def format_compute_host_info(compute_host):
        compute_host_formatted = {
            'name': compute_host.name,
            'active': compute_host.active,
            'enabled': compute_host.enabled,
            'zone_id': compute_host.zone_id,
            'hardware_platform': compute_host.hardware_platform,
            'memory': compute_host.resources.memory,
            'cores': compute_host.resources.cores,
            'dedicated': compute_host.dedicated,
            'dedicated_host_id': compute_host.dedicated_host_id,
            'dedicated_group_id': compute_host.dedicated_group_id,
            'dedicated_server_id': compute_host.dedicated_server_id,
        }
        return compute_host_formatted

    def cluster_formatted(self, cluster):
        if cluster["type"] == "kafka":
            get = cluster.get("get")
            listhosts = cluster.get("listhosts")
            config = get.config
            resources = config.kafka.resources
            hosts_list = list(listhosts.hosts)
            hosts = []
            result = {
                "type": "kafka",
                "cluster_id": str(cluster["cluster_id"]),
                "cloud_id": self.get_folder_by_id(str(get.folder_id)).cloud_id,
                "folder_id": str(get.folder_id),
                "name": str(get.name),
                "env": self.MDB_ENVIRONMENT[int(get.environment)],
                "health": str(self.KAFKA_HEALTH[int(get.health)]),
                "status": str(self.KAFKA_STATUS[int(get.status)]),
                "created_at": str(datetime.utcfromtimestamp(int(get.created_at.seconds)).strftime('%Y-%m-%d %H:%M:%S')),
                "net_id": str(get.network_id),
                "subnet_id": hosts_list[0].subnet_id,
                "sgs_id": self.format_sg_by_network_id(str(get.network_id)),
                "host_class": resources.resource_preset_id,
                "disk_type": resources.disk_type_id,
                "disk_size": str(int((int(resources.disk_size) / 1073741824))) + " GB",
                "version": config.version,
                "usage_link": get_nda_link(self.profile.solomon_cluster_usage_url.format(str(cluster["cluster_id"]), list(listhosts.hosts)[0].name))
            }
            for host in hosts_list:
                instance_name = str(host.name).split('.')[0]
                host_result = {
                    "name": host.name,
                    "instance_id": self.use_grpc(self.instance_service_channel.List,
                                        instance_service_pb2.ListInstancesRequest(view='BASIC',
                                                                                  folder_id='b1gdepbkva865gm1nbkq',
                                                                                      filter=f'name="{instance_name}"')).instances[0].id,
                    "role": self.KAFKA_HOST_TYPE[int(host.role)],
                    "health": self.KAFKA_HOST_HEALTH[int(host.health)],
                    "public": host.assign_public_ip
                }
                hosts.append(host_result)

            result['hosts'] = hosts

        elif cluster["type"] == "elasticsearch":
            get = cluster.get("get")
            listhosts = cluster.get("listhosts")
            config = get.config
            resources = config.elasticsearch.data_node.resources
            hosts_list = list(listhosts.hosts)
            hosts = []
            result = {
                "type": "elasticsearch",
                "cluster_id": cluster["cluster_id"],
                "cloud_id": self.get_folder_by_id(str(get.folder_id)).cloud_id,
                "folder_id": str(get.folder_id),
                "name": str(get.name),
                "env": self.MDB_ENVIRONMENT[int(get.environment)],
                "health": str(self.ELASTICSEARCH_HEALTH[int(get.health)]),
                "status": str(self.ELASTICSEARCH_STATUS[int(get.status)]),
                "created_at": datetime.utcfromtimestamp(int(get.created_at.seconds)).strftime('%Y-%m-%d %H:%M:%S'),
                "net_id": str(get.network_id),
                "subnet_id": hosts_list[0].subnet_id,
                "sgs_id": self.format_sg_by_network_id(str(get.network_id)),
                "host_class": resources.resource_preset_id,
                "disk_type": resources.disk_type_id,
                "disk_size": str(int((int(resources.disk_size) / 1073741824))) + " GB",
                "version": config.version,
                "usage_link": get_nda_link(self.profile.solomon_cluster_usage_url.format(str(cluster["cluster_id"]), list(listhosts.hosts)[0].name))
            }
            for host in hosts_list:
                instance_name = str(host.name).split('.')[0]
                host_result = {
                    "name": host.name,
                    "instance_id": self.use_grpc(self.instance_service_channel.List,
                                        instance_service_pb2.ListInstancesRequest(view='BASIC',
                                                                                  folder_id='b1gdepbkva865gm1nbkq',
                                                                                      filter=f'name="{instance_name}"')).instances[0].id,
                    "role": self.ELASTICSEARCH_HOST_TYPE[int(host.type)],
                    "health": self.ELASTICSEARCH_HOST_HEALTH[int(host.health)],
                    "public": host.assign_public_ip
                }
                hosts.append(host_result)

            result['hosts'] = hosts

        elif cluster["type"] == "greenplum":
            get = cluster.get("get")
            listmasterhosts = cluster.get("listmasterhosts")
            listsegmenthosts = cluster.get("listsegmenthosts")
            config = get.config
            resources = get.master_config.resources
            hosts_list = list(listmasterhosts.hosts) + list(listsegmenthosts.hosts)
            hosts = []
            result = {
                "type": "greenplum",
                "cluster_id": cluster["cluster_id"],
                "cloud_id": self.get_folder_by_id(str(get.folder_id)).cloud_id,
                "folder_id": str(get.folder_id),
                "name": str(get.name),
                "env": self.MDB_ENVIRONMENT[int(get.environment)],
                "health": str(self.GREENPLUM_HEALTH[int(get.health)]),
                "status": str(self.GREENPLUM_STATUS[int(get.status)]),
                "created_at": datetime.utcfromtimestamp(int(get.created_at.seconds)).strftime('%Y-%m-%d %H:%M:%S'),
                "net_id": str(get.network_id),
                "subnet_id": hosts_list[0].subnet_id,
                "sgs_id": self.format_sg_by_network_id(str(get.network_id)),
                "host_class": resources.resource_preset_id,
                "disk_type": resources.disk_type_id,
                "disk_size": str(int((int(resources.disk_size) / 1073741824))) + " GB",
                "version": config.version,
                "usage_link": get_nda_link(self.profile.solomon_cluster_usage_url.format(str(cluster["cluster_id"]), list(listmasterhosts.hosts)[0].name))
            }
            for host in hosts_list:
                instance_name = str(host.name).split('.')[0]
                host_result = {
                    "name": host.name,
                    "instance_id": self.use_grpc(self.instance_service_channel.List,
                                        instance_service_pb2.ListInstancesRequest(view='BASIC',
                                                                                  folder_id='b1gdepbkva865gm1nbkq',
                                                                                      filter=f'name="{instance_name}"')).instances[0].id,
                    "role": self.GREENPLUM_HOST_TYPE[int(host.type)],
                    "health": self.GREENPLUM_HOST_HEALTH[int(host.health)],
                    "public": host.assign_public_ip
                }
                hosts.append(host_result)

            result['hosts'] = hosts

        elif cluster["type"] == "sqlserver":
            get = cluster.get("get")
            listhosts = cluster.get("listhosts")
            config = get.config
            resources = config.resources
            hosts_list = list(listhosts.hosts)
            hosts = []
            result = {
                "type": "sqlserver",
                "cluster_id": cluster["cluster_id"],
                "cloud_id": self.get_folder_by_id(str(get.folder_id)).cloud_id,
                "folder_id": str(get.folder_id),
                "name": str(get.name),
                "env": self.MDB_ENVIRONMENT[int(get.environment)],
                "health": str(self.SQLSERVER_HEALTH[int(get.health)]),
                "status": str(self.SQLSERVER_STATUS[int(get.status)]),
                "created_at": datetime.utcfromtimestamp(int(get.created_at.seconds)).strftime('%Y-%m-%d %H:%M:%S'),
                "net_id": str(get.network_id),
                "subnet_id": hosts_list[0].subnet_id,
                "sgs_id": str(get.security_group_ids),
                "host_class": resources.resource_preset_id,
                "disk_type": resources.disk_type_id,
                "disk_size": str(int((int(resources.disk_size) / 1073741824))) + " GB",
                "version": config.version,
                "usage_link": get_nda_link(self.profile.solomon_cluster_usage_url.format(str(cluster["cluster_id"]),
                                                                                         list(listhosts.hosts)[0].name))
            }
            for host in hosts_list:
                instance_name = str(host.name).split('.')[0]
                host_result = {
                    "name": host.name,
                    "instance_id": self.use_grpc(self.instance_service_channel.List,
                                        instance_service_pb2.ListInstancesRequest(view='BASIC',
                                                                                  folder_id='b1gdepbkva865gm1nbkq',
                                                                                      filter=f'name="{instance_name}"')).instances[0].id,
                    "health": self.GREENPLUM_HOST_HEALTH[int(host.health)],
                    "public": host.assign_public_ip
                }
                hosts.append(host_result)

            result['hosts'] = hosts

        return result
