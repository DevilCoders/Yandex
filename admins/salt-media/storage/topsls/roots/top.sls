{{saltenv|default('base')}}:
  '*':
    - units.common

    - common.monrun.salt-state
    - templates.juggler-search
    - templates.timezone
    - units.virtual-host
    - units.salt-minion-stop
    - units.robot-storage-duty

  "c:elliptics-test-collector":
    - match: grain
    - elliptics-test-collector
    - units.cocaine
  "c:elliptics-test-storage":
    - match: grain
    - elliptics-test-storage
  "c:elliptics-test-proxies":
    - match: grain
    - elliptics-test-proxies
  "c:elliptics-test-cloud":
    - match: grain
    - elliptics-test-cloud
    - units.cocaine

  "c:elliptics-storage":
    - match: grain
    - elliptics-storage
  "c:elliptics-dom0-lxd":
    - match: grain
    - elliptics-dom0-lxd
  "c:elliptics-test-dom0-lxd":
    - match: grain
    - elliptics-test-dom0-lxd
  "c:elliptics-proxy":
    - match: grain
    - elliptics-proxy
  "c:elliptics-cloud":
    - match: grain
    - elliptics-cloud
    - units.cocaine
  "c:elliptics-nscfg-sync":
    - match: grain
    - units.mastermind.nscfg-mongo-sync
  "c:elliptics-s3cleanup":
    - match: grain
    - elliptics-s3cleanup
  "c:elliptics-logstore":
    - match: grain
    - elliptics-logstore
  "c:elliptics-(test-)?yarl":
    - match: grain_pcre
    - units.yarl.master
    - templates.yasmagentnew
  "conductor:group:elliptics-valve":
    - match: grain
    - elliptics-valve
  "conductor:group:elliptics-test-valve":
    - match: grain
    - elliptics-test-valve
  "c:elliptics-mondb":
    - match: grain
    - elliptics-mondb
  "c:elliptics-rs":
    - match: grain
    - elliptics-rs
  "c:elliptics-l3mon":
    - match: grain
    - elliptics-l3mon
  "c:^elliptics-collector(-unstable)?$":
    - match: grain_pcre
    - elliptics-collector
    - units.cocaine

  "c:^elliptics-storage-lost.*":
    - match: grain_pcre
    - elliptics-storage-lost

  "c:elliptics-clickhouse":
    - match: grain
    - elliptics-clickhouse

  "c:elliptics-logshatter":
    - match: grain
    - elliptics-logshatter

  'c:elliptics-strm-log':
    - match: grain
    - elliptics-strm-log

  'c:elliptics-test-rbtorrent':
    - match: grain
    - elliptics-test-rbtorrent

  'c:elliptics-rbtorrent':
    - match: grain
    - elliptics-rbtorrent

  'c:elliptics-walle-cms':
    - match: grain
    - elliptics-walle-cms

  # storage/roots/*
  "c:^storage(-test)?-mulcagate$":
    - match: grain_pcre
    - mulcagate

  'c:elliptics_ycloud-storage':
    - match: grain
    - elliptics_ycloud-storage

  'c:elliptics-stable-salt':
    - match: grain
    - elliptics-stable-salt

  'c:elliptics-test-lepton':
    - match: grain
    - elliptics-test-lepton
  'c:elliptics-lepton':
    - match: grain
    - elliptics-lepton

  'conductor:groups:hbf_test_server':
    - match: grain
    - hbf_test_server
  'conductor:groups:nocdev-dom0-lxd':
    - match: grain
    - nocdev-dom0-lxd
  'conductor:groups:nocdev-decapinger':
    - match: grain
    - nocdev-decapinger
  'conductor:groups:nocdev-dom0-lxd-kernel':
    - match: grain
    - nocdev-dom0-lxd-kernel
  'conductor:groups:nocdev-ck':
    - match: grain
    - nocdev-ck
  'conductor:groups:nocdev-zk':
    - match: grain
    - nocdev-zk
  'conductor:groups:nocdev-test-ck':
    - match: grain
    - nocdev-test-ck
  'conductor:groups:nocdev-4k':
    - match: grain
    - nocdev-4k
  'conductor:groups:nocdev-test-4k':
    - match: grain
    - nocdev-test-4k
  'conductor:groups:nocdev-test-packfw':
    - match: grain
    - nocdev-test-packfw
  'conductor:groups:nocdev-packfw':
    - match: grain
    - nocdev-packfw
  'conductor:groups:nocdev-vcs':
    - match: grain
    - nocdev-vcs


  'conductor:group:deploy':
    - match: grain
    - deploy-mds-proxy

  # Federations
  'c:^elliptics-(test-)?storage-federations$':
    - match: grain_pcre
    - elliptics-storage-federations

  'c:^elliptics-(test-)?collector-federations$':
    - match: grain_pcre
    - elliptics-collector-federations

  'c:^elliptics-(test-)?cloud-federations$':
    - match: grain_pcre
    - elliptics-cloud-federations

# AVATARS
  "c:avatars-mds-test-proxy":
    - match: grain
    - avatars-mds-test-proxy
  "c:avatars-mds-proxy":
    - match: grain
    - avatars-mds-proxy

  "c:avatars-test-proxy":
    - match: grain
    - avatars-test-proxy
  "c:avatars-proxy":
    - match: grain
    - avatars-proxy
  "c:avatars-storage":
    - match: grain
    - avatars-storage
  "c:avatars-recovery":
    - match: grain

  "c:cdn-storage-regional":
    - match: grain
    - cdn-storage-regional

# RESIZER
  "c:resizer-test-front":
    - match: grain
    - resizer-test-front
  "resize.load.yandex.net":
    - templates.unistat-lua
    - units.mds-access-log-tskv
    - units.resizer-secrets
  "c:resizer-front":
    - match: grain
    - parsers
    - templates.push-client
    - resizer-front
    - units.iface-ip-conf
    - units.resizer-secrets
    - units.mds-access-log-tskv
  "c:resizer-intra-front":
    - match: grain
    - resizer-intra-front
    - units.iface-ip-conf
    - units.resizer-secrets
    - units.mds-access-log-tskv
  "c:resizer-front-prestable":
    - match: grain
    - parsers
    - templates.push-client
    - resizer-front
    - units.iface-ip-conf
    - units.resizer-secrets
    - units.mds-access-log-tskv

# APE_PROD
  'conductor:group:ape-all':
    - match: grain
    - sys
  'c:ape-storage':
    - match: grain
    - storage
  'conductor:group:ape-front-12-stable':
    - match: grain
    - front-12
  'conductor:group:ape-cloud':
    - match: grain
    - cloud
  'conductor:group:ape-cloud-12-stable':
    - match: grain
    - cloud-12
  'conductor:group:ape-cloud-12-prestable':
    - match: grain
    - cloud-12-prestable
  'conductor:group:ape-front-12-prestable':
    - match: grain
    - front-12-prestable
  'c:ape-mongo':
    - match: grain
    - mongo
  'conductor:group:ape-registry-stable':
    - match: grain
    - registry
  'conductor:group:ape-registry-v2':
    - match: grain
    - registry-v2
  'c:ape-proxy':
    - match: grain
    - proxy
    - templates.media-tskv.parsers
  'c:ape-registry-v2':
    - match: grain
    - registry-v2
    - templates.media-tskv.parsers
  'c:ape-elastic':
    - match: grain
    - elastic
  'c:ape-grafana':
    - match: grain
    - grafana
  'c:ape-coreface':
    - match: grain
    - ape-coreface
  'conductor:group:ape-zk-discovery':
    - match: grain
    - ape-zk-discovery
  'conductor:group:ape-zk-dark':
    - match: grain
    - ape-zk-dark
  'c:ape-infra-build':
    - match: grain
    - sandbox-agents
  'conductor:group:ape-state-offload':
    - match: grain
    - ape-state-offload
  'conductor:group:ape-state-storage':
    - match: grain
    - ape-state-storage
  'c:ape-orcs':
    - match: grain
    - orchestrator-server
  'c:ape-load-cloud':
    - match: grain
    - ape-load-cloud
  'c:ape-load-front':
    - match: grain
    - ape-load-front
  'c:ape-pipeline':
    - match: grain
    - ape-pipeline

# APE
  'conductor:groups:ape-test-front-12':
    - match: grain
    - ape-test-front-12
  'c:ape-test-cloud-12':
    - match: grain
    - ape-test-cloud-12
  'c:ape-test-storage':
    - match: grain
    - ape-test-storage
  'c:ape-test-coreface':
    - match: grain
    - ape-test-coreface
  'conductor:groups:ape-test-zk-alldc':
    - match: grain
    - ape-test-zk
  'c:ape-test-kibana':
    - match: grain
    - kibana
  'conductor:groups:ape-test-state-storage':
    - match: grain
    - ape-test-state-storage
  'conductor:groups:ape-test-state-offload':
    - match: grain
    - ape-test-state-offload
