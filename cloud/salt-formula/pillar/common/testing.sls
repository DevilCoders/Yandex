secrets:
  api:
    cert: "api.crt"
    key: "api.key"
  serialws:
    cert: "serialws.crt"
    key: "serialws.key"
  sqs:
    cert: "queues.api.cloud-df.yandex.net.pem"
    key: "queues.api.cloud-df.yandex.net.key.pem"

push_client:
  enabled: True
  defaults:
    # FIXME(CLOUD-17254): Rework billing pillar for testing cluster
    ident: yc-df-pre
    tvm:
      client_id: 2001289

compute-node:
  limit_shared_cores: false
  enable_features: ["new-network-api"]
  allow_software_virtualization: true
  allow_nested_virtualization: true

placement:
  dc: man

oct:
  log_settings:
    defaults:
      sandesh_level: SYS_DEBUG
      pylogger_level: DEBUG

# FIXME(CLOUD-17254): Rework billing pillar for testing cluster
billing:
  solomon:
    cluster: predf
  id_prefix: bu2
  s3:
    private_api: https://storage-idm.private-api.cloud-preprod.yandex.net:1443
    url: https://storage.cloud-preprod.yandex.net
    endpoint_url: https://storage.cloud-preprod.yandex.net
    reports_bucket: reports-testing
  api:
    auth_enabled: True
    tvm_enabled: False
  uploader:
    enabled: True
    parallel: True
    source:
      logbroker:
        topics:
          - name: yc-df-pre--billing-compute-instance
          - name: yc-df-pre--billing-compute-snapshot
          - name: yc-df-pre--billing-compute-image
          - name: yc-df-pre--billing-object-storage
          - name: yc-df-pre--billing-object-requests
          - name: yc-df-pre--billing-nbs-volume
          - name: yc-df-pre--billing-sdn-traffic
          - name: yc-df-pre--billing-sdn-fip
          - name: yc-df-pre--billing-ai-requests
  engine:
    logbroker:
      topics:
        - rt3.vla--yc-df-pre--billing-compute-instance
        - rt3.vla--yc-df-pre--billing-compute-snapshot
        - rt3.vla--yc-df-pre--billing-compute-image
        - rt3.vla--yc-df-pre--billing-object-storage
        - rt3.vla--yc-df-pre--billing-object-requests
        - rt3.vla--yc-df-pre--billing-nbs-volume
        - rt3.vla--yc-df-pre--billing-sdn-traffic
        - rt3.vla--yc-df-pre--billing-sdn-fip
        - rt3.vla--yc-df-pre--billing-ai-requests

compute-api:
  api_level: staging
  db_level: current
  enable_features: ["new-network-api"]

  default_api_limits:
    max_operations_per_folder: 1000

  default_quotas:
    cloud:
      instances: 1000
      cores: 1000
      memory: 1000G
      templates: 1000
      snapshots: 1000
      total_snapshot_size: 1000G
      nbs_disks: 1000
      total_disk_size: 1000G
      images: 1000
      networks: 1000
      subnets: 1000
      target_groups: 100
      network_load_balancers: 5
      external_addresses: 1000
      external_qrator_addresses: 100000
      external_smtp_direct_addresses: 1000
      external_static_addresses: 1000
      route_tables: 1000
      static_routes: 10000

identity:
  monitoring_endpoint:
    public: http://[::1]:4336/public/v1alpha1/health?monrun=1
    private: http://[::1]:2637/private/v1alpha1/health?monrun=1
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
  push:
    url: 'https://console-testing.cloud.yandex.net'
  system_accounts:
    public_keys_file: "testing.json"

access-service:
  config:
    max_cores: 2
    max_memory: 2G
  push-client:
    enabled: True

resource-manager:
  push-client:
    enabled: True

iam-takeout-agent:
  tvm:
    client_id: 2011552
    blackbox_instance: "Mimino"
    allowed_clients: [2009783, 2011552]

resource-manager-takeout-agent:
  tvm:
    client_id: 2011556
    blackbox_instance: "Mimino"
    allowed_clients: [2009783, 2011552]

api-adapter:
  mdb_lb_host: mdb.cloud-testing.yandex.net

api-gateway:
  region: man
  cluster: testing
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
    - { id: yandex.cloud.loadbalancer.v1alpha.NetworkLoadBalancerService, location: REGIONAL }
    - { id: yandex.cloud.loadbalancer.v1alpha.TargetGroupService, location: REGIONAL }
    - { id: yandex.cloud.k8s.v1.ClusterService, location: REGIONAL }
    - { id: yandex.cloud.k8s.v1.NodeGroupService, location: REGIONAL }
  discovery:
    - { type: COMPUTE, address: "127.0.0.1:4433", location_type: GLOBAL, location: global, id_prefix: c2p }
    - { type: COMPUTE, address: "127.0.0.1:4433", location_type: REGIONAL, location: ru-central1, id_prefix: dfs }
    - { type: COMPUTE, address: "127.0.0.1:4433", location_type: ZONAL, location: ru-central1-a, id_prefix: fsa }
    - { type: COMPUTE, address: "127.0.0.1:4433", location_type: ZONAL, location: ru-central1-b, id_prefix: all }
    - { type: COMPUTE, address: "127.0.0.1:4433", location_type: ZONAL, location: ru-central1-c, id_prefix: bl8 }
    - { type: VPC, address: "127.0.0.1:4433", location_type: GLOBAL, location: global, id_prefix: a19 }
    - { type: VPC, address: "127.0.0.1:4433", location_type: REGIONAL, location: ru-central1, id_prefix: b37 }
    - { type: VPC, address: "127.0.0.1:4433", location_type: ZONAL, location: ru-central1-a, id_prefix: ema }
    - { type: VPC, address: "127.0.0.1:4433", location_type: ZONAL, location: ru-central1-b, id_prefix: fkp }
    - { type: VPC, address: "127.0.0.1:4433", location_type: ZONAL, location: ru-central1-c, id_prefix: flq }
    - { type: IAM, address: "127.0.0.1:4433", location_type: GLOBAL, location: global, id_prefix: d26 }
    - { type: RESOURCE_MANAGER, address: "127.0.0.1:4433", location_type: GLOBAL, location: global, id_prefix: bat }
    - { type: MDB, address: "127.0.0.1:4433", location_type: GLOBAL, location: global, id_prefix: f8h }
    - { type: MDB_POSTGRESQL, address: "127.0.0.1:4433", location_type: GLOBAL, location: global, id_prefix: f8h }
    - { type: MDB_CLICKHOUSE, address: "127.0.0.1:4433", location_type: GLOBAL, location: global, id_prefix: f8h }
    - { type: MDB_MONGODB, address: "127.0.0.1:4433", location_type: GLOBAL, location: global, id_prefix: f8h }
    - { type: MDB_MYSQL, address: "127.0.0.1:4433", location_type: GLOBAL, location: global, id_prefix: f8h }
    - { type: MDB_REDIS, address: "127.0.0.1:4433", location_type: GLOBAL, location: global, id_prefix: f8h }
    - { type: LOAD_BALANCER, address: "127.0.0.1:4433", location_type: GLOBAL, location: global, id_prefix: etq }
    - { type: LOAD_BALANCER, address: "127.0.0.1:4433", location_type: REGIONAL, location: ru-central1, id_prefix: eeh }
    - { type: LOAD_BALANCER, address: "127.0.0.1:4433", location_type: ZONAL, location: ru-central1-a, id_prefix: c0i }
    - { type: LOAD_BALANCER, address: "127.0.0.1:4433", location_type: ZONAL, location: ru-central1-b, id_prefix: b2v }
    - { type: LOAD_BALANCER, address: "127.0.0.1:4433", location_type: ZONAL, location: ru-central1-c, id_prefix: ajv }
    - { type: KUBERNETES, address: "127.0.0.1:4450", location_type: GLOBAL, location: global, id_prefix: ebk }
    - { type: KUBERNETES, address: "127.0.0.1:4450", location_type: REGIONAL, location: ru-central1, id_prefix: c49 }
  endpoints:
    - { name: endpoint, host: api.cloud-testing.yandex.net }
    - { name: compute, host: api.cloud-testing.yandex.net }
    - { name: vpc, host: api.cloud-testing.yandex.net }
    - { name: iam, host: api.cloud-testing.yandex.net }
    - { name: resourcemanager, host: api.cloud-testing.yandex.net }
    - { name: operation, host: api.cloud-testing.yandex.net }
    - { name: mdb-postgresql, host: api.cloud-testing.yandex.net }
    - { name: mdb-clickhouse, host: api.cloud-testing.yandex.net }
    - { name: mdb-mongodb, host: api.cloud-testing.yandex.net }
    - { name: mdb-mysql, host: api.cloud-testing.yandex.net }
    - { name: mdb-redis, host: api.cloud-testing.yandex.net }
    - { name: managed-postgresql, host: api.cloud-testing.yandex.net }
    - { name: managed-clickhouse, host: api.cloud-testing.yandex.net }
    - { name: managed-mongodb, host: api.cloud-testing.yandex.net }
    - { name: managed-mysql, host: api.cloud-testing.yandex.net }
    - { name: managed-redis, host: api.cloud-testing.yandex.net }
    - { name: load-balancer, host: load-balancer.api.cloud-testing.yandex.net }
    - { name: k8s, host: managed-kubernetes.api.cloud-testing.yandex.net }

microcosm:
  features:
    sync_resource_statuses: true

  push_client:
    enabled: True

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
    snapshot: 0e6ee1d6-e005-01fc-c31e-bb1d96c2cb0a
    ycloud: ac3c83da-85ce-f59d-b573-b7ccd7183c63
    ycr: df2dcfee-3e7a-a034-a03c-631622618e7f
    SQS: 74a1fb86-8db2-dff3-75b8-474fbf6e1bf1
    solomon: 702a9926-f451-af65-81df-82b49a42341c

load-balancer:
  hc-ctrl:
    endpoint: hc.private-api.cloud-testing.yandex.net:4051
  lb-ctrl:
    endpoint: lb.private-api.cloud-testing.yandex.net:4051

kikimr_prefix: "testing_"

s3:
  mds_endpoint:
    read:  "http://storage-int.mds.yandex.net:80"
    write: "http://storage-int.mds.yandex.net:1111"
