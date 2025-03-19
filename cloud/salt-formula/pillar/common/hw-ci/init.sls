include:
 - common.hw-ci.billing

secrets:
  api:
    cert: "api.crt"
    key: "api.key"
  serialws:
    cert: "serialws.crt"
    key: "serialws.key"
  metadata:
    # FIXME: don't enable until CLOUD-18176 is done
    generate_random_key: False
  sqs:
    cert: "queues.api.cloud-df.yandex.net.pem"
    key: "queues.api.cloud-df.yandex.net.key.pem"

push_client:
  enabled: True
  defaults:
    ident: yc-test
    tvm:
      client_id: 2001289
  instances:
    access-service:
      ident: yc-access-service
      proto: rt
      # unit topic be created disable this one
      enabled: False
    resource-manager:
      ident: yc-resource-manager
      proto: rt
      # unit topic be created disable this one
      enabled: False

compute-api:
  api_level: staging
  db_level: current
  enable_features: ["new-network-api"]
  default_quotas:
    network_load_balancers: 5

compute-node:
  limit_shared_cores: false
  enable_features: ["new-network-api"]
  allow_software_virtualization: true
  allow_nested_virtualization: true
  default_quotas:
    project:
      instances: 100
      templates: 100
      snapshots: 50
      nbs_disks: 100
      total_disk_size: 10000G
  deallocation:
    delay: 10

identity:
  blackbox:
    - passport_url: "http://blackbox-mimino.yandex.net/blackbox"
      tvm_client_id: 239
      mds_url: "https://avatars.mds.yandex.net"
      sessionid_cookie_host: "yandex.ru"

    - passport_url: "https://pass-test.yandex.ru/blackbox"
      tvm_client_id: 224
      mds_url: "https://avatars.mdst.yandex.net"
      sessionid_cookie_host: "yandex.ru"
  tvm:
    client_id: 2000667
    blackbox_instance: "Mimino"

access-service:
  config:
    max_cores: 2
    max_memory: 2G
  push-client:
    enabled: True

resource-manager:
  config:
    max_memory: 500M
  push-client:
    enabled: True

api-adapter:
  mdb_lb_host: mdb.yc-ci.cloud.yandex.net
  push-client:
    enabled: True

api-gateway:
  push_client:
    enabled: True

  region: man
  cluster: hw-ci
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
    - { id: yandex.cloud.vpc.v1.RouteTableService, location: GLOBAL }
    - { id: yandex.cloud.vpc.v1.SubnetService, location: REGIONAL }
    - { id: yandex.cloud.iam.v1.IamTokenService, location: GLOBAL }
    - { id: yandex.cloud.iam.v1.awscompatibility.AccessKeyService, location: GLOBAL }
    - { id: yandex.cloud.iam.v1.KeyService, location: GLOBAL }
    - { id: yandex.cloud.iam.v1.RoleService, location: GLOBAL }
    - { id: yandex.cloud.iam.v1.ServiceAccountService, location: GLOBAL }
    - { id: yandex.cloud.iam.v1.UserAccountService, location: GLOBAL }
    - { id: yandex.cloud.iam.v1.YandexPassportUserAccountService, location: GLOBAL }
    - { id: yandex.cloud.iam.v1.ApiKeyService, location: GLOBAL }
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
    - { id: yandex.cloud.mdb.mysql.v1alpha.BackupService, location: GLOBAL }
    - { id: yandex.cloud.mdb.mysql.v1alpha.ClusterService, location: GLOBAL }
    - { id: yandex.cloud.mdb.mysql.v1alpha.DatabaseService, location: GLOBAL }
    - { id: yandex.cloud.mdb.mysql.v1alpha.ResourcePresetService, location: GLOBAL }
    - { id: yandex.cloud.mdb.mysql.v1alpha.UserService, location: GLOBAL }
    - { id: yandex.cloud.mdb.postgresql.v1.BackupService, location: GLOBAL }
    - { id: yandex.cloud.mdb.postgresql.v1.ClusterService, location: GLOBAL }
    - { id: yandex.cloud.mdb.postgresql.v1.DatabaseService, location: GLOBAL }
    - { id: yandex.cloud.mdb.postgresql.v1.ResourcePresetService, location: GLOBAL }
    - { id: yandex.cloud.mdb.postgresql.v1.UserService, location: GLOBAL }
    - { id: yandex.cloud.mdb.redis.v1alpha.BackupService, location: GLOBAL }
    - { id: yandex.cloud.mdb.redis.v1alpha.ClusterService, location: GLOBAL }
    - { id: yandex.cloud.mdb.redis.v1alpha.ResourcePresetService, location: GLOBAL }
    - { id: yandex.cloud.resourcemanager.v1.CloudService, location: GLOBAL }
    - { id: yandex.cloud.resourcemanager.v1.FolderService, location: GLOBAL }
    - { id: yandex.cloud.containerregistry.v1.RegistryService, location: GLOBAL }
    - { id: yandex.cloud.containerregistry.v1.RepositoryService, location: GLOBAL }
    - { id: yandex.cloud.containerregistry.v1.ImageService, location: GLOBAL }
    - { id: yandex.cloud.loadbalancer.v1alpha.NetworkLoadBalancerService, location: REGIONAL }
    - { id: yandex.cloud.loadbalancer.v1alpha.TargetGroupService, location: REGIONAL }
    - { id: yandex.cloud.serverless.functions.v1alpha.FunctionService, location: GLOBAL }
    - { id: yandex.cloud.k8s.v1.ClusterService, location: REGIONAL }
    - { id: yandex.cloud.k8s.v1.NodeGroupService, location: REGIONAL }
  discovery:
    - { type: COMPUTE, address: "127.0.0.1:4433", location_type: GLOBAL, location: global, id_prefix: fpj }
    - { type: COMPUTE, address: "127.0.0.1:4433", location_type: REGIONAL, location: ru-central1, id_prefix: c2v }
    - { type: COMPUTE, address: "127.0.0.1:4433", location_type: ZONAL, location: ru-central1-a, id_prefix: ca3 }
    - { type: COMPUTE, address: "127.0.0.1:4433", location_type: ZONAL, location: ru-central1-b, id_prefix: a2v }
    - { type: COMPUTE, address: "127.0.0.1:4433", location_type: ZONAL, location: ru-central1-c, id_prefix: cmh }
    - { type: VPC, address: "127.0.0.1:4433", location_type: GLOBAL, location: global, id_prefix: cpa }
    - { type: VPC, address: "127.0.0.1:4433", location_type: REGIONAL, location: ru-central1, id_prefix: feo }
    - { type: VPC, address: "127.0.0.1:4433", location_type: ZONAL, location: ru-central1-a, id_prefix: al7 }
    - { type: VPC, address: "127.0.0.1:4433", location_type: ZONAL, location: ru-central1-b, id_prefix: fg4 }
    - { type: VPC, address: "127.0.0.1:4433", location_type: ZONAL, location: ru-central1-c, id_prefix: csh }
    - { type: IAM, address: "127.0.0.1:4433", location_type: GLOBAL, location: global, id_prefix: fp2 }
    - { type: RESOURCE_MANAGER, address: "127.0.0.1:4433", location_type: GLOBAL, location: global, id_prefix: a5b }
    - { type: MDB, address: "127.0.0.1:4433", location_type: GLOBAL, location: global, id_prefix: d8e }
    - { type: MDB_POSTGRESQL, address: "127.0.0.1:4433", location_type: GLOBAL, location: global, id_prefix: d8e }
    - { type: MDB_CLICKHOUSE, address: "127.0.0.1:4433", location_type: GLOBAL, location: global, id_prefix: d8e }
    - { type: MDB_MONGODB, address: "127.0.0.1:4433", location_type: GLOBAL, location: global, id_prefix: d8e }
    - { type: MDB_MYSQL, address: "127.0.0.1:4433", location_type: GLOBAL, location: global, id_prefix: d8e }
    - { type: MDB_REDIS, address: "127.0.0.1:4433", location_type: GLOBAL, location: global, id_prefix: d8e }
    - { type: CONTAINER_REGISTRY, address: "127.0.0.1:4438", location_type: GLOBAL, location: global, id_prefix: cri }
    - { type: LOAD_BALANCER, address: "127.0.0.1:4433", location_type: GLOBAL, location: global, id_prefix: dg8 }
    - { type: LOAD_BALANCER, address: "127.0.0.1:4433", location_type: REGIONAL, location: ru-central1, id_prefix: e9p }
    - { type: LOAD_BALANCER, address: "127.0.0.1:4433", location_type: ZONAL, location: ru-central1-a, id_prefix: fco }
    - { type: LOAD_BALANCER, address: "127.0.0.1:4433", location_type: ZONAL, location: ru-central1-b, id_prefix: bm5 }
    - { type: LOAD_BALANCER, address: "127.0.0.1:4433", location_type: ZONAL, location: ru-central1-c, id_prefix: fb8 }
    - { type: KUBERNETES, address: "127.0.0.1:4450", location_type: GLOBAL, location: global, id_prefix: ebk }
    - { type: KUBERNETES, address: "127.0.0.1:4450", location_type: REGIONAL, location: ru-central1, id_prefix: c49 }
  endpoints:
    - { name: endpoint, host: api.cloud-hwci.yandex.net }
    - { name: compute, host: compute.api.cloud-hwci.yandex.net }
    - { name: vpc, host: vpc.api.cloud-hwci.yandex.net }
    - { name: iam, host: iam.api.cloud-hwci.yandex.net }
    - { name: resourcemanager, host: resource-manager.api.cloud-hwci.yandex.net }
    - { name: resource-manager, host: resource-manager.api.cloud-hwci.yandex.net }
    - { name: operation, host: operation.api.cloud-hwci.yandex.net }
    - { name: mdb-postgresql, host: mdb.api.cloud-hwci.yandex.net }
    - { name: mdb-clickhouse, host: mdb.api.cloud-hwci.yandex.net }
    - { name: mdb-mongodb, host: mdb.api.cloud-hwci.yandex.net }
    - { name: mdb-mysql, host: mdb.api.cloud-hwci.yandex.net }
    - { name: mdb-redis, host: mdb.api.cloud-hwci.yandex.net }
    - { name: managed-postgresql, host: mdb.api.cloud-hwci.yandex.net }
    - { name: managed-clickhouse, host: mdb.api.cloud-hwci.yandex.net }
    - { name: managed-mongodb, host: mdb.api.cloud-hwci.yandex.net }
    - { name: managed-mysql, host: mdb.api.cloud-hwci.yandex.net }
    - { name: managed-redis, host: mdb.api.cloud-hwci.yandex.net }
    - { name: container-registry, host: container-registry.api.cloud-hwci.yandex.net }
    - { name: load-balancer, host: load-balancer.api.cloud-hwci.yandex.net }
    - { name: serverless-functions, host: serverless-functions.api.cloud-hwci.yandex.net }
    - { name: k8s, host: managed-kubernetes.api.cloud-hwci.yandex.net }

  container_registry:
    host: container-registry.private-api.ycp.cloud-hwci.yandex.net
    port: 443

  serverless_functions:
    host: serverless-functions.private-api.ycp.cloud-hwci.yandex.net
    port: 443

  managed_kubernetes:
    host: mk8s.private-api.ycp.cloud-hwci.yandex.net
    port: 443

microcosm:
  features:
    sync_resource_statuses: false

solomon-agent:
  prometheus-plugin:
    access-service:
      access_service:
        url: http://localhost:9990/metrics
        pull-interval: 15s
    api-gateway:
      api_envoy:
        url: http://localhost:9102/metrics
        pull-interval: 15s
      api_gateway:
        url: http://localhost:9998/metrics
        pull-interval: 15s
      api_als:
        url: http://localhost:4437/metrics
        pull-interval: 15s
    api-adapter:
      api_adapter:
        url: http://localhost:4440/metrics
        pull-interval: 15s
    microcosm:
      microcosm:
        url: http://localhost:8765/metrics
        pull-interval: 15s
    resource-manager:
      resource_manager:
        url: http://localhost:7380/metrics
        pull-interval: 15s

placement:
  dc: man

kikimr_secrets:
  cfg_dir: /etc/kikimr_secrets
  secrets_dir: /var/lib/kikimr_secrets
  disk:
    cfg_dir: /etc/kikimr_secrets
    dm_name: secrets.kikimr
    device_serial: INTERNAL_DATA
  tenants_keys:
    billing: 71cf33a7-9759-07d1-7bcb-ee5f97bf6fbd
    iam: a4eac1f1-c2a8-49ac-3023-459aa6d598bf
    loadbalancer: 31d6a6d6-d8ac-6bb0-7090-c88d20cb9738
    microcosm: e400bd26-d122-954d-e3e2-59c0262ee68a
    mkt: ab857bb5-ba65-3c93-2082-5aaa1dd57e4a
    s3: 6f19e68b-7131-122a-ac2e-b975f5f6429d
    solomon: 702a9926-f451-af65-81df-82b49a42341c
    snapshot: 0e6ee1d6-e005-01fc-c31e-bb1d96c2cb0a
    ycloud: ac3c83da-85ce-f59d-b573-b7ccd7183c63
    ycr: df2dcfee-3e7a-a034-a03c-631622618e7f
    SQS: 74a1fb86-8db2-dff3-75b8-474fbf6e1bf1

load-balancer:
  hc-ctrl:
    endpoint: hc.private-api.cloud-hwci.yandex.net:4051
  lb-ctrl:
    endpoint: lb.private-api.cloud-hwci.yandex.net:4051

kikimr_prefix: "hw-ci_"
