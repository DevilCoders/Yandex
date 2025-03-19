include:
 - common.prod.billing

dns:
  virtual_dns:
    ttl: 600
  forward_dns:
    default: yandex_ns_cache
    v6only: yandex_dns64

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

push_client:
  enabled: True
  defaults:
    ident: yc
    tvm:
      client_id: 2001287
  instances:
    sdn_antifraud:
      enabled: True
    billing-nlb:
      enabled: True
    cloud_logs:
      ident: yc-serverless-logs@prod

compute-api:
  responsibles: svc_yccompute,svc_ycincome,svc_ycsecurity,svc_yc_iam_dev,svc_duty_on_ycloud,svc_ycvpc,svc_networkloadbalancer

compute-node:
  limit_shared_cores: true
  allow_software_virtualization: false
  allow_nested_virtualization: false
  deallocation:
    delay: 120

console:
  push:
    url: 'https://console.cloud.yandex.net/push/status'

placement:
  dc: null

identity:
  default_owner: yndx-ycprod-bs
  blackbox:
    - passport_url: "http://blackbox.yandex.net/blackbox"
      tvm_client_id: 222
      mds_url: "https://avatars.mds.yandex.net"
      sessionid_cookie_host: "yandex.ru"
  tvm:
    client_id: 2000669
    blackbox_instance: "Prod"
  push:
    url: 'https://console.cloud.yandex.net'
  system_accounts:
    public_keys_file: "prod.json"

access-service:
  config:
    max_cores: 6
    max_memory: 8G
  push-client:
    enabled: True

resource-manager:
  config:
    max_memory: 500M
  push-client:
    enabled: True

iam-takeout-agent:
  tvm:
    client_id: 2011552
    blackbox_instance: "Prod"
    allowed_clients: [2009785, 2011552]

resource-manager-takeout-agent:
  tvm:
    client_id: 2011556
    blackbox_instance: "Prod"
    allowed_clients: [2009785, 2011552]


api-adapter:
  mdb_lb_host: mdb.private-api.cloud.yandex.net
  push_client:
    enabled: True


api-gateway:
  push_client:
    enabled: True

  region: ru-central1
  cluster: prod
  certificate_file: /etc/envoy/ssl/certs/yc-api-gateway.crt
  private_key_file: /etc/envoy/ssl/private/yc-api-gateway.key
  services:
    - { id: yandex.cloud.compute.v1.DiskService, location: REGIONAL }
    - { id: yandex.cloud.compute.v1.DiskTypeService, location: GLOBAL }
    - { id: yandex.cloud.compute.v1.ImageService, location: GLOBAL }
    - { id: yandex.cloud.compute.v1.InstanceService, location: REGIONAL }
    - { id: yandex.cloud.compute.v1.SnapshotService, location: GLOBAL }
    - { id: yandex.cloud.compute.v1.ZoneService, location: GLOBAL }
    - { id: yandex.cloud.compute.v1.instancegroup.InstanceGroupService, location: GLOBAL }
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
  discovery:
    - { type: COMPUTE, address: "127.0.0.1:4433", location_type: GLOBAL, location: global, id_prefix: fd8 }
    - { type: COMPUTE, address: "127.0.0.1:4433", location_type: REGIONAL, location: ru-central1, id_prefix: btq }
    - { type: COMPUTE, address: "127.0.0.1:4433", location_type: ZONAL, location: ru-central1-a, id_prefix: fhm }
    - { type: COMPUTE, address: "127.0.0.1:4433", location_type: ZONAL, location: ru-central1-b, id_prefix: epd }
    - { type: COMPUTE, address: "127.0.0.1:4433", location_type: ZONAL, location: ru-central1-c, id_prefix: ef3 }
    - { type: VPC, address: "127.0.0.1:4433", location_type: GLOBAL, location: global, id_prefix: enp }
    - { type: VPC, address: "127.0.0.1:4433", location_type: REGIONAL, location: ru-central1, id_prefix: euu }
    - { type: VPC, address: "127.0.0.1:4433", location_type: ZONAL, location: ru-central1-a, id_prefix: e9b }
    - { type: VPC, address: "127.0.0.1:4433", location_type: ZONAL, location: ru-central1-b, id_prefix: e2l }
    - { type: VPC, address: "127.0.0.1:4433", location_type: ZONAL, location: ru-central1-c, id_prefix: b0c }
    - { type: IAM, address: "127.0.0.1:4433", location_type: GLOBAL, location: global, id_prefix: aje }
    - { type: RESOURCE_MANAGER, address: "127.0.0.1:4433", location_type: GLOBAL, location: global, id_prefix: b1g }
    - { type: MDB, address: "127.0.0.1:4433", location_type: GLOBAL, location: global, id_prefix: c9q }
    - { type: MDB_POSTGRESQL, address: "127.0.0.1:4433", location_type: GLOBAL, location: global, id_prefix: c9q }
    - { type: MDB_CLICKHOUSE, address: "127.0.0.1:4433", location_type: GLOBAL, location: global, id_prefix: c9q }
    - { type: MDB_MONGODB, address: "127.0.0.1:4433", location_type: GLOBAL, location: global, id_prefix: c9q }
    - { type: MDB_MYSQL, address: "127.0.0.1:4433", location_type: GLOBAL, location: global, id_prefix: c9q }
    - { type: MDB_REDIS, address: "127.0.0.1:4433", location_type: GLOBAL, location: global, id_prefix: c9q }
    - { type: CONTAINER_REGISTRY, address: "127.0.0.1:4438", location_type: GLOBAL, location: global, id_prefix: crp }
    - { type: MICROCOSM_INSTANCEGROUP, address: "127.0.0.1:4439", location_type: GLOBAL, location: global, id_prefix: cl1 }
    - { type: LOAD_BALANCER, address: "127.0.0.1:4433", location_type: GLOBAL, location: global, id_prefix: b7r }
    - { type: LOAD_BALANCER, address: "127.0.0.1:4433", location_type: REGIONAL, location: ru-central1, id_prefix: fg6 }
    - { type: LOAD_BALANCER, address: "127.0.0.1:4433", location_type: ZONAL, location: ru-central1-a, id_prefix: f5o }
    - { type: LOAD_BALANCER, address: "127.0.0.1:4433", location_type: ZONAL, location: ru-central1-b, id_prefix: ese }
    - { type: LOAD_BALANCER, address: "127.0.0.1:4433", location_type: ZONAL, location: ru-central1-c, id_prefix: blr }
  endpoints:
    - { name: endpoint, host: api.cloud.yandex.net }
    - { name: compute, host: compute.api.cloud.yandex.net }
    - { name: vpc, host: vpc.api.cloud.yandex.net }
    - { name: iam, host: iam.api.cloud.yandex.net }
    - { name: resourcemanager, host: resource-manager.api.cloud.yandex.net }
    - { name: resource-manager, host: resource-manager.api.cloud.yandex.net }
    - { name: operation, host: operation.api.cloud.yandex.net }
    - { name: mdb-postgresql, host: mdb.api.cloud.yandex.net }
    - { name: mdb-clickhouse, host: mdb.api.cloud.yandex.net }
    - { name: mdb-mongodb, host: mdb.api.cloud.yandex.net }
    - { name: mdb-mysql, host: mdb.api.cloud.yandex.net }
    - { name: mdb-redis, host: mdb.api.cloud.yandex.net }
    - { name: managed-postgresql, host: mdb.api.cloud.yandex.net }
    - { name: managed-clickhouse, host: mdb.api.cloud.yandex.net }
    - { name: managed-mongodb, host: mdb.api.cloud.yandex.net }
    - { name: managed-mysql, host: mdb.api.cloud.yandex.net }
    - { name: managed-redis, host: mdb.api.cloud.yandex.net }
    - { name: container-registry, host: container-registry.api.cloud.yandex.net }
    - { name: load-balancer, host: load-balancer.api.cloud.yandex.net }
    - { name: storage, host: storage.yandexcloud.net }
    - { name: serialssh, host: serialssh.cloud.yandex.net, port: 9600 }

  container_registry:
    host: container-registry.private-api.ycp.cloud.yandex.net
    port: 443

  microcosm_instancegroup:
    host: instance-group.private-api.ycp.cloud.yandex.net
    port: 443

microcosm:
  features:
    sync_resource_statuses: true

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

e2e-tests:
  skipped_tests:
    - test_remote_dns64
    # CLOUD-17182: enable ipv4 test for permnets only @pre-prod
    - test_permanent_vm.py::test_ipv4_connectivity

kikimr_secrets:
  cfg_dir: /etc/kikimr_secrets
  secrets_dir: /var/lib/kikimr_secrets
  disk:
    cfg_dir: /etc/kikimr_secrets
    dm_name: secrets.kikimr
    device_serial: INTERNAL_DATA
  tenants_keys:
    billing: 33980dd0-1468-6818-acc1-33d52a25a014
    iam: 8f1f8573-0a45-12c0-1f19-0ceb01b0fb8f
    loadbalancer: 128b31fc-7c72-c6dd-1f5b-9da628dddcb4
    microcosm: d1e550c3-95b2-a769-1192-0c173908e7f5
    mkt: 8a862dfa-096f-5ca0-7b26-e7a98251981f
    s3: 8bcc464d-556c-129f-f09f-b3d467c62061
    solomon: 6185d43e-d9ea-761d-fe6c-9d21f338cb32
    snapshot: 7a4514f7-b6a3-7072-09ab-8e444539082c
    ycloud: 5776d352-e942-62e8-6ed2-9245fe6f1990
    ycr: c0bf3063-d19a-0222-fd5d-64d7e27e55c1
    SQS: 37536152-345c-42c1-66c7-72d96bf3df7e
    instancegroup: fc864f49-ed74-548a-833b-bd80343409a6
    containerregistry: e606d779-bcfe-b907-e573-92e91d2fd26d
    serverlessfunctions: 8ab333ab-b8e6-9831-e250-2bdcd7218913
    serverlesslogs: a3f08829-a67b-b1e8-4bf3-0000e42478a8
    iotdevices: 4b6b9c74-d2c3-5474-bb3f-38a595577e50
    ai: 23d9da4a-f302-96c1-eaa3-43969d8c6f67
    ydbc: f7725c2e-9b15-614e-653c-c6a6b711e132
    kms: 516cbb4e-bf2a-7a4f-44f3-31031a4b7ed0
    apigateway-k8s: decd9024-2314-dd1a-7733-e038e99aaa9b
    apigateway-alb: a9ccb726-6090-cd27-d6fe-1a0430550a79

load-balancer:
  hc-ctrl:
    endpoint: hc.private-api.cloud.yandex.net:4051
  lb-ctrl:
    endpoint: lb.private-api.cloud.yandex.net:4051

kikimr_prefix: ""

message-queue:
  endpoint: message-queue.api.cloud.yandex.net

oct:
  restart:
    additional_delay_after_checks_sec:
      contrail-api: 600
      contrail-control: 600
      contrail-discovery: 600
      contrail-schema: 600
      contrail-dns: 1800
      contrail-vrouter-agent: 30
      contrail-vrouter-agent-and-kernel-module: 30

s3:
  mds_endpoint:
    read:  "http://storage-int.mds.yandex.net:80"
    write: "http://storage-int.mds.yandex.net:1111"
