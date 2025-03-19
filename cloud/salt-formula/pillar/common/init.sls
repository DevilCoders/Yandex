include:
 - common.billing

{%- set environment = grains['cluster_map']['environment'] %}
{% set cloudrepo_branches = {
  'dev': { 'branches': [ { 'unstable': 100 }, { 'testing': 600 }, { 'prestable': 600 }, { 'stable': 600 } ] },
  'hw-ci': { 'branches': [ { 'testing': 600 }, { 'prestable': 600 }, { 'stable': 600 } ] },
  'testing': { 'branches': [ { 'testing': 600 }, { 'prestable': 600 }, { 'stable': 600 } ] },
  'pre-prod': { 'branches': [ { 'prestable': 600 }, { 'stable': 600 } ] },
  'prod': { 'branches': [ { 'stable': 600 } ] }
}
%}

cloudrepo_branches: {{ cloudrepo_branches }}

{% set kikimr_tenant_ports = {
  "billing":   {"grpc_port": 2136, "mon_port": 8966, "ic_port": 19101 },
  "iam":  {"grpc_port": 2137, "mon_port": 8967, "ic_port": 19102 },
  "snapshot":  {"grpc_port": 2138, "mon_port": 8968, "ic_port": 19103 },
  "ycloud":    {"grpc_port": 2139, "mon_port": 8969, "ic_port": 19104 },
  "compute":   {"grpc_port": 2140, "mon_port": 8970, "ic_port": 19105 },
  "loadbalancer":   {"grpc_port": 2141, "mon_port": 8971, "ic_port": 19106 },
}
%}

kikimr_tenant_ports: {{ kikimr_tenant_ports }}

dns:
  # This is "gray vDNS" where user VMs are created. Common for all stands.
  # Domain suffix: .<region_name>.<internal_zone>
  virtual_dns:
    internal_zone: internal
    region_name: ru-central1 # FIXME: remove it from here and get it from host location info.
    ttl: 60
  forward_dns:
    default: yandex_dns64
    v6only: yandex_dns64

selfdns_api:
  token:
    AQAD-qJSJxaaAAAAkEocZVp460WJvumodqF4l7c

zoidberg:
  oauth:
    conductor: AQAD-qJSJldbAAAA7fWCx13_3kDeqCO7L3Pkx_M
    intranet_id: aa3515759cf44562879eb8dc3418c9b6

push_client:
  defaults:
    dc: my
    host: logbroker.yandex.net
    tvm:
       enabled: True
       server_id: 2001059
       secret_file: /var/lib/yc/push-client/tvm.secret

  instances:
    billing:
      dc: vla
      host: vla.logbroker.yandex.net
    billing-nlb:
      dc: vla
      host: vla.logbroker.yandex.net
    cloud_logs:
      ident: yc-serverless-logs@preprod

disks:
  ssd:
    iopsSeqRead: 20000
    iopsSeqWrite: 20000
    iopsRandRead: 20000
    iopsRandWrite: 20000
  hdd:
    iopsSeqRead: 20000
    iopsSeqWrite: 20000
    iopsRandRead: 200
    iopsRandWrite: 200

identity:
  default_owner: yndx-ycdf-bs
  private_url: http://[::]:2637/private/v1
  monitoring_endpoint:
    public: http://[::1]:4336/public/v1alpha1/health?monrun=1
    private: http://[::1]:2637/private/v1alpha1/health?monrun=1
  blackbox:
    - passport_url: "https://pass-test.yandex.ru/blackbox"
      tvm_client_id: 224
      mds_url: "https://avatars.mdst.yandex.net"
      sessionid_cookie_host: "yandex.ru"
  tvm:
    client_id: 2000665
    blackbox_instance: "Test"
  system_accounts:
    public_keys_file: "test.json"

scheduler:
  monitoring_endpoint: http://[::]:7000/v1/status

lbs:
  monitoring_endpoint: http://[::1]:7807/v1/status

compute-api:
  private_url: http://[::1]:9000/private/v1
  monitoring_endpoint: http://[::1]:9000/private/v1/health?monrun=1
  responsibles: svc_yccompute,svc_ycincome,svc_ycsecurity,svc_yc_iam_dev,svc_duty_on_ycloud,svc_ycvpc,svc_networkloadbalancer

  default_api_limits:
    max_operations_per_folder: 15

  global_hard_limits:
    instance:
      max_nbs_disks: 6
      max_network_interfaces: 1

  default_quotas:
    cloud:
      instances: 8
      cores: 8
      gpus: 0
      memory: 64G
      templates: 100
      snapshots: 32
      total_snapshot_size: 400G
      nbs_disks: 32
      total_disk_size: 200G
      network_hdd_total_disk_size: 200G
      network_ssd_total_disk_size: 200G
      images: 32
      networks: 2
      subnets: 12
      target_groups: 100
      network_load_balancers: 2
      external_addresses: 8
      external_qrator_addresses: 100000
      external_smtp_direct_addresses: 0
      external_static_addresses: 2
      route_tables: 8
      static_routes: 256

compute-worker:
  monitoring_endpoint: http://[::1]:9100/v1/health?monrun=1

compute-node:
  monitoring_endpoint: http://[::1]:8000/v1/health?monrun=1

compute-head:
  monitoring_endpoint: http://localhost:9080/health

compute-metadata:
  monitoring_endpoint: http://localhost:6771/v1/health

snapshot:
  mds:
    host: "storage-int.mdst.yandex.net"
    wport: 1111
    rport: 80
    auth_string: "Basic Z2xhbmNlOjA3OGVkMDE5OWI2YmI5MTE1N2VlZmRkZjc3OGRhYzc3"
    namespace: glance
  s3:
    region: us-east-1
    endpoint: http://s3.mds.yandex.net
    token_endpoint: http://169.254.169.254:21212/latest
    profile: s3mds-v2-596
  monitoring_endpoint: http://[::1]:7629/status

serialproxy:
  monitoring_endpoint: http://localhost:9602/v1/health

serialssh:
  monitoring_endpoint: http://[::1]:9601/health

{% set hostname = grains['nodename'] %}
{% set base_role = grains['cluster_map']['hosts'][hostname]['base_role'] %}
{% set host_tags = grains['cluster_map']['hosts'][hostname].get('tags', []) %}

access-service:
  grpcServer:
    port: 4286
  monitoring_endpoint:
    host: 'localhost'
    port: 4285
    path: '/ping'
  cache_reload:
    batch_size: 1000
    page_size: 1000
    shard_batch_size: 1000
    interval: 'PT30S'
    immutable_interval: 'PT1H'

resource-manager:
  config:
    max_memory: 500M
  grpcServer:
    port: 7376

oct:
  rabbit:
    cookie: JOONZRUZCVBOIFWYYJWU
  database:
    configdb_name: oc_config_db
    analyticsdb_name: oc_analitycs_db
    cassandra_lastalive_file: /var/lib/cassandra/lastalive.timestamp
    cassandra_dont_rejoin_after: 86400
  contrail-dns:
    reload_interval_min: 5  # Keep in sync with named_max_retransmissions * named_retransmission_interval @ contrail-dns.conf (CLOUD-17650)
    named_collect_metrics_interval_min: 5
  log_settings:
    defaults:
      # These defaults apply to:
      # contrail-api, contrail-discovery, contrail-schema, contrail-control, contrail-dns,
      # contrail-vrouter-agent, contrail-collector, contrail-query-engine, contrail-analytics-api,
      # ifmap, zookeeper, cassandra
      sandesh_level: SYS_INFO         # Sandesh levels: SYS_EMERG, SYS_ALERT, SYS_CRIT, SYS_ERR, SYS_WARN, SYS_NOTICE, SYS_INFO, SYS_DEBUG
      sandesh_send_rate_limit: 10000  # Throttle system logs transmitted per second. System logs are dropped if the sending rate is exceeded
      pylogger_level: INFO    # Python levels: DEBUG, INFO, WARNING, ERROR, CRITICAL (valid only for python services)
      log4cplus_level: DEBUG  # Note that these are mostly useless cause C++ Sandesh
                              # resets Log4Cplus level during initialization to match sandesh_level
      files_count: 50
      file_size_mb: 100
    #contrail-analytics-api:   # Example of overriding
    #  sandesh_level: SYS_NOTICE
    contrail-control:
      sandesh_level: SYS_NOTICE # Log notices for ifmap
    contrail-dns:           # This service has additional config parameter:
      named_level: warning  # named levels: critical, error, warning, notice, info, debug, dynamic
    contrail-web:           # This one can configure only level, no configuration for log file rotation is avaliable
      level: notice         # levels: debug, info, notice, warning, error, crit, alert, emerg
    ifmap:
      level: INFO      # log4j levels: TRACE, DEBUG, INFO, WARN, ERROR, ALL or OFF
    zookeeper:
      level: INFO      # log4j levels: TRACE, DEBUG, INFO, WARN, ERROR, ALL or OFF
    redis:             # Redis can configure only level, no configuration for log file rotation is avaliable
      level: notice    # redis levels: debug, verbose, notice, warning
    cassandra:
      level: DEBUG     # logback levels: TRACE, DEBUG, INFO, WARN, ERROR, ALL or OFF
  flow_log:
    directory: /var/log/flows
  flow_export_rate: 8000
  restart:
    additional_delay_after_checks_sec:
      contrail-api: 60
      contrail-control: 60
      contrail-discovery: 60
      contrail-schema: 60
      contrail-dns: 120
      contrail-vrouter-agent: 10
      contrail-vrouter-agent-and-kernel-module: 10

monitoring:
  need_service_restart:
    # We use dir on tmpfs because no restarts are needed after reboot
    marker_dir: /var/run/yc-need-service-restart


# Static network configuration bits
network:
  interfaces:
    vhost_dev: vhost0

  # Where to render config files before they
  # are synced to /etc/network.
  #
  # We want this to persist across reboots otherwise
  # changes such as deleting a file here won't be
  # detected as changes by Salt.
  tmp_config_prefix: /var/lib/yc/network-config
  # TODO(k-zaitsev): Moving from /var/run to /var/lib. Remove after CLOUD-10651 has been deployed
  old_tmp_config_prefix: /var/run/yc-network-config

network-billing-collector:
  worker_count: 4
  wake_up_sec: 300 # 5 minutes
  gc_sec: 604800 # 7 days
  network_attachment_info_dir: /var/lib/yc/compute-node/network_attachments
  accounting:
    log_file: /var/lib/yc/network-billing-collector/accounting/accounting.log
    rotation_max_bytes: 104857600  # 100 Mb
    backup_count: 10
  antifraud:
    log_file: /var/lib/yc/network-billing-collector/antifraud/antifraud.log
    rotation_max_bytes: 104857600  # 100 Mb
    backup_count: 50

api-adapter:
  solomon_pull_port: 4440

e2e-tests:
  ydb_cluster: "global"
  skipped_tests:
    # CLOUD-17182: enable ipv4 test for permnets only @pre-prod
    - test_permanent_vm.py::test_ipv4_connectivity

service_hosts:
  bootstrap:
    - vla04-s9-1.cloud.yandex.net

  bastion:
    - myt1-s3-4.cloud.yandex.net
    - vla04-s15-37.cloud.yandex.net
    - vla04-s9-38.cloud.yandex.net
    - sas09-s9-1.cloud.yandex.net

# Retrieved from `http://hbf.yandex.net/macros/$MACRO'
hbf_macroses:
  _CLOUDNETS_:
    - '2a02:6b8:bf00::/56'
    - '2a02:6b8:bf00:100::/56'
    - '2a02:6b8:bf00:1000::/56'
    - '2a02:6b8:bf00:1100::/56'
    - '2a02:6b8:bf00:2000::/56'

  _UNIFIEDUSERNETS_:
    - '2620:10f:d000:101::/64'
    - '2620:10f:d000:307::/64'
    - '2620:10f:d000:308::/64'
    - '2a02:6b8:0:4::/64'
    - '2a02:6b8:0:5::/64'
    - '2a02:6b8:0:6::/64'
    - '2a02:6b8:0:106::/64'
    - '2a02:6b8:0:107::/64'
    - '2a02:6b8:0:108::/64'
    - '2a02:6b8:0:164::/64'
    - '2a02:6b8:0:21c::/64'
    - '2a02:6b8:0:21d::/64'
    - '2a02:6b8:0:21e::/64'
    - '2a02:6b8:0:21f::/64'
    - '2a02:6b8:0:401::/64'
    - '2a02:6b8:0:402::/64'
    - '2a02:6b8:0:405::/64'
    - '2a02:6b8:0:406::/64'
    - '2a02:6b8:0:407::/64'
    - '2a02:6b8:0:408::/64'
    - '2a02:6b8:0:409::/64'
    - '2a02:6b8:0:40a::/64'
    - '2a02:6b8:0:40c::/64'
    - '2a02:6b8:0:40e::/64'
    - '2a02:6b8:0:419::/64'
    - '2a02:6b8:0:420::/64'
    - '2a02:6b8:0:421::/64'
    - '2a02:6b8:0:460::/64'
    - '2a02:6b8:0:505::/64'
    - '2a02:6b8:0:506::/64'
    - '2a02:6b8:0:60f::/64'
    - '2a02:6b8:0:616::/64'
    - '2a02:6b8:0:619::/64'
    - '2a02:6b8:0:81f::/64'
    - '2a02:6b8:0:825::/64'
    - '2a02:6b8:0:827::/64'
    - '2a02:6b8:0:828::/64'
    - '2a02:6b8:0:82c::/64'
    - '2a02:6b8:0:82e::/64'
    - '2a02:6b8:0:845::/64'
    - '2a02:6b8:0:848::/64'
    - '2a02:6b8:0:c33::/64'
    - '2a02:6b8:0:c3a::/64'
    - '2a02:6b8:0:c3e::/64'
    - '2a02:6b8:0:c42::/64'
    - '2a02:6b8:0:c4e::/64'
    - '2a02:6b8:0:1492::/64'
    - '2a02:6b8:0:1495::/64'
    - '2a02:6b8:0:1496::/64'
    - '2a02:6b8:0:149a::/64'
    - '2a02:6b8:0:149b::/64'
    - '2a02:6b8:0:149d::/64'
    - '2a02:6b8:0:2105::/64'
    - '2a02:6b8:0:210b::/64'
    - '2a02:6b8:0:2307::/64'
    - '2a02:6b8:0:2308::/64'
    - '2a02:6b8:0:2309::/64'
    - '2a02:6b8:0:230f::/64'
    - '2a02:6b8:0:2311::/64'
    - '2a02:6b8:0:2312::/64'
    - '2a02:6b8:0:2605::/64'
    - '2a02:6b8:0:260b::/64'
    - '2a02:6b8:0:2704::/64'
    - '2a02:6b8:0:2707::/64'
    - '2a02:6b8:0:2807::/64'
    - '2a02:6b8:0:2809::/64'
    - '2a02:6b8:0:2903::/64'
    - '2a02:6b8:0:290c::/64'
    - '2a02:6b8:0:291e::/64'
    - '2a02:6b8:0:2a04::/64'
    - '2a02:6b8:0:2a05::/64'
    - '2a02:6b8:0:2d0c::/64'
    - '2a02:6b8:0:2d12::/64'
    - '2a02:6b8:0:2e03::/64'
    - '2a02:6b8:0:2e15::/64'
    - '2a02:6b8:0:2f05::/64'
    - '2a02:6b8:0:2f0a::/64'
    - '2a02:6b8:0:3202::/64'
    - '2a02:6b8:0:3204::/64'
    - '2a02:6b8:0:3205::/64'
    - '2a02:6b8:0:3207::/64'
    - '2a02:6b8:0:3208::/64'
    - '2a02:6b8:0:3209::/64'
    - '2a02:6b8:0:320a::/64'
    - '2a02:6b8:0:3264::/64'
    - '2a02:6b8:0:3303::/64'
    - '2a02:6b8:0:3304::/64'
    - '2a02:6b8:0:3307::/64'
    - '2a02:6b8:0:3504::/64'
    - '2a02:6b8:0:3505::/64'
    - '2a02:6b8:0:3603::/64'
    - '2a02:6b8:0:3604::/64'
    - '2a02:6b8:0:370f::/64'
    - '2a02:6b8:0:3710::/64'
    - '2a02:6b8:0:3711::/64'
    - '2a02:6b8:0:3712::/64'
    - '2a02:6b8:0:3713::/64'
    - '2a02:6b8:0:3714::/64'
    - '2a02:6b8:0:3715::/64'
    - '2a02:6b8:0:3716::/64'
    - '2a02:6b8:0:3717::/64'
    - '2a02:6b8:0:3718::/64'
    - '2a02:6b8:0:3719::/64'
    - '2a02:6b8:0:3804::/64'
    - '2a02:6b8:0:3806::/64'
    - '2a02:6b8:0:3903::/64'
    - '2a02:6b8:0:390e::/64'
    - '2a02:6b8:0:3a05::/64'
    - '2a02:6b8:0:3a08::/64'
    - '2a02:6b8:0:3b05::/64'
    - '2a02:6b8:0:3b06::/64'
    - '2a02:6b8:0:4308::/64'
    - '2a02:6b8:0:4404::/64'
    - '2a02:6b8:0:440a::/64'
    - '2a02:6b8:0:4506::/64'
    - '2a02:6b8:0:450c::/64'
    - '2a02:6b8:0:4606::/64'
    - '2a02:6b8:0:460c::/64'
    - '2a02:6b8:0:4806::/64'
    - '2a02:6b8:0:480c::/64'
    - '2a02:6b8:0:4907::/64'
    - '2a02:6b8:0:490b::/64'
    - '2a02:6b8:0:4a06::/64'
    - '2a02:6b8:0:4a0c::/64'
    - '2a02:6b8:0:4b06::/64'
    - '2a02:6b8:0:4b0b::/64'
    - '2a02:6b8:0:4c06::/64'
    - '2a02:6b8:0:4c0c::/64'
    - '2a02:6b8:0:4d06::/64'
    - '2a02:6b8:0:4d0b::/64'
    - '2a02:6b8:0:4d0e::/64'
    - '2a02:6b8:0:4e06::/64'
    - '2a02:6b8:0:4e0b::/64'
    - '2a02:6b8:0:c005::/64'
    - '2a02:6b8:b010:c007::/64'
    - '2a02:6b8:b010:c043::/64'
    - '2a02:6b8:b010:d001::/64'
    - '2a02:6b8:b010:d002::/64'
    - '2a02:6b8:b010:d003::/64'
    - '2a02:6b8:b010:d004::/64'
    - '2a02:6b8:b010:d005::/64'
    - '2a02:6b8:b010:d006::/64'
    - '2a02:6b8:b010:d007::/64'
    - '2a02:6b8:b010:d008::/64'
    - '2a02:6b8:b010:d009::/64'
    - '2a02:6b8:b010:d00a::/64'
    - '2a02:6b8:b010:d00b::/64'
    - '2a02:6b8:b010:d00c::/64'
    - '2a02:6b8:b010:e008::/64'
    - '2a02:6b8:b012:1004::/64'

{% set zone_id = salt['grains.get']('cluster_map:hosts:%s:location:zone_id' % hostname, None) %}

{% set qemu_version = '1:2.12.0-20' %}

{# ###### OpenContrail Package Version ##### #}
{# Note: all packages >= 3.2.3.80 built from branch R3.2.3.x
   has relaxed dependencies, it's better to use them #}
{# See wiki for more information about dependencies:
   https://wiki.yandex-team.ru/cloud/devel/sdn/OpenContrail-Packages/ #}

{# Packages used only on oct-heads: #}
    {%- set oct_config_version = '3.2.3.124.20190426132400-0' %}
    {%- set oct_dns_version = '3.2.3.63~20190221135700-0' %}

    {%- if 'cloud-17939-upgrade-bind' in host_tags %}
    {%- set oct_dns_version = '3.2.3.89~20190320131300-0' %}
    {%- endif %}

    {%- if 'cloud-18583-single-view-dns-13063-contrail-named-lock-20138-zone-file-removing-logging-tag' in host_tags %}
    {%- set oct_dns_version = '3.2.3.147.20190625022600-0' %}
    {%- endif %}

    {%- set oct_control_version = '3.2.3.127.20190430085400-0' %}
    {%- set oct_web_version = '3.2.3.89~20190320131300-0' %}

{# Packages used only on compute-nodes #}
    {# Note: utils and python_packages version should be >= vrouter-dkms version
       python_packages are not really used and can be removed in the future #}
    {%- set oct_vrouter_dkms_version            = '3.2.3.141.20190619174300-0' %}
    {%- set oct_vrouter_utils_version           = oct_vrouter_dkms_version     %}
    {%- set oct_vrouter_python_packages_version = oct_vrouter_dkms_version     %}
    {%- set oct_vrouter_agent_version           = '3.2.3.144.20190624115100-0' %}

{# Packages used on both: #}
    {# contrail-vrouter-agent, contrail-dns and contrail-control depends contrail-lib
       but there is high chance it's actually not used. #}
    {# oct_python_packages_version sets version for python-contrail, contrail-nodemgr and contrail-utils.
       contrail-config depends on python-contrail #}

    {%- if base_role in ('oct-head', 'cloudvm') %}
    {%- set oct_contrail_lib_version = '3.2.3.106~20190404091800-0' %}  {# Safe to have newer version, because its content (libthrift) never changes. #}
    {%- set oct_python_packages_version = '3.2.3.55~20190220115700-0' %}
    {%- else %}
    {%- set oct_contrail_lib_version = '3.2.3.54~20190121181000-0' %}
    {%- set oct_python_packages_version = '3.2.3.54~20190121181000-0' %}
    {%- endif %}

{# OVERRIDES: #}
    {# Use this to upgrade components separately on head/compute nodes or PROD/PREPROD.
       Remember, that CI/CVM uses base_role == 'cloudvm' #}

{# ##### End of OpenContrail Package Versions ##### #}

{% if base_role == 'loadbalancer-node' %}
    {%- set salt_version = '2018.3.2-yandex2' %}
{% else %}
    {%- set salt_version = '2017.7.2-yandex1' %}
{% endif %}

{% set compute_version = '0.5.1-3684.190620' %}
{% set scheduler_version = '0.5.2-181.190618' %}
{% set serialproxy_version = '0.1-210.190531' %}
{% set serialssh_version = '0.1-210.190531' %}
{% set snapshot_version = '0.5.1-325.190617' %}


{% set identity_version = '0.2.1-1494.190626' %}

{% set solomon_version = '4287374.stable-2018-12-10' %}
{% set solomon_conf_version = '4287374.stable-2018-12-10' %}

yc-pinning:
  packages:
    apparmor-ycloud-dhcp-prof: 1.0.1
    apparmor-ycloud-svc-serialssh-prof: 1.0.0
    cassandra: 2.2.11.12
    config-caching-dns: 1.0-49
    contrail-api-client: 1.6.95
    hbf-agent: 3.2.5
    i40evf-dkms: 3.5.6-1.7
    mlnx-ofed-kernel-dkms: 4.3-OFED.4.3.1.0.1.1.g8509e41
    mlnx-ofed-kernel-utils: 4.3-OFED.4.3.1.0.1.1.g8509e41
    gobgp: 1.32.3.32
    ifmap-server: 0.3.2-1contrail6+testing1
    iproute2: 4.9.0-1ubuntu3.1
    juggler-client: 2.3.1809101707
    libprotobuf10: 3.0.0-9ubuntu1.6
    librte-eal3: 16.11.1-0ubuntu3.1
    librte-ethdev5: 16.11.1-0ubuntu3.1
    librte-kvargs1: 16.11.1-0ubuntu3.1
    librte-mbuf2: 16.11.1-0ubuntu3.1
    librte-mempool2: 16.11.1-0ubuntu3.1
    librte-meter1: 16.11.1-0ubuntu3.1
    librte-pdump1: 16.11.1-0ubuntu3.1
    librte-pmd-ring2: 16.11.1-0ubuntu3.1
    librte-ring1: 16.11.1-0ubuntu3.1
    librte-vhost3: 16.11.1-0ubuntu3.1
    libshflags: 1.0.3-yandex1
    libzookeeper-java: 3.4.10-2+yandex0.3
    libzookeeper-mt2: 3.4.10-2+yandex0.3
    linux-headers-4.14.46-15+yc2: 4.14.46-15+yc2
    linux-image-4.14.46-15+yc2: 4.14.46-15+yc2
    linux-libc-dev: 4.14.46-15+yc2
    linux-tools: 4.14.46-15+yc2
    linux-virtual: 4.14.46-15+yc2
    monrun: 1.3.1
    nodejs: 7.7.3-1nodesource1~xenial1.1
    openvswitch-common: 2.7.0-1+dpdk.35
    python-cassandra: 3.7.1-2.1mirantis.1
    python-consistent-hash: 1.0-0tcp1+xenial1.3
    python-geventhttpclient: 1.3.1-1contrail0.2
    python-openvswitch: 2.7.0-1+dpdk.35
    qemu: {{qemu_version}}
    qemu-kvm: {{qemu_version}}
    qemu-block-extra: {{qemu_version}}
    qemu-system-common: {{qemu_version}}
    qemu-system-x86: {{qemu_version}}
    qemu-utils: {{qemu_version}}
    seabios: 1.11.1-1ubuntu1
    rabbitmq-server: 3.6.1-yandex0-1
    salt-common: {{ salt_version }}
    salt-master: {{ salt_version }}
    salt-minion: {{ salt_version }}
    yandex-archive-keyring: 2016.06.17-1
    yandex-cauth: 1.6.3
    yandex-internal-root-ca: 2013.02.11-3
    yandex-jdk8: 8.144-tzdata2017b
    yandex-juggler-http-check: '0.16'
    yandex-logbroker-sqs: 3859131.trunk.281610954
    yandex-logbroker-sqs-tests: 3662902.trunk.257020256
    yandex-netmon-agent: 0.7.0-3948601
    yandex-oops-agent: 1.0-23
    yandex-push-client: 6.49.7
    yandex-solomon-sysmond: "1:7.3"
    yandex-z2-worker: "24.2"
    yandex-selfdns-client: 0.2.7
    yandex-solomon-agent-bin: "1:13.8"
    yandex-solomon-alerting: {{ solomon_version }}
    yandex-solomon-gateway: {{ solomon_version }}
    yandex-solomon-coremon: {{ solomon_version }}
    yandex-solomon-fetcher: {{ solomon_version }}
    yandex-solomon-backend: {{ solomon_version }}
    yandex-solomon-web: {{ solomon_version }}
    yandex-solomon-stockpile: 4288584.stable-2018-11-28
    yandex-solomon-common: 4288884.stable-2018-12-10
    yandex-solomon-common-conf: 4287374.stable-2018-12-10
    yandex-solomon-alerting-conf-cloud: {{ solomon_conf_version }}
    yandex-solomon-gateway-conf-cloud:  {{ solomon_conf_version }}
    yandex-solomon-coremon-conf-cloud: {{ solomon_conf_version }}
    yandex-solomon-fetcher-conf-cloud: {{ solomon_conf_version }}
    yandex-solomon-backend-conf-cloud: {{ solomon_conf_version }}
    yandex-solomon-stockpile-conf-cloud: 4288584.stable-2018-11-28
    yc-access-service: 1.8139+fb93ecb
    yc-api-gateway: 0.5.2-1457.190313
    yc-api-gateway-tests: 0.5.2-1457.190313
    yc-api-adapter: 1.8390+9826b26
    yc-api-adapter-tests: 1.8390+9826b26
    yc-automated-tests: 0.3-357.181206
    yc-autorecovery: 0.1-9.190513
    yc-autoscale-agent: 0.5.2-1316.190225
    yc-cassandra-plugin-jolokia: 0.1-1.4
    yc-cli: 0.1-287.190611
    yc-compute: {{ compute_version }}
    yc-compute-node: {{ compute_version }}
    yc-compute-tests: {{ compute_version }}
    yc-e2e-tests: 0.1-222.190611
    yc-compute-head: 0.1-0.190616 
    yc-compute-metadata: 0.1-42.190531
    yc-graphics: 0.0.1-1
    yc-healthcheck-ctrl: 0.5.2-207.180828
    yc-healthcheck-node: 0.5.2-207.180828
    yc-healthcheck-tests: 0.0.1-167
    yc-iam-takeout-agent: {{ identity_version }}
    yc-identity: {{ identity_version }}
    yc-identity-tests: {{ identity_version }}
    yc-network-billing-collector: 0.2-54.190311
    yc-network-config: 0.1-1.190514
    yc-network-oncall-tools: 0.1-29.190426
    yc-resource-manager: 0.7490+6e8771a
    yc-rm-takeout-agent: {{ identity_version }}
    yc-salt-master-config: 0.0.1-15.180427
    yc-scheduler: {{ scheduler_version }}
    yc-scms: {{ identity_version }}
    yc-selfdns-plugins: 0.1-1
    yc-serialproxy: {{ serialproxy_version }}
    yc-serialssh: {{ serialssh_version }}
    yc-snapshot: {{ snapshot_version }}
    yc-solomon-agent-plugins: 0.5.1-104.190626
    yc-solomon-agent-systemd: 0.0.1-1.2
    yc-token-agent: 5168698.trunk.0
    yc-log-reader: 0.5.1-41.190430
    yc-loadbalancer-ctrl: 0.5.2-207.180828
    zookeeper: 3.4.10-2+yandex0.3
    zookeeperd: 3.4.10-2+yandex0.3
    redis-server: 2:3.0.6-1ubuntu0.3
    redis-tools: 2:3.0.6-1ubuntu0.3
    nginx: 1.14.2-1.yandex.2
    python-anyjson: 0.3.3-1build1
    contrail-analytics: 3.2.3-should-not-be-installed
    contrail-config: {{ oct_config_version }}
    contrail-control: {{ oct_control_version }}
    contrail-control-dbg: {{ oct_control_version }}
    contrail-dns: {{ oct_dns_version }}
    contrail-lib: {{ oct_contrail_lib_version }}
    contrail-nodemgr: {{ oct_python_packages_version }}
    contrail-utils: {{ oct_python_packages_version }}
    contrail-vrouter-agent: {{ oct_vrouter_agent_version }}
    contrail-vrouter-agent-dbg: {{ oct_vrouter_agent_version }}
    contrail-vrouter-dkms: {{ oct_vrouter_dkms_version }}
    contrail-vrouter-source: {{ oct_vrouter_dkms_version }}
    contrail-vrouter-utils: {{ oct_vrouter_utils_version }}
    contrail-web-controller: {{ oct_web_version }}
    contrail-web-core: {{ oct_web_version }}
    python-contrail: {{ oct_python_packages_version }}
    python-contrail-vrouter-api: {{ oct_vrouter_python_packages_version }}
    python-opencontrail-vrouter-netns: {{ oct_vrouter_python_packages_version }}
    osquery-vanilla: 3.3.1.0
    osquery-yandex-generic-config: 1.1.0.1
    yc-setup-configs: 0.2-32.190614

# INFRA PKGS
{% set systemd_packages_version = "229-4ubuntu21.21" %}
{% set samba_libs_packages_version = "2:4.3.11+dfsg-0ubuntu0.16.04.20" %}
{% set libcurl_packages_version = "7.47.0-1ubuntu2.13" %}
    apt: 1.2.29ubuntu0.1
    busybox-initramfs: 1:1.22.0-15ubuntu1.4
    curl: {{ libcurl_packages_version }}
    gettext: 0.19.7-2ubuntu3.1
    git: 1:2.7.4-0ubuntu1.6
    gnupg: 1.4.20-1ubuntu3.3
    gpgv: 1.4.20-1ubuntu3.3
    intel-microcode: 3.20180807a.0ubuntu0.16.04.1
    libcurl3: {{ libcurl_packages_version }}
    libcurl3-gnutls: {{ libcurl_packages_version }}
    libgnutls30: 3.4.10-4ubuntu1.5
    libglib2.0-0: 2.48.2-0ubuntu4.1
    libnss3: 2:3.28.4-0ubuntu0.16.04.5
    libnss3-nssdb: 2:3.28.4-0ubuntu0.16.04.5
    libpolkit-backend-1-0: 0.105-14.1ubuntu0.5
    libpng12-0: 1.2.54-1ubuntu1.1
    libseccomp2: 2.4.1-0ubuntu0.16.04.2
    libsnmp30: 5.7.3+dfsg-1ubuntu4.2
    libssl1.0.0: 1.0.2g-1ubuntu4.15
    openssl: 1.0.2g-1ubuntu4.15
    libssl-dev: 1.0.2g-1ubuntu4.15
    libssl-doc: 1.0.2g-1ubuntu4.15
    libsmbclient: {{ samba_libs_packages_version }}
    libwbclient0: {{ samba_libs_packages_version }}
    libxml2: 2.9.3+dfsg1-1ubuntu0.6
    libxslt1.1: 1.1.28-2.1ubuntu0.2
    ntp: 1:4.2.8p4+dfsg-3ubuntu5.9
    ssh: 1:7.2p2-4ubuntu2.8
    openssh-client: 1:7.2p2-4ubuntu2.8
    openssh-server: 1:7.2p2-4ubuntu2.8
    openssh-sftp-server: 1:7.2p2-4ubuntu2.8
    perl: 5.22.1-9ubuntu0.6
    policykit-1: 0.105-14.1ubuntu0.5
    python-lxml: 3.5.0-1ubuntu0.1
    python-openssl: 0.15.1-2ubuntu0.2
    python-urllib3: 1.13.1-2ubuntu0.16.04.3
    python2.7: 2.7.12-1ubuntu0~16.04.4
    python2.7-minimal: 2.7.12-1ubuntu0~16.04.4
    python3-lxml: 3.5.0-1ubuntu0.1
    python3.5: 3.5.2-2ubuntu0~16.04.5
    python3.5-minimal: 3.5.2-2ubuntu0~16.04.5
    python3-requests: 2.9.1-3ubuntu0.1
    python3-urllib3: 1.13.1-2ubuntu0.16.04.3
    samba-libs: {{ samba_libs_packages_version }}
    systemd: {{ systemd_packages_version }}
    libpam-systemd: {{ systemd_packages_version }}
    libsystemd0: {{ systemd_packages_version }}
    systemd-sysv: {{ systemd_packages_version }}
    systemd-coredump: {{ systemd_packages_version }}
    zsh: 5.1.1-1ubuntu2.3
    yandex-hw-watcher: 0.6.5.9
    libldb1: 2:1.1.24-1ubuntu3.1
    file: 1:5.25-2ubuntu1.2
    libmagic1: 1:5.25-2ubuntu1.2
    wget: 1.17.1-1ubuntu1.5
    sudo: 1.8.16-0ubuntu1.6

secrets:
  internal_api:
    sign_crt_fp_sha256: "776F85331312474514D5A561E8BA06483813D13FF79FA8B4CC04D5AF2B464643"

osquery_tags:
  api-gateway: ycloud-svc-apigtw-config
  api-adapter: ycloud-svc-apiadapter-config
  bastion: ycloud-svc-bastion-config
  billing: ycloud-svc-billing-config
  cgw: ycloud-svc-cgw-config
  iam: ycloud-svc-iam-config
  loadbalancer-node: ycloud-svc-lb-config
  marketplace: ycloud-svc-mrkt-config
  oct-head: ycloud-svc-oct-config
  s3-proxy: ycloud-svc-s3-config
  serialssh: ycloud-svc-serialssh-config
  snapshot: ycloud-svc-snapshot-config
  solomon-gateway: ycloud-svc-solomon-config
  solomon-core: ycloud-svc-solomon-config
  solomon-stockpile: ycloud-svc-solomon-config
  sqs-dn: ycloud-svc-sqs-config
  yql: ycloud-svc-yql-config
  head: ycloud-svc-kikimrdn-config
  seed: ycloud-svc-kikimrdn-config
  kikimr_dyn_nodes: ycloud-svc-kikimrdn-config
{%- if 'cloud-20595-osquery-test' in host_tags %}
  compute: ycloud-hv-seccomp-config
{%- else %}
  compute: ycloud-hv-config
{%- endif %}
  slb-adapter: ycloud-svc-slbadapter-config
  cloudvm: ''
