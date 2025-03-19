"""
Module with main bootstrap class
"""
import collections
import logging
import operator

from .worker import ComputeApi, ConductorApi


class Bootstrap:
    """
    Main bootstrap class
    """
    DEPLOYS = dict()

    def __init__(self, config, worker_config):
        self.config = config
        self.worker_config = worker_config
        self.compute = ComputeApi(worker_config)
        self.conductor = ConductorApi(worker_config)
        self.operations = collections.defaultdict(list)
        self.log = self._init_logging()

    @classmethod
    def deploy_handler(cls, typ):
        """
        Decorator to register deploy handler in `self.DEPLOYS`
        """
        def wrapper(handler):
            """
            Adds handler to deploy dict
            """
            cls.DEPLOYS[typ] = handler
            return handler

        return wrapper

    def get_deploy_handler(self, typ):
        """
        Returns deploy handler for type
        """
        if typ not in self.DEPLOYS:
            raise RuntimeError('I don\'t know how to deploy type {type}'.format(type=typ))
        return self.DEPLOYS[typ]

    @classmethod
    def _init_logging(cls):
        logger = logging.getLogger(str(cls))
        logger.setLevel(logging.DEBUG)
        return logger

    def group_config(self, group):
        """
        Returns config for given group
        """
        return self.config.configuration.__dict__[group]

    def get_hosts(self, group=None):
        """
        Returns dict with hosts of given group or all
        """
        hosts = self.config.configuration.__dict__
        if group is None:
            return hosts
        return hosts[group]

    def get_hosts_fqdn(self, group):
        """
        Returns fqdn of all hosts in specified group
        """
        hosts = map(operator.itemgetter('fqdn'), self.get_hosts(group)['hosts'])
        return list(hosts)

    # pylint: disable=protected-access
    def create_hosts(self, group=None):
        """
        Creates all needed hosts and fill self.operations with
        corresponding operations which need to be polled.
        """
        # Create parent conductor group if need
        parent_group = self.config.conductor_group
        for host_type, config in self.get_hosts().items():
            if group is not None and host_type != group:
                continue
            cond_group = config['conductor_group']
            self.conductor.group_exists(cond_group, parent_group)
            self.log.info('Creating virtual machines for group "%s"', host_type)
            disk_size = getattr(config, 'data_size', None)
            for host in config['hosts']:
                self.conductor.host_exists(host['fqdn'], host['geo'], cond_group)
                self.log.info('Creating "%s"', host['fqdn'])
                interfaces = []
                for network in config['networks']:
                    if isinstance(network, str):
                        interfaces.append({
                            'primary_v6_address_spec': {},
                            'subnetId':
                                self.compute._get_geo_subnet(
                                    self.worker_config.compute.folder_id, network,
                                    self.worker_config.compute.geo_map.get(host['geo'], host['geo']))['id'],
                        })
                    elif isinstance(network, dict):
                        interfaces.append({
                            'primary_v4_address_spec': {},
                            'subnetId':
                                self.compute._get_geo_subnet(
                                    network['folder_id'], network['network_id'],
                                    self.worker_config.compute.geo_map.get(host['geo'], host['geo']))['id'],
                        })
                args = {'networkInterfaceSpecs': interfaces}
                operation_id = self.compute.instance_exists(
                    geo=host['geo'],
                    fqdn=host['fqdn'],
                    image_type=config['image_type'],
                    cores=config['cores'],
                    core_fraction=config.get('core_fraction', 100),
                    memory=config['memory'],
                    platform_id=config['platform_id'],
                    subnet_id=interfaces[0]['subnetId'],
                    disk_type_id=config['disk_type_id'],
                    disk_size=disk_size,
                    root_disk_size=config['root_size'],
                    assign_public_ip=False,
                    **args)
                if operation_id:
                    self.operations[host_type].append(operation_id)

        for host_type in self.get_hosts():
            self.wait_host_group(host_type)

    def wait_host_group(self, group):
        """
        Wait while given host group will be created and ready to deploying
        """
        logging.info('Waiting operations in group "%s"', group)
        for operation_id in self.operations[group]:
            self.compute.operation_wait(operation_id)
        logging.info('Waiting hosts in group "%s"', group)
        for host in self.get_hosts_fqdn(group):
            logging.info('Waiting host "%s" to be running', host)
            self.compute.instance_wait(host, 'RUNNING')

    def deploy(self):
        """
        Start bootstrap deploy
        """
        self.create_hosts()
        for group in [
                'salt',
                'zk',
                'metadb',
                'mdb-health',
                'api',
                'api-admin',
                'worker',
                'mdb-internal-api',
                'report',
                'e2e',
                'mdb-dns',
                'deploy-api',
                'deploy-db',
                'deploy-salt',
                'dataproc-manager',
                'mdb-secrets-db',
                'mdb-secrets-api',
        ]:
            group_config = self.group_config(group)
            handler = self.get_deploy_handler(group_config['deploy_type'])(group_config, self.config)
            handler.deploy(self.compute)

    def drop(self, group=None):
        """
        Cleanup installation
        """
        operations = []
        for host_type, config in self.get_hosts().items():
            if group is not None and host_type != group:
                continue
            for host in config['hosts']:
                self.conductor.host_absent(host['fqdn'])
                self.log.info('Dropping instance "%s"', host['fqdn'])
                operation_id = self.compute.instance_absent(host['fqdn'])
                operations.append((host['fqdn'], operation_id))
        logging.info('Waiting operations')
        for host, operation_id in operations:
            self.compute.operation_wait(operation_id)
            self.compute.instance_wait(host, 'DELETED')
