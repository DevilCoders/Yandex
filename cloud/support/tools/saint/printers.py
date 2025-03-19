import abc
import copy
import json
import logging
import textwrap
from typing import Tuple

import yaml
from prettytable import PrettyTable


class Color:
    red = '\033[91m'
    green = '\033[92m'
    yellow = '\033[93m'
    blue = '\033[94m'
    magenta = '\033[95m'
    cyan = '\033[96m'
    white = '\033[97m'
    grey = '\033[90m'
    bold = '\033[1m'
    italic = '\033[3m'
    underline = '\033[4m'
    END = '\033[0m'


# Bytes to human-readable output wrapper
def get_bytes_size_string(bytes_size, granularity=2):
    result = []
    sizes = (
        ('TB', 1024 ** 4),
        ('GB', 1024 ** 3),
        ('MB', 1024 ** 2),
        ('KB', 1024),
        ('B', 1)
    )

    if bytes_size is None or bytes_size == 0:
        return bytes_size

    for name, count in sizes:
        value = bytes_size // count
        if value:
            bytes_size -= value * count
            result.append('{v} {n}'.format(v=value, n=name))
    return ', '.join(result[:granularity])


class AbstractPrinter(abc.ABC):
    def __init__(self, args):
        super().__init__()
        self.args = args

    @abc.abstractmethod
    def print_one_key(self, data, key):
        pass

    @abc.abstractmethod
    def print_as_json(self, data):
        pass

    @abc.abstractmethod
    def print_as_yaml(self, data):
        pass

    @abc.abstractmethod
    def print_as_table(self, data):
        pass

    def print(self, data):
        try:
            if self.args.key:
                self.print_one_key(data, self.args.key)
            elif self.args.json:
                self.print_as_json(data)
            elif self.args.yaml:
                self.print_as_yaml(data)
            else:
                self.print_as_table(data)
        except AttributeError as e:
            self.print_as_table(data)


class SimplePrinter(AbstractPrinter):
    def print_one_key(self, data, key):
        print(data.get(key))

    def print_as_json(self, data):
        print(json.dumps(data, ensure_ascii=False))

    def print_as_yaml(self, data):
        print(yaml.dump(data, allow_unicode=True, sort_keys=False))

    def print_as_table(self, data):
        """
        Thinks that data is a dict and prints it as NAME → VALUE dict
        """
        table = PrettyTable()
        table.field_names = ['NAME', 'VALUE']
        table.align = 'l'
        for key, value in data.items():
            key, value = self.colorize_table_row(key, value)
            table.add_row([key, value])

        print(table)

    def colorize_table_row(self, key: str, value: str):
        """
        Override this method for custom colorization of your table rows. By default does nothing.
        See examples below.
        """
        return key, value


class ListPrinter(AbstractPrinter):
    # Override these fields in subclass for customization:
    column_names = ()
    default_sort_by = None

    # There are optional fields too, but you also can override them in subclass:
    align = None

    def print_one_key(self, data, key):
        for element in data:
            print(element.get(key))

    def print_as_yaml(self, data):
        for element in data:
            print(yaml.dump(element, allow_unicode=True, sort_keys=False))

    def print_as_json(self, data):
        for element in data:
            print(json.dumps(element, ensure_ascii=False))

    def _convert_object_to_table_row(self, obj):
        row = []
        for column_name in self.column_names:
            if type(column_name) is tuple:
                # Iterate over column names to find first presented
                for name in column_name:
                    if obj.get(name) is not None:
                        row.append(obj.get(name))
                        break
                else:
                    row.append(None)
            else:
                row.append(obj.get(column_name))

        return row

    def _get_field_names(self):
        names = []
        for column_name in self.column_names:
            if type(column_name) is tuple:
                names.append(column_name[0].upper())
            else:
                names.append(column_name.upper())
        return names

    def print_as_table(self, data):
        if not self.column_names:
            raise Exception('Internal error. {} doesn\'t have "column_names" field!'.format(self.__class__.__name__))

        table = PrettyTable()
        table.field_names = self._get_field_names()

        if self.align is not None:
            table.align = self.align

        for element in data:
            row = self._convert_object_to_table_row(element)
            row = list(self.colorize_table_row(element, *row))
            table.add_row(row)

        if self.default_sort_by is None:
            # By default, if default_sort_by is not specified, sort by first column
            self.default_sort_by = self.column_names[0]

        self.default_sort_by = self.default_sort_by.upper()

        # If --sort is not defined, sort by default key
        if not self.args.sort:
            print(table.get_string(sortby=self.default_sort_by))
            return

        # else sort by specified key
        try:
            print(table.get_string(sortby=self.args.sort.upper()))
        except Exception:
            logging.warning('Use default sort. Invalid field name received')
            print(table.get_string(sortby=self.default_sort_by))

    def colorize_table_row(self, obj: any, *fields) -> Tuple:
        """
        Override this method for custom colorization of your table rows. By default does nothing.
        See examples below.
        """
        return tuple(fields)


class CloudPrinter(SimplePrinter):
    def colorize_table_row(self, key: str, value: str):
        if value == 'BLOCKED':
            value = '{c1}{value}{c2}'.format(c1=Color.red, value=value, c2=Color.END)
        elif value == 'ACTIVE':
            value = '{c1}{value}{c2}'.format(c1=Color.green, value=value, c2=Color.END)
        elif value == 'CREATING':
            value = '{c1}{value}{c2}'.format(c1=Color.yellow, value=value, c2=Color.END)
        return key, value


class BillingAccountPrinter(SimplePrinter):
    def colorize_table_row(self, key: str, value: str):
        if value in ['TRIAL_EXPIRED', 'TRIAL_SUSPENDED', 'INACTIVE', 'SUSPENDED']:
            value = '{c1}{value}{c2}'.format(c1=Color.red, value=value, c2=Color.END)
        if value == 'paid':
            value = '{c1}{value}{c2}'.format(c1=Color.green, value=value, c2=Color.END)
        if value in ['ACTIVE', 'SERVICE']:
            value = '{c1}{value}{c2}'.format(c1=Color.green, value=value, c2=Color.END)
        return key, value


class BucketPrinter(SimplePrinter):
    @staticmethod
    def _convert(bucket):
        return {
            'bucket_name': bucket.get('name'),
            'cloud_id': bucket.get('cloud_id'),
            'folder_id': bucket.get('service_id'),
            'storage_class': bucket.get('default_storage_class'),
            'public': bucket.get('is_anonymous_read_enabled'),
            'used_space': get_bytes_size_string(bucket.get('used_space')),
            'max_space': get_bytes_size_string(bucket.get('max_size')) if bucket.get('max_size') else 'unlimited',
            'standard_obj_count': bucket.get('simple_objects_count'),
            'standard_size': get_bytes_size_string(bucket.get('simple_objects_size')),
            'cold_obj_count': bucket.get('cold_simple_objects_count'),
            'cold_size': get_bytes_size_string(bucket.get('cold_simple_objects_size')),
            'created_at': bucket.get('created_ts')
        }

    def print(self, data):
        super().print(self._convert(data))


class MetadataPrinter(SimplePrinter):
    def print_as_table(self, metadata):
        if type(metadata) is dict:
            for k, v in metadata.items():
                print(k, v)
        else:
            print(metadata)


class InstancePrinter(SimplePrinter):
    def colorize_table_row(self, key: str, value: str):
        if value in ('error', 'crashed', 'stopped', '5%', '20%', '50%') or \
            ('snapshot' in str(value) or 'userimage' in str(value)):
            value = '{c1}{value}{c2}'.format(c1=Color.red, value=value, c2=Color.END)
        if value == 'running':
            value = '{c1}{value}{c2}'.format(c1=Color.green, value=value, c2=Color.END)
        if key == 'metadata':
            value = '\n'.join([f'{textwrap.fill(i, width=80)}' if len(i) > 71 else i for i in value.split('\n')])
        if key == 'description':
            value = textwrap.fill(value, width=80)
        return key, value

    def print_as_table(self, data):
        super().print_as_table(data)

        if not self.args.metadata:
            print('Metadata display disabled. Use --metadata or -m to enable.')

    def print(self, data):
        data_copy = copy.copy(data)
        super().print(data_copy)


class NodePrinter(SimplePrinter):
    def print(self, data):
        data_copy = copy.copy(data)
        super().print(data_copy)


class DiskPrinter(SimplePrinter):
    pass


class MdbClusterPrinter(SimplePrinter):
    def print(self, data):
        data_copy = copy.copy(data)
        del data_copy['hosts']
        super().print(data_copy)


class PromocodePrinter(SimplePrinter):
    pass


class BankCardPrinter(SimplePrinter):
    pass


class FolderListPrinter(ListPrinter):
    column_names = (('folder_id', 'id'), 'status', 'name', ('created_at', 'createdAt'), 'description', 'link')
    default_sort_by = 'status'


class MonetaryGrantsPrinter(ListPrinter):
    column_names = ('Id', 'Start Date', 'End Date', 'Initial Amount', 'Consumed Amount', 'Reason Id')
    default_sort_by = 'End Date'

class PaymentsPrinter(ListPrinter):

    column_names = ('id', 'amount', 'created_at', 'modified_at', 'type', 'status', 'description')
    default_sort_by = 'created_at'

class UserListPrinter(ListPrinter):
    column_names = ('login', 'email', 'role', 'user_id', 'passport_uid', 'is_notified')
    default_sort_by = 'login'
    align = 'l'


class CloudListPrinter(ListPrinter):
    column_names = ('cloud_id', ('name', 'cloud_name'), 'status')
    default_sort_by = 'name'
    align = 'l'

    def colorize_table_row(self, cloud, cloud_id, name, status):
        if status in ['BLOCKED']:
            status = '{c1}{value}{c2}'.format(c1=Color.red, value=status, c2=Color.END)
            cloud_id = '{c1}{value}{c2}'.format(c1=Color.red, value=cloud_id, c2=Color.END)
            name = '{c1}{value}{c2}'.format(c1=Color.red, value=name, c2=Color.END)

        elif status in ['CREATING']:
            status = '{c1}{value}{c2}'.format(c1=Color.yellow, value=status, c2=Color.END)
            cloud_id = '{c1}{value}{c2}'.format(c1=Color.yellow, value=cloud_id, c2=Color.END)
            name = '{c1}{value}{c2}'.format(c1=Color.yellow, value=name, c2=Color.END)

        return cloud_id, name, status


class CloudListPrinterWRoles(ListPrinter):
    column_names = ('cloud_id', ('name', 'cloud_name'), 'status', 'roles')
    default_sort_by = 'name'
    align = 'l'

    def colorize_table_row(self, cloud, cloud_id, name, status, roles):
        if status in ['BLOCKED']:
            status = '{c1}{value}{c2}'.format(c1=Color.red, value=status, c2=Color.END)
            cloud_id = '{c1}{value}{c2}'.format(c1=Color.red, value=cloud_id, c2=Color.END)
            name = '{c1}{value}{c2}'.format(c1=Color.red, value=name, c2=Color.END)

        elif status in ['CREATING']:
            status = '{c1}{value}{c2}'.format(c1=Color.yellow, value=status, c2=Color.END)
            cloud_id = '{c1}{value}{c2}'.format(c1=Color.yellow, value=cloud_id, c2=Color.END)
            name = '{c1}{value}{c2}'.format(c1=Color.yellow, value=name, c2=Color.END)

        return cloud_id, name, status, '\n'.join(roles)


class BucketListPrinter(ListPrinter):
    column_names = (('folder_id', 'service_id'), 'name', ('storage_class', 'default_storage_class'),
                    ('public', 'is_anonymous_read_enabled'), 'used_space')
    default_sort_by = 'folder_id'
    align = 'l'

    def colorize_table_row(self, folder, folder_id, name, storage_class, public, used_space):
        return folder_id, name, storage_class, public, get_bytes_size_string(used_space)


class OperationListPrinter(ListPrinter):
    column_names = ('operation_id', 'created_at', 'modified_at', 'created_by', 'description', 'done', 'error')
    default_sort_by = 'created_at'
    align = 'l'

    def print_as_table(self, data):
        if len(data) == 0:
            logging.warning('Operations list is empty')
        else:
            super().print_as_table(data)


class DiskListPrinter(ListPrinter):
    column_names = ('disk_id', 'size', ('type', 'attach_type'), 'type_id', 'mode', 'attach_status', 'status',
                    'disk_dashboard', 'bandwidth', 'iops', 'auto_delete')
    default_sort_by = 'type'
    align = 'c'

    def colorize_table_row(self, disk, disk_id, size, type, type_id, mode, attach_status, status,
                           disk_dashboard, iops, bandwidth, deletable):
        if mode == 'READ_ONLY':
            mode = '{c1}{value}{c2}'.format(c1=Color.red, value=mode, c2=Color.END)

        return disk_id, size, type, type_id, mode, attach_status, status, disk_dashboard, iops, bandwidth, deletable


class NetListPrinter(ListPrinter):
    column_names = (
    '#', 'public_ip', 'internal_ip', 'folder_id', 'network_id', 'subnet_id', 's_groups', 'super_flow', 'etc')
    default_sort_by = '#'
    align = 'l'


class InstanceListPrinter(ListPrinter):
    column_names = ('instance_id', 'status', 'name', ('os', 'os_type'), 'zone', ('core%', 'core_fraction'),
                    ('cpu', 'cores'), 'memory', 'internal_ip', 'public_ip', 'subnet_id')
    default_sort_by = 'status'
    align = 'c'

    def colorize_table_row(self, instance, instance_id, status, name, os, zone, core, cpu, memory, internal_ip,
                           public_ip, subnet_id):
        if status in ('error', 'crashed'):
            instance_id = '{c1}{value}{c2}'.format(c1=Color.red, value=instance_id, c2=Color.END)
            status = '{c1}{value}{c2}'.format(c1=Color.red, value=status, c2=Color.END)

        return instance_id, status, name, os, zone, core, cpu, memory, internal_ip, public_ip, subnet_id

class ComputeNodeInstancesListPrinter(ListPrinter):
    column_names = ('instance_id', 'folder_id', 'status', 'name', ('os', 'os_type'), ('core%', 'core_fraction'),
                    ('cpu', 'cores'), ('preempt', 'preemptible'), 'memory', ('NVME', 'local_nvme'), 'gpu')
    default_sort_by = 'status'
    align = 'c'

    def colorize_table_row(self, instance, instance_id, folder_id, status, name, os, core, cpu, preemptible, memory,
                           local_nvme, gpu):
        if status in ('error', 'crashed'):
            instance_id = '{c1}{value}{c2}'.format(c1=Color.red, value=instance_id, c2=Color.END)
            status = '{c1}{value}{c2}'.format(c1=Color.red, value=status, c2=Color.END)

        return instance_id, folder_id, status, name, os, core, cpu, preemptible, memory, local_nvme, gpu

class MdbClusterListPrinter(ListPrinter):
    column_names = (
    ('id', 'cluster_id'), 'name', 'type', 'status', 'health', 'preset', 'folder_id', 'disk_type', 'disk_size')
    default_sort_by = 'type'
    align = 'c'

    def colorize_table_row(self, cluster, cluster_id, name, type, status, health, preset, folder_id, disk_type,
                           disk_size):
        if status in ('ERROR', 'CRASHED'):
            cluster_id = '{c1}{value}{c2}'.format(c1=Color.red, value=cluster_id, c2=Color.END)
            status = '{c1}{value}{c2}'.format(c1=Color.red, value=status, c2=Color.END)
        if health in ('DEGRADED', 'UNKNOWN', 'DEAD'):
            health = '{c1}{value}{c2}'.format(c1=Color.red, value=health, c2=Color.END)

        return cluster_id, name, type, status, health, preset, folder_id, disk_type, disk_size


class MdbHostsListPrinter(ListPrinter):
    column_names = ('name', 'instance_id', 'role', 'health', 'public')
    default_sort_by = 'role'
    align = 'c'

    def colorize_table_row(self, cluster, name, instance_id, role, health, public):
        if health in ('DEGRADED', 'UNKNOWN', 'DEAD'):
            health = '{c1}{value}{c2}'.format(c1=Color.red, value=health, c2=Color.END)
        return name, instance_id, role, health, public


class IpAddressListPrinter(ListPrinter):
    column_names = (
    'ip_address', 'ephemeral', 'used', 'zone', 'ddos', 'smtp', 'resource', 'resource_id', 'folder_id', 'address_id')
    default_sort_by = 'used'
    align = 'l'


class SubnetListPrinter(ListPrinter):
    column_names = (
    'subnet_id', 'network_id', 'zone', 'cidr_v4', 'name', 'egress_nat', 'route_table', 'dhcp', 'cidr_v6')
    default_sort_by = 'network_id'
    align = 'l'


class StandardImagesPrinter(ListPrinter):
    column_names = ('id', 'name', 'family', 'status', 'product_ids', 'description')
    default_sort_by = 'name'
    align = 'l'

    def colorize_table_row(self, obj, id, name, family, status, prduct_ids, description):
        description = textwrap.fill(description, width=50)
        return id, name, family, status, prduct_ids, description


class AccountRolesPrinter(ListPrinter):
    column_names = ('resource_id', 'resource_type', 'account_roles')
    default_sort_by = 'resource_type'
    align = 'l'
