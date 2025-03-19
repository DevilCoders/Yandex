from cloud.mdb.dbaas_worker.internal.providers.load_balancer import LoadBalancer
from cloud.mdb.internal.python.loadbalancer import (
    AttachedTargetGroup,
    HealthCheck,
    HttpOptions,
    InternalAddressSpec,
    IpVersion,
    ListenerSpec,
    Protocol,
    Target,
    Type,
)

from cloud.mdb.dbaas_worker.internal.providers.managed_kubernetes import ManagedKubernetes
from cloud.mdb.dbaas_worker.internal.providers.managed_postgresql import ManagedPostgresql, DatabaseSpec, UserSpec
from cloud.mdb.internal.python.managed_kubernetes import (
    ScalePolicy,
    FixedScale,
    NetworkSettings,
    ContainerRuntimeSettings,
    NetworkSettingsType,
    ContainerRuntimeSettingsType,
    NodeTemplate,
    MasterSpec,
    ZonalMasterSpec,
    NetworkInterfaceSpec,
    NodeAddressSpec,
)
from ...common.cluster.create import ClusterCreateExecutor
from ...utils import register_executor
from ....crypto import decrypt

from .utils import (
    get_xml_config,
    get_security_group_ids,
    get_region_id,
    get_network_load_balancer_name,
    get_target_group_name,
)


@register_executor('metastore_cluster_create')
class MetastoreClusterCreate(ClusterCreateExecutor):
    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.managed_kubernetes = ManagedKubernetes(self.config, self.task, self.queue)
        self.managed_postgresql = ManagedPostgresql(self.config, self.task, self.queue)
        self.network_load_balancer = LoadBalancer(self.config, self.task, self.queue)

    def run(self):
        self.acquire_lock()
        cid = self.task['cid']
        subcluster_id = self.args['subcid']

        pillar = self.pillar.get('cid', self.task['cid'], ['data'])

        username = pillar['db_user_name']
        password = decrypt(self.config, pillar['db_user_password'])
        postgresql_cluster_id = pillar['postgresql_cluster_id']
        db_name = pillar['db_name']

        self.managed_postgresql.user_exists(
            cluster_id=postgresql_cluster_id,
            user_spec=UserSpec(
                name=username,
                password=password,
                conn_limit=50,
            ),
        )

        self.managed_postgresql.database_exists(
            cluster_id=postgresql_cluster_id, database_spec=DatabaseSpec(name=db_name, owner=username)
        )

        if 'kubernetes_cluster_id' not in pillar:
            zone_ids = pillar['zone_ids']
            # TODO support regional master
            master_spec = MasterSpec(zonal_master=ZonalMasterSpec(zone_id=zone_ids[0]))
            operation_id, kubernetes_cluster_id = self.managed_kubernetes.cluster_exists(
                folder_id=self.task['folder_id'],
                subcluster_id=subcluster_id,
                network_id=pillar['network_id'],
                service_account_id=pillar['service_account_id'],
                node_service_account_id=pillar['node_service_account_id'],
                master_spec=master_spec,
            )
            self.logger.info(f'operation_id = {operation_id}, kubernetes_cluster_id = {kubernetes_cluster_id}')
        else:
            kubernetes_cluster_id = pillar['kubernetes_cluster_id']

        operation_id, node_group_id = self.managed_kubernetes.node_group_exists(
            name=f'node-group-{cid}',
            subcluster_id=subcluster_id,
            kubernetes_cluster_id=kubernetes_cluster_id,
            node_template=NodeTemplate(
                network_settings=NetworkSettings(type=NetworkSettingsType.STANDARD),
                container_runtime_settings=ContainerRuntimeSettings(type=ContainerRuntimeSettingsType.DOCKER),
                network_interface_specs=[
                    NetworkInterfaceSpec(
                        primary_v4_address_spec=NodeAddressSpec(
                            one_to_one_nat_spec=None,
                        ),
                        primary_v6_address_spec=NodeAddressSpec(one_to_one_nat_spec=None),
                        security_group_ids=get_security_group_ids(),
                        subnet_ids=pillar['service_subnet_ids'],
                    ),
                    NetworkInterfaceSpec(
                        primary_v4_address_spec=NodeAddressSpec(
                            one_to_one_nat_spec=None,
                        ),
                        security_group_ids=[],
                        subnet_ids=pillar['user_subnet_ids'],
                    ),
                ],
            ),
            scale_policy=ScalePolicy(
                fixed_scale=FixedScale(size=pillar['min_servers_per_zone']),
            ),
        )
        self.logger.info(f'operation_id = {operation_id}, node_group_id = {node_group_id}')

        namespace_name = pillar['kubernetes_namespace_name']

        self.managed_kubernetes.initialize_kubernetes_master(kubernetes_cluster_id, namespace=namespace_name)

        self.managed_kubernetes.kubernetes_master.namespace_exists()

        self.managed_kubernetes.kubernetes_master.secret_exists(
            name='metastore-secret',
            entries={
                'username': username,
                'password': password,
            },
        )

        postgres_cluster_hostname = pillar['postgresql_hostname']

        self.managed_kubernetes.kubernetes_master.configmap_exists(
            name='metastore-site-configmap',
            entries={
                'metastore-site.xml': get_xml_config(postgres_cluster_hostname, db_name),
                'db_hostname': postgres_cluster_hostname,
                'db_name': db_name,
            },
        )
        metastore_init_job = self.managed_kubernetes.kubernetes_master.parse_resource(
            'resources/metastore/metastore-db-init-job.yaml'
        )
        metastore_init_job['spec']['template']['spec']['nodeSelector'] = {'yandex.cloud/node-group-id': node_group_id}
        self.managed_kubernetes.kubernetes_master.apply_config_from_dict(metastore_init_job)
        result = self.managed_kubernetes.kubernetes_master.wait_for_job(
            job_name='metastore-db-init',
            timeout_seconds=180,
        )
        if result:
            metastore_deployment = self.managed_kubernetes.kubernetes_master.parse_resource(
                'resources/metastore/metastore-server.yaml'
            )
            metastore_deployment['spec']['template']['spec']['nodeSelector'] = {
                'yandex.cloud/node-group-id': node_group_id
            }
            self.managed_kubernetes.kubernetes_master.apply_config_from_dict(metastore_deployment)
            self.managed_kubernetes.kubernetes_master.apply_configs(
                resource_paths=('resources/metastore/metastore-service.yaml',),
            )

            nodes = self.managed_kubernetes.client.list_nodes(node_group_id=node_group_id)
            targets = []
            for node in nodes:
                instance_id = node.cloud_status.id
                instance = self.compute_api.get_instance(fqdn=None, instance_id=instance_id)
                user_vpc_interface = instance.network_interfaces[1]
                targets.append(
                    Target(
                        subnet_id=user_vpc_interface.subnet_id,
                        address=user_vpc_interface.primary_v4_address.address,
                    )
                )

            region_id = get_region_id()
            subnet_id = pillar['user_subnet_ids'][0]
            kubernetes_cluster = self.managed_kubernetes.client.get_cluster(cluster_id=kubernetes_cluster_id)
            node_port = self.managed_kubernetes.kubernetes_master.get_node_port(service_name='metastore-service')

            _, target_group_id = self.network_load_balancer.client.create_target_group(
                name=get_target_group_name(cid),
                folder_id=kubernetes_cluster.folder_id,
                region_id=region_id,
                targets=targets,
                wait=True,
            )

            _, network_load_balancer_id = self.network_load_balancer.client.create_network_load_balancer(
                folder_id=kubernetes_cluster.folder_id,
                region_id=region_id,
                type=Type.INTERNAL,
                name=get_network_load_balancer_name(cid),
                listener_specs=[
                    ListenerSpec(
                        name='thrift',
                        port=9083,
                        target_port=node_port,
                        protocol=Protocol.TCP,
                        internal_address_spec=InternalAddressSpec(
                            subnet_id=subnet_id,
                            ip_version=IpVersion.IPV4,
                        ),
                    ),
                ],
                attached_target_groups=[
                    AttachedTargetGroup(
                        target_group_id=target_group_id,
                        health_checks=[
                            HealthCheck(
                                name='default',
                                unhealthy_threshold=2,
                                healthy_threshold=2,
                                options=HttpOptions(
                                    port=10256,
                                    path='/healthz',
                                ),
                            ),
                        ],
                    ),
                ],
                wait=True,
            )
            network_load_balancer = self.network_load_balancer.client.get_network_load_balancer(
                network_load_balancer_id=network_load_balancer_id,
            )
            endpoint_ip = network_load_balancer.listeners[0].address
            self.pillar.exists('cid', self.task['cid'], ['data'], ['endpoint_ip'], [endpoint_ip])
            self.logger.info(f'endpoint_ip = {endpoint_ip}')
        else:
            raise Exception('Init job is failed')

        self.release_lock()
