secrets:
  api:
    cert: "api.crt"
    key: "api.key"
  metadata:
    # FIXME: don't enable until CLOUD-18176 is done
    generate_random_key: False

push_client:
  enabled: True
  defaults:
    ident: yc-test
    tvm:
      client_id: 2001289
  instances:
    sdn_antifraud:
      enabled: True

billing:
  id_prefix: ac0
  api:
    tvm_enabled: False

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

microcosm:
  features:
    sync_resource_statuses: false

identity:
  default_owner: tuman-cli@yc-test.yaconnect.com

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
    max_memory: 1G

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
    YMQ: 74a1fb86-8db2-dff3-75b8-474fbf6e1bf1

load-balancer:
  hc-ctrl:
    endpoint: localhost:4051
  lb-ctrl:
    endpoint: localhost:4051

kikimr_prefix: "dev_"

e2e-tests:
  skipped_tests: "test_ipv4_connectivity,test_martian_connectivity,test_static_routes.py,test_ipv4_dns_ext_records"
  
