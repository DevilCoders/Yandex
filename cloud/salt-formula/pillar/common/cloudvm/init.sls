include:
 - common.cloudvm.billing

secrets:
  api:
    cert: "localhost.crt"
    key: "localhost.key"
  metadata:
    # FIXME: don't enable until CLOUD-18176 is done
    generate_random_key: False

placement:
  dc: virt

push_client:
  enabled: False
  defaults:
    ident: yc-test
    tvm:
      client_id: 2001289

access-service:
  config:
    max_cores: 1
    max_memory: 1G
    master_token: GhgiFgoUZm9vYjAwN2IwMDUwMDAwMDAwMDA=
    master_subject_user_id: foob007b005000000000

oct:
  log_settings:
    defaults:
      sandesh_level: SYS_DEBUG
      pylogger_level: DEBUG

identity:
  default_owner: tuman-cli@yc-test.yaconnect.com

compute-api:
  api_level: staging
  db_level: current
  enable_features: ["new-network-api"]

  default_api_limits:
    max_operations_per_folder: 150

  default_quotas:
    cloud:
      instances: 60
      cores: 50
      memory: 50G
      templates: 100
      snapshots: 50
      total_snapshot_size: 400M
      nbs_disks: 100
      total_disk_size: 5G
      network_hdd_total_disk_size: 5G
      network_ssd_total_disk_size: 5G
      images: 30
      networks: 60
      subnets: 360
      target_groups: 100
      network_load_balancers: 10
      external_addresses: 50
      external_qrator_addresses: 100000
      external_smtp_direct_addresses: 50
      external_static_addresses: 50
      route_tables: 50
      static_routes: 1000

compute-node:
  limit_shared_cores: false
  enable_features: ["new-network-api"]
  allow_software_virtualization: true
  allow_nested_virtualization: true

api-adapter:
  mdb_lb_host: mdb.yc-ci.cloud.yandex.net

api-gateway:
  region: cloudvm
  cluster: cloudvm
  certificate_file: /etc/envoy/ssl/certs/yc-api-gateway.crt
  private_key_file: /etc/envoy/ssl/private/yc-api-gateway.key
  services:
    - { id: yandex.cloud.compute.v1.DiskService, location: REGIONAL }
    - { id: yandex.cloud.compute.v1.DiskTypeService, location: GLOBAL }
    - { id: yandex.cloud.compute.v1.ImageService, location: GLOBAL }
    - { id: yandex.cloud.compute.v1.InstanceService, location: REGIONAL }
    - { id: yandex.cloud.compute.v1.SnapshotService, location: GLOBAL }
    - { id: yandex.cloud.compute.v1.ZoneService, location: GLOBAL }
    - { id: yandex.cloud.vpc.v1.NetworkService, location: GLOBAL }
    - { id: yandex.cloud.vpc.v1.SubnetService, location: REGIONAL }
    - { id: yandex.cloud.iam.v1.IamTokenService, location: GLOBAL }
    - { id: yandex.cloud.iam.v1.awscompatibility.AccessKeyService, location: GLOBAL }
    - { id: yandex.cloud.iam.v1.RoleService, location: GLOBAL }
    - { id: yandex.cloud.iam.v1.ServiceAccountService, location: GLOBAL }
    - { id: yandex.cloud.iam.v1.UserAccountService, location: GLOBAL }
    - { id: yandex.cloud.iam.v1.YandexPassportUserAccountService, location: GLOBAL }
    - { id: yandex.cloud.mdb.clickhouse.v1.BackupService, location: GLOBAL }
    - { id: yandex.cloud.mdb.clickhouse.v1.ClusterService, location: GLOBAL }
    - { id: yandex.cloud.mdb.clickhouse.v1.DatabaseService, location: GLOBAL }
    - { id: yandex.cloud.mdb.clickhouse.v1.ResourcePresetService, location: GLOBAL }
    - { id: yandex.cloud.mdb.clickhouse.v1.UserService, location: GLOBAL }
    - { id: yandex.cloud.mdb.mongodb.v1.BackupService, location: GLOBAL }
    - { id: yandex.cloud.mdb.mongodb.v1.ClusterService, location: GLOBAL }
    - { id: yandex.cloud.mdb.mongodb.v1.DatabaseService, location: GLOBAL }
    - { id: yandex.cloud.mdb.mongodb.v1.ResourcePresetService, location: GLOBAL }
    - { id: yandex.cloud.mdb.mongodb.v1.UserService, location: GLOBAL }
    - { id: yandex.cloud.mdb.postgresql.v1.BackupService, location: GLOBAL }
    - { id: yandex.cloud.mdb.postgresql.v1.ClusterService, location: GLOBAL }
    - { id: yandex.cloud.mdb.postgresql.v1.DatabaseService, location: GLOBAL }
    - { id: yandex.cloud.mdb.postgresql.v1.ResourcePresetService, location: GLOBAL }
    - { id: yandex.cloud.mdb.postgresql.v1.UserService, location: GLOBAL }
    - { id: yandex.cloud.resourcemanager.v1.CloudService, location: GLOBAL }
    - { id: yandex.cloud.resourcemanager.v1.FolderService, location: GLOBAL }
    - { id: yandex.cloud.containerregistry.v1.RegistryService, location: GLOBAL }
    - { id: yandex.cloud.containerregistry.v1.RepositoryService, location: GLOBAL }
    - { id: yandex.cloud.containerregistry.v1.ImageService, location: GLOBAL }
    - { id: yandex.cloud.loadbalancer.v1alpha.NetworkLoadBalancerService, location: REGIONAL }
    - { id: yandex.cloud.loadbalancer.v1alpha.TargetGroupService, location: REGIONAL }
  discovery:
    - { type: COMPUTE, address: "127.0.0.1:4433", location_type: GLOBAL, location: global, id_prefix: dcp }
    - { type: COMPUTE, address: "127.0.0.1:4433", location_type: REGIONAL, location: ru-central1, id_prefix: cn8 }
    - { type: COMPUTE, address: "127.0.0.1:4433", location_type: ZONAL, location: ru-central1-a, id_prefix: cbq }
    - { type: VPC, address: "127.0.0.1:4433", location_type: GLOBAL, location: global, id_prefix: ehj }
    - { type: VPC, address: "127.0.0.1:4433", location_type: REGIONAL, location: ru-central1, id_prefix: a5n }
    - { type: VPC, address: "127.0.0.1:4433", location_type: ZONAL, location: ru-central1-a, id_prefix: fru }
    - { type: IAM, address: "127.0.0.1:4433", location_type: GLOBAL, location: global, id_prefix: d77 }
    - { type: RESOURCE_MANAGER, address: "127.0.0.1:4433", location_type: GLOBAL, location: global, id_prefix: cro }
    - { type: MDB, address: "127.0.0.1:4433", location_type: GLOBAL, location: global, id_prefix: agf }
    - { type: MDB_POSTGRESQL, address: "127.0.0.1:4433", location_type: GLOBAL, location: global, id_prefix: agf }
    - { type: MDB_CLICKHOUSE, address: "127.0.0.1:4433", location_type: GLOBAL, location: global, id_prefix: agf }
    - { type: MDB_MONGODB, address: "127.0.0.1:4433", location_type: GLOBAL, location: global, id_prefix: agf }
    - { type: CONTAINER_REGISTRY, address: "127.0.0.1:4438", location_type: GLOBAL, location: global, id_prefix: cre }
    - { type: LOAD_BALANCER, address: "127.0.0.1:4433", location_type: GLOBAL, location: global, id_prefix: epc }
    - { type: LOAD_BALANCER, address: "127.0.0.1:4433", location_type: REGIONAL, location: ru-central1, id_prefix: cbq }
    - { type: LOAD_BALANCER, address: "127.0.0.1:4433", location_type: ZONAL, location: ru-central1-a, id_prefix: d11 }
    - { type: LOAD_BALANCER, address: "127.0.0.1:4433", location_type: ZONAL, location: ru-central1-b, id_prefix: fv8 }
    - { type: LOAD_BALANCER, address: "127.0.0.1:4433", location_type: ZONAL, location: ru-central1-c, id_prefix: el2 }
  endpoints:
    - { name: endpoint, host: localhost }
    - { name: compute, host: compute.cloudvm.local }
    - { name: vpc, host: vpc.cloudvm.local }
    - { name: iam, host: iam.cloudvm.local }
    - { name: resourcemanager, host: resourcemanager.cloudvm.local }
    - { name: operation, host: operation.cloudvm.local }
    - { name: mdb-postgresql, host: mdb.cloudvm.local }
    - { name: mdb-clickhouse, host: mdb.cloudvm.local }
    - { name: mdb-mongodb, host: mdb.cloudvm.local }
    - { name: managed-postgresql, host: mdb.cloudvm.local }
    - { name: managed-clickhouse, host: mdb.cloudvm.local }
    - { name: managed-mongodb, host: mdb.cloudvm.local }
    - { name: container-registry, host: container-registry.cloudvm.local }
    - { name: load-balancer, host: lb.cloudvm.local }

  container_registry:
    host: container-registry.private-api.ycp.cloudvm.local
    port: 443

microcosm:
  features:
    sync_resource_statuses: false


load-balancer:
  hc-ctrl:
    endpoint: local-lb.cloud-lab.yandex.net:4051
  lb-ctrl:
    endpoint: local-lb.cloud-lab.yandex.net:4051

kikimr_prefix: "dev_"
