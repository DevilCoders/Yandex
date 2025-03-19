from app.saint.cloudfacade import CloudFacade
from app.saint.helpers import *
import re


class WebSaint():
    cloudfacade = None

    def __init__(self):
        home_dir = expanduser('~')
        config = configparser.RawConfigParser()
        config.read('./rei.cfg'.format(home_dir))
        profiles = load_profiles_from_config(config)
        self.cloudfacade = CloudFacade(profile=profiles['prod'])

    def __new__(cls, *args, **kwargs):
        if not hasattr(cls, 'instance'):
            cls.instance = super(WebSaint, cls).__new__(cls)
        return cls.instance

    def get_folder_info(self, args):
        command = args['command']
        argument = args['argument']
        r_result = {}

        if command == 'f':
            folder = self.cloudfacade.get_folder_by_id(argument)
            if folder:
                r_result = self.cloudfacade.format_folder(folder)
            else:
                r_result = f"No folder {command}"

        return r_result


    def get_cloud_info(self, args):
        r_object = []
        result = {}
        command = args['command']
        argument = args['argument']
        subcommand = args['subcommand']

        if command == 'c':

            cloud = self.cloudfacade.get_cloud_by_id(argument)
            if cloud:
                r_object = self.cloudfacade.format_cloud(cloud)
                # if subcommand== 'af':
                #     """
                #     '--all-folders', '-af'
                #     """
                folders = self.cloudfacade.folders_get_by_cloud(argument)
                if folders:
                    result['folders'] = [self.cloudfacade.format_folder(i) for i in folders.folders]
                else:
                    result['folders'] = [f"Cloud {command} is empty", ]
                # elif subcommand== 'ru':
                #     """
                #     '--users', '-ru'
                #     """
                users = self.cloudfacade.get_all_users(argument)
                if users:
                    result['users'] = users
                else:
                    result['users'] = f"No users in cloud {command}. Check reaper"
                # elif subcommand== 'ba':
                #     """
                #     '--billing-account', '-ba'
                #     """
                billing = self.cloudfacade.resolve_billing(argument, id_type='cloud_id')
                if billing:
                    result['billing'] = self.cloudfacade.get_billing_full(billing)
                # elif subcommand== 'owners':
                #     """
                #     '--owners'
                #     """
                owners = self.cloudfacade.get_owners(argument)
                if owners:
                    result['owners'] = owners
                else:
                    result['owners'] = f"No owners found for cloud {command}"
        else:
            r_object = f'No cloud {command} found. Check if you entered correct id and use appropriate profile'

        result['object'] = r_object,
        # result['params'] = r_params
        return result

    def get_instance_info(self, args):
        r_result = {}

        instance = self.cloudfacade.get_instance(args['argument'], view='FULL')
        if instance:
            r_result['instance'] = self.cloudfacade.format_instance_full(instance, view='FULL')
            r_result['instance']['metadata'] = clean_metadata(r_result['instance']['metadata'])

            operations = [self.cloudfacade.format_operation(op) for op in
                          self.cloudfacade.instance_operations_list(args['argument'])]
            if operations:
                r_result['operations'] = operations
            else:
                r_result['operations'] = f"No recent operations for instance {instance}. Try YQL"
            disks = self.cloudfacade.get_instances_disks(instance)
            if disks:
                r_result['disks'] = self.cloudfacade.format_disk_list(disks)
            else:
                r_result['disks'] = f'No disks found for instance {instance}'
            nets = self.cloudfacade.format_instances_interfaces(instance)
            if nets:
                r_result['network_ifaces'] = nets
            else:
                r_result['network_ifaces'] = f'Instance {instance} has no network interfaces (wat?)'
        return r_result

    def get_ip_info(self, args):
        r_result = {}
        ip_address = self.cloudfacade.get_address_by_ip(args['argument'])
        if ip_address:
            r_result['ip_address'] = self.cloudfacade.format_address(ip_address)

            instance_id = self.cloudfacade.get_instance_by_ip(args['argument'])
            if instance_id:
                args2 = args.copy()
                args2['command'] = 'i'
                args2['argument'] = instance_id.instance_id
                instance = self.get_instance_info(args2)
                if instance:
                    r_result['resource'] = dict(**instance)
            else:
                r_result['resource'] = 'Target resource is load balancer'
        return r_result

    # small infos

    def get_disk_info(self, args):
        disk = self.cloudfacade.disk_get(args['argument'])
        if disk:
            return self.cloudfacade.format_disk(disk)
        else:
            return {}

    def get_disk_image_info(self, args):
        r_result = {}
        data = self.cloudfacade.check_source_image(args['argument'])
        if data:
            r_result = self.cloudfacade.format_disk_image(data)
        else:
            r_result = f"Image {args['argument']} does not exist"

        return r_result

    def get_bucket(self, args):
        r_result = {}
        bucket = self.cloudfacade.cloud_get_by_bucket(args['argument'])
        if bucket:
            r_result = self.cloudfacade.format_bucket(bucket)

        return r_result

    def get_account_info(self, args):
        r_result = {}
        user_account = None
        if args['subcommand'] == 'org' and args.get('subargument'):
            user_acc = self.cloudfacade.account_get_from_organization(args['argument'], args['subargument']).members[
                0].subject_claims
            if user_acc:
                user_account = format_account(user_acc, 1)
        else:
            user_acc = self.cloudfacade.account_get_by_login(args['argument'])
            if user_acc:
                user_account = format_account(*user_acc)
        if user_account:
            r_result['account'] = user_account

        ua = user_account.get('account_id') if user_account.get('account_id') else user_account.get('id')

        user_roles = self.cloudfacade.get_user_roles(ua)
        if user_roles:
            r_result['account_roles'] = self.cloudfacade.format_user_roles(user_roles.access_bindings)
        else:
            r_result['account_roles'] = f"No permissions assigned to user {args['argument']}"

        return r_result

    # list things

    def list_k8s_cluster_masters(self, args):
        r_object = {}

        instances_by_folder = self.cloudfacade.gmk8s_master_list(args['argument'])

        if len(instances_by_folder) == 1:
            r_object['master'] = self.cloudfacade.format_instance_full(instances_by_folder[0],
                                                                                        view='BASIC')

            disks = self.cloudfacade.get_instances_disks(instances_by_folder[0])
            if disks[0]['disk'].disk_id:
                r_object['master']['disks'] = self.cloudfacade.format_disk_list(disks)

            else:
                r_params = "Instance is starting or has no disks"
        elif len(instances_by_folder) > 2:

            if args['subcommand'] == 'verbom':
                for master_instance in instances_by_folder:

                    r_object[f'master-{master_instance.zone_id}'] = self.cloudfacade.format_instance_full(master_instance, view='BASIC')
                    disks = self.cloudfacade.get_instances_disks(master_instance)

                    if disks[0]['disk'].disk_id:
                        r_object[f'master-{master_instance.zone_id}']['disks'] = self.cloudfacade.format_disk_list(disks)
                    else:
                        r_object[f'master-{master_instance.zone_id}']['disks'] = ["Instance is starting or has no disks", ]

            else:
                for master_instance in instances_by_folder:
                    r_object[f'master-{master_instance.zone_id}'] = self.cloudfacade.format_instance_full(master_instance)
        else:
            r_object = f"Wrong cluster name or no cluster {args['argument']} found"

        return r_object

    def list_disk_standard_images(self, args):
        r_result = {}
        data = self.cloudfacade.folder_images_list()
        if data:
            r_result['standard_images'] = self.cloudfacade.format_images_list(data)
        else:
            r_result['standard_images'] = "Standatd images folder is empty or wat? Contact @the-nans"
        return r_result

    def list_all_instances(self, args):

        r_result = {'instances': []}
        folders_list = self.cloudfacade.format_folders(self.cloudfacade.folders_get_by_cloud(args['argument']))
        if folders_list:
            for folder in folders_list:
                folder_id = folder.get('id')
                ai = self.cloudfacade.get_instances_in_folder(folder_id)
                if ai:
                    for i in ai.instances:
                        print(f"{i.id} {i.folder_id}")
                        r_result['instances'].append(self.cloudfacade.format_instance_full(i))
        return r_result

    def list_all_subnets(self, args):
        r_result = {}
        raw_subnets = self.cloudfacade.subnets_list_by_cloud(args['argument'])

        if raw_subnets:
            subnets = [self.cloudfacade.format_subnet(sn) for sn in raw_subnets]
            r_result = subnets
        else:
            r_result['subnets'] = f"No subnets found in cloud {args['argument']}"
        return r_result

    def list_all_ips(self, args):

        addresses = self.cloudfacade.addresses_list_by_cloud(args['argument'])
        if addresses:
            r_result = self.cloudfacade.format_address_list(addresses)
        else:
            r_result = {'addresses': f"No addresses found in cloud {args['argument']}"}
        return r_result

    # billing stuff

    def get_billing_account(self, args):
        r_result = {}
        billing = self.cloudfacade.get_billing_full(args['argument'])
        if billing:
            r_result['billing'] = billing
            billing_account_clouds = self.cloudfacade.get_clouds_by_billing_account_id(args['argument'])
            if billing_account_clouds:
                r_result['billing']['clouds'] = [
                    self.cloudfacade.format_cloud(self.cloudfacade.get_cloud_by_id(cloud.get('id'))) for cloud in
                    billing_account_clouds]
            else:
                logging.warning('Clouds not found for {}'.format(args['argument']))

        return r_result

    def get_grants(self, args):
        r_result = {}

        grants = self.cloudfacade.get_grants(args['argument'])
        if grants:
            r_result['active_grants'] = grants
        else:
            r_result['active_grants'] = "No active grants"

        dead_grants = self.cloudfacade.get_grants(args['argument'], True)
        if dead_grants:
            r_result['dead_grants'] = dead_grants
        else:
            r_result['dead_grants'] = "No inactive grants found"

        return r_result

    def get_payment_history(self, args):
        r_result = {}
        payments = self.cloudfacade.get_payments_history(args['argument'])
        if payments:
            r_result = payments
        else:
            r_result = {'result': "No payments found"}
        return r_result

    # show help and exit
    def show_help(self, args):
        pass
