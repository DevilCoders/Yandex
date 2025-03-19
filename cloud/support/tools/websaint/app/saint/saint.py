#!/usr/bin/env python3
# coding: utf-8
import http.client as http_client
import re
import printers
from parser import get_argparser
from requests.packages.urllib3.exceptions import InsecureRequestWarning
from helpers import *
from grpc_gw import *
from cloudfacade import CloudFacade

requests.packages.urllib3.disable_warnings(InsecureRequestWarning)

# Config init
profiles = None
home_dir = expanduser('~')
config = configparser.RawConfigParser()
config.read('./rei.cfg'.format(home_dir))


try:
    # ca_cert = config.get('CA', 'cert')
    # open(ca_cert, 'r')
    os.environ['REQUESTS_CA_BUNDLE'] = './allCA.crt'
    profiles = load_profiles_from_config(config)
except (FileNotFoundError, ValueError, configparser.NoSectionError, configparser.NoOptionError) as e:
    logging.debug(e)
    print('\nCorrupted config or no config file present.\nInitialization...')
    init_config_setup()
    quit()


# ya make argparse bypass error
def main():
    pass


'''
CLOUDINC
Фильтрация и сортировка списка ресурсов по:
- платный/триал/сервисный
- бизнес/личный
- активный/заблокированный
'''


def resources_filer():
    pass


# Arguments parse
parser = get_argparser()
args = parser.parse_args()

if args.debug:
    http_client.HTTPConnection.debuglevel = 1
    logging.basicConfig(format='[%(asctime)s] [%(levelname)s] %(message)s',
                        datefmt='%D %H:%M:%S',
                        level=logging.DEBUG)
else:
    logging.basicConfig(format='[%(levelname)s] %(message)s',
                        level=logging.WARNING)



if args.profile:
    if args.profile not in profiles:
        logging.error('Unknown profile {}. Available profiles are: {}'.format(args.profile, ','.join(profiles.keys())))
        logging.info('View ~/.rei/rei.conf for details, edit it for adding new profile.')
        quit()

    cloud_facade = CloudFacade(profile=profiles[args.profile], args=args)
elif args.pre:
    cloud_facade = CloudFacade(profile=profiles['preprod'], args=args)
else:
    cloud_facade = CloudFacade(profile=profiles[WellKnownProfiles.prod.name], args=args)

if args.cloud:
    """
    --cloud', '-c'
    """

    cloud = cloud_facade.get_cloud_by_id(args.cloud)
    if cloud:
        printers.CloudPrinter(args).print(cloud_facade.format_cloud(cloud))

        if args.all_folders:
            """
            '--all-folders', '-af'
            """
            folders = cloud_facade.folders_get_by_cloud(args.cloud)
            if folders:
                printers.FolderListPrinter(args).print(cloud_facade.format_folders(folders))
            else:
                print(f"Cloud {args.cloud} is empty")
        elif args.users:
            """
            '--users', '-ru'
            """
            users = cloud_facade.get_all_users(args.cloud)
            if users:
                printers.UserListPrinter(args).print(users)
            else:
                print(f"No users in cloud {args.cloud}. Check reaper")
        elif args.billing_account:
            """
            '--billing-account', '-ba'
            """
            billing = cloud_facade.resolve_billing(args.cloud, id_type='cloud_id')
            if billing:
                printers.BillingAccountPrinter(args).print(cloud_facade.get_billing_full(billing))
        elif args.owners:
            """
            '--owners'
            """
            owners = cloud_facade.get_owners(args.cloud)
            if owners:
                printers.UserListPrinter(args).print(owners)
            else:
                print(f"No owners found for cloud {args.cloud}")
    else:
        print(f'No cloud {args.cloud} found. Check if you entered correct id and use appropriate profile')
elif args.get_mk8s_masters:
    """
    cloud_group.add_argument('--get-mk8s-masters', '-gmk8s', type=str, help='return mk8s masters by mk8s cid')
    """
    instances_by_folder = cloud_facade.gmk8s_master_list(args.get_mk8s_masters)

    if len(instances_by_folder) == 1:
        printers.InstancePrinter(args).print(cloud_facade.format_instance_full(instances_by_folder[0], view='BASIC'))
        disks = cloud_facade.get_instances_disks(instances_by_folder[0])
        if disks[0]['disk'].disk_id:
            dl = cloud_facade.format_disk_list(disks)
            printers.DiskListPrinter(args).print(dl)
        else:
            print ("Instance is starting or has no disks")
    elif len(instances_by_folder) > 2:
        if args.verbose_masters:
            for master_instance in instances_by_folder:
                printers.InstancePrinter(args).print(
                    cloud_facade.format_instance_full(instances_by_folder[0], view='BASIC'))
                disks = cloud_facade.get_instances_disks(instances_by_folder[0])

                if disks[0]['disk'].disk_id:
                    printers.DiskListPrinter(args).print(cloud_facade.format_disk_list(disks))
                else:
                    print("Instance is starting or has no disks")
        else:
            regional_cluster_masters = []
            for master_instance in instances_by_folder:
                regional_cluster_masters.append(cloud_facade.format_instance_full(master_instance))
            printers.InstanceListPrinter(args).print(regional_cluster_masters)
    else:
        print(f"Wrong cluster name or no cluster {args.get_mk8s_masters} found")
elif args.billing:
    """
    cloud_group.add_argument('--billing', '-b', metavar='BA_ID', type=str, help='resolve billing ID.')
    """
    billing = cloud_facade.get_billing_full(args.billing)
    if billing:
        printers.BillingAccountPrinter(args).print(billing)

        if args.grants:
            print('ACTIVE GRANTS:')
            grants = cloud_facade.get_grants(args.billing)
            if grants:
                printers.MonetaryGrantsPrinter(args).print(grants)
            else:
                print("No active grants")
        if args.dead_grants:
            print('INACTIVE GRANTS:')
            dead_grants = cloud_facade.get_grants(args.billing, True)
            if dead_grants:
                printers.MonetaryGrantsPrinter(args).print(dead_grants)
            else:
                print("No inactive grants found")
        if args.payments:
            payments = cloud_facade.get_payments_history(args.billing)
            if payments:
                printers.PaymentsPrinter(args).print(payments)
            else:
                print("No payments found")
        billing_account_clouds = cloud_facade.get_clouds_by_billing_account_id(args.billing)
        if billing_account_clouds:
            clouds = [cloud_facade.format_cloud(cloud_facade.get_cloud_by_id(cloud.get('id'))) for cloud in billing_account_clouds]
            print('CLOUDS:')
            printers.CloudListPrinter(args).print(clouds)
        else:
            logging.warning('Clouds not found for {}'.format(args.billing))
    else:
        print(f"No billing account {args.billing} found")

#ToDo: отсюда и далее. Проверяем, не None ли между получением данных и отправкой их на форматирование
elif args.folder:
    """
    '--folder', '-f'
    """
    folder = cloud_facade.format_folder(cloud_facade.get_folder_by_id(args.folder))
    if folder:
        cloud = cloud_facade.format_cloud(cloud_facade.get_cloud_by_id(folder.get('cloudId')))
        printers.CloudPrinter(args).print(cloud)
        printers.FolderListPrinter(args).print([folder])
    else:
        print(f"Folder {args.folder} not found")
elif args.bucket:
    """
    '--bucket', '-s3'
    """
    if args.move_to:
        """
        '--move-to', '-mv'
        """
        cloud_facade.move_bucket(args.bucket, args.move_to)
    else:
        if re.fullmatch(r'[a-z0-9]{20}', args.bucket):
            buckets = cloud_facade.get_all_buckets(args.bucket)
            if buckets:
                printers.BucketListPrinter(args).print(buckets)
        else:
            bucket = cloud_facade.cloud_get_by_bucket(args.bucket)
            if bucket:
                printers.BucketPrinter(args).print(bucket)

elif args.instance:
    """
    '--instance', '-i'
    """
    view = 'FULL' if args.metadata else 'BASIC'
    instance = cloud_facade.get_instance(args.instance, view='FULL')
    if instance:
        printers.InstancePrinter(args).print(cloud_facade.format_instance_full(instance, view=view))

        if args.operations:
            """
            '--operations', '-o'
            """

            operations = [cloud_facade.format_operation(op) for op in
                          cloud_facade.instance_operations_list(args.instance)]
            if operations:
                printers.OperationListPrinter(args).print(operations)
            else:
                print(f"No recent operations for instance {instance}. Try YQL")
        if args.disks:
            """
            '--disks', '-ad'
            """
            disks = cloud_facade.get_instances_disks(instance)
            if disks:
                printers.DiskListPrinter(args).print(cloud_facade.format_disk_list(disks))
            else:
                print(f'No disks found for instance {instance}')
        if args.nets:
            """
            '--nets', '-ni'
            """
            nets = cloud_facade.format_instances_interfaces(instance)
            if nets:
                printers.NetListPrinter(args).print_as_table(nets)
            else:
                print(f'Instance {instance} has no network interfaces (wat?)')
elif args.disk:
    """
    instance_group.add_argument('--disk', '-d', type=str, help='show disk info and resolve to cloud_id')
    """
    disk = cloud_facade.disk_get(args.disk)
    if disk:
        printers.DiskPrinter(args).print(cloud_facade.format_disk(disk))

elif args.ip_address:
    """
    instance_group.add_argument('--ip-address', '-ip', type=str, metavar='IP', help='resolve instance by IP-address')
    """

    ip_address = cloud_facade.get_address_by_ip(args.ip_address)
    if ip_address:
        printers.SimplePrinter(args).print(cloud_facade.format_address(ip_address))
        if ip_address.instance_id:
            print("resource type: Cloud Instance")
            instance = cloud_facade.get_instance(ip_address.instance_id, view='FULL')
            if instance:
                printers.InstancePrinter(args).print(cloud_facade.format_instance_full(instance))
            else:
                print(f"No instance {ip_address.instance_id} found")
            if args.operations:
                """
                addons_group.add_argument('--operations', '-o', action='store_true', help='show resource operations')
                """
                operations = [cloud_facade.format_operation(op) for op in cloud_facade.instance_operations_list(ip_address.instance_id)]
                if operations:
                    printers.OperationListPrinter(args).print(operations)
                else:
                    print(f"No recent operations for instance {instance}. Try YQL")
            if args.disks:
                """
                '--disks', '-ad'
                """
                disks = cloud_facade.get_instances_disks(instance)
                if disks:
                    printers.DiskListPrinter(args).print(cloud_facade.format_disk_list(disks))
                else:
                    print(f'Instance {ip_address.instance_id} has no disks')
            if args.nets:
                """
                '--nets', '-ni'
                """
                nets = cloud_facade.format_instances_interfaces(instance)
            else:
                print(f'Instance {instance} has no network interfaces (wat?)')
        if ip_address.network_load_balancer_id:
            print("Resource_type: Network Load Balancer")

elif args.all_instances:
    """
    '--all-instances', '-ai'
    """
    folders_list = cloud_facade.format_folders(cloud_facade.folders_get_by_cloud(args.all_instances))
    if folders_list:
        instances_by_folder = cloud_facade.get_all_instances(folders_list)
        for folder, instances in instances_by_folder.items():
            (folder_id, folder_name) = folder
            if len(instances) == 0 and not args.json and not args.key:
                logging.warning('Folder {} is empty'.format(folder_id))
            else:
                if not args.json and not args.key:
                    print('\nFolder: {name}, ID: {id}'.format(name=folder_name, id=folder_id))
                printers.InstanceListPrinter(args).print(instances)
        else:
            print(f"No instances found in cloud {args.all_instances}")
    else:
        print(f"No folders found in cloud {args.all_instances}")

elif args.account:
    """
    '--account', '-acc'
    """
    if args.organization:
        user_acc = cloud_facade.account_get_from_organization(args.account, args.organization).members[0].subject_claims
        if user_acc:
            user_account = format_account(user_acc, 1)
    else:
        user_acc = cloud_facade.account_get_by_login(args.account)
        if user_acc:
            user_account = format_account(*user_acc)
    if user_account:
        printers.SimplePrinter(args).print(user_account)

    ua = user_account.get('account_id') if user_account.get('account_id') else user_account.get('id')

    user_roles = cloud_facade.get_user_roles(ua)
    if user_roles:
        printers.AccountRolesPrinter(args).print(cloud_facade.format_user_roles(user_roles))
    else:
        print(f"No permissions assigned to user {args.account}")
elif args.clusters:
    """
    '--clusters', '-cr'
    """
    clusters = cloud_facade.get_all_clusters(args.clusters)
    if clusters:
        printers.MdbClusterListPrinter(args).print(clusters)
    else:
        print(f"No MDB clusters found in folder {args.clusters}")

elif args.managed_db:
    cluster = None
    """
    '--managed-db', '-mdb'
    """
    if args.id:
        """
        '--id'
        """
        cluster = cloud_facade.cluster_resolve(args.id, id_type='clusterId')
    elif args.fqdn:
        """
        '--fqdn'
        """
        cluster = cloud_facade.cluster_resolve(args.fqdn, id_type='fqdn')
    elif args.shard:
        """
        '--shard'
        """
        cluster = cloud_facade.cluster_resolve(args.shard, id_type='shardId')
    elif args.node:
        """
        '--node'
        """
        cluster = cloud_facade.cluster_resolve(args.node, id_type='instanceId')
    else:
        logging.error('Pass --id, --fqdn, --shard or --node')
        cluster = None
    if cluster:
        printers.MdbClusterPrinter(args).print(cluster)

        if args.operations:
            """
            '--operations', '-o'
            """
            cluster_id = cluster.get('cluster_id')
            db_type = cluster.get('type')
            operations = cloud_facade.cluster_operation_get(cluster_id, db_type=db_type)

            printers.OperationListPrinter(args).print(operations)
        printers.MdbHostsListPrinter(args).print(cluster.get('hosts', []))
    else:
        print(f"Unable to find cluster {args.managed_db}")


elif args.addresses:
    """
    '--addresses', '-ips'
    """
    addresses = cloud_facade.addresses_list_by_cloud(args.addresses)
    if addresses:
        printers.IpAddressListPrinter(args).print(cloud_facade.format_address_list(addresses))
    else:
        print(f"No addresses found in cloud {args.addresses}")

elif args.init:
    """
    '--init'
    """
    init_config_setup()

elif args.subnets:
    """
    '--subnets', '-sn'
    """
    raw_subnets = cloud_facade.subnets_list_by_cloud(args.subnets).subnets
    if raw_subnets:
        subnets = [cloud_facade.format_subnet(sn) for sn in raw_subnets]
        printers.SubnetListPrinter(args).print(subnets)

elif args.get_node:
    """
    '--get-node', '-gn'
    """
    print(cloud_facade.get_compute_node(args.get_node))

elif args.clear_cache:
    """
    addons_group.add_argument('--clear-cache', '-cc', action='store_true', help='clean cache for IAM-token')
    """
    cloud_facade.clear_iam_token_cache()

elif args.promocode:
    """
    cloud_group.add_argument('--promocode', '-p', type=str, help='check promocode')
    """
    promocode = cloud_facade.check_promocode(args.promocode)
    if promocode:
        printers.PromocodePrinter(args).print(promocode)
    else:
        print(f"No promocode {args.promocode} found")
elif args.bin_check:
    """
    cloud_group.add_argument('--bin-check', '-bin', type=str, help='show card info by BIN (6 first digits)')
    """
    data = cloud_facade.bin_check(args.bin_check)
    if data:
        printers.BankCardPrinter(args).print(data)
    else:
        print(f"Unknown bin {args.bin_check}")
elif args.standard_images:
    """
    addons_group.add_argument('--standard-images', '-simg', action='store_true', help='get standard images list')
    """
    data = cloud_facade.folder_images_list()
    if data:
        printers.StandardImagesPrinter(args).print(cloud_facade.format_images_list(data))
    else:
        print("Standatd images folder is empty or wat? Contact @the-nans")
elif args.get_image:
    """
     '--get-image', '-img'
    """
    data = cloud_facade.format_disk_image(cloud_facade.check_source_image(args.get_image))
    if data:
        printers.SimplePrinter(args).print(data)
    else:
        print(f"Image {args.get_image} does not exist")
else:
    logging.error('Unknown command. Use saint --help for more details.')
