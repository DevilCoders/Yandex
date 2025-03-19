{{saltenv|default('base')}}:
  '*':
    - units.salt-state
    - units.secure
    - units.new-juggler-client
  'c:elliptics-test-all':
    - match: grain
    - units.new-juggler-client
  'c:elliptics-test-proxies':
    - match: grain_pcre
    - elliptics-test-proxies
  "c:elliptics-test-collector":
    - match: grain
    - elliptics-test-collector
  'c:elliptics-test-cloud':
    - match: grain
    - elliptics-test-cloud
  'c:elliptics-test-storage':
    - match: grain
    - elliptics-test-storage
  "c:elliptics-test-dom0-lxd":
    - match: grain_pcre
    - elliptics-test-dom0-lxd
  'c:elliptics-test-rbtorrent':
    - match: grain
    - elliptics-test-rbtorrent

  "c:elliptics-all":
    - match: grain

  "c:^elliptics-collector(-unstable)?$":
    - match: grain_pcre
    - elliptics-collector
  "c:elliptics-valve":
    - match: grain
    - elliptics-valve
  "c:elliptics-dom0-lxd":
    - match: grain_pcre
    - elliptics-dom0-lxd
  'c:elliptics-proxy':
    - match: grain
    - elliptics-proxy
  'c:elliptics-cloud':
    - match: grain
    - elliptics-cloud
  'c:elliptics-s3cleanup':
    - match: grain
    - elliptics-s3cleanup
  'c:elliptics-storage':
    - match: grain
    - elliptics-storage
  'c:elliptics-l3mon':
    - match: grain
    - elliptics-l3mon
  'c:elliptics-logshatter':
    - match: grain
    - elliptics-logshatter
  'c:elliptics-clickhouse':
    - match: grain
    - elliptics-clickhouse
  'c:elliptics-strm-log':
    - match: grain
    - elliptics-strm-log
  'c:elliptics-rbtorrent':
    - match: grain
    - elliptics-rbtorrent
  'c:elliptics-walle-cms':
    - match: grain
    - elliptics-walle-cms

  # storage/pillar/*
  "c:^storage(-test)?-mulcagate$":
    - match: grain_pcre
    - mulcagate

  'c:elliptics_ycloud-storage':
    - match: grain
    - elliptics_ycloud-storage

  'c:elliptics-logstore-new':
    - match: grain
    - elliptics-logstore

  "c:elliptics-(test-)?yarl":
    - match: grain_pcre
    - elliptics-yarl

  'c:elliptics-stable-salt':
    - match: grain
    - elliptics-stable-salt

  'c:elliptics-test-lepton':
    - match: grain
    - elliptics-test-lepton

  'c:elliptics-lepton':
    - match: grain
    - elliptics-lepton

  'c:elliptics-sentry':
    - match: grain
    - elliptics-sentry

  'c:nocdev-dom0-lxd':
    - match: grain_pcre
    - nocdev-dom0-lxd
  'c:nocdev-decapinger':
    - match: grain
    - nocdev-decapinger
  'c:nocdev-ck':
    - match: grain
    - nocdev-ck
  'c:nocdev-test-ck':
    - match: grain
    - nocdev-test-ck
  'c:nocdev-4k':
    - match: grain
    - nocdev-4k
  'c:nocdev-test-4k':
    - match: grain
    - nocdev-test-4k
  'c:nocdev-test-packfw':
    - match: grain
    - nocdev-test-packfw
  'c:nocdev-packfw':
    - match: grain
    - nocdev-packfw
  'c:nocdev-vcs':
    - match: grain
    - nocdev-vcs

  'conductor:group:deploy':
    - match: grain
    - deploy-test-proxies

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
  'c:avatars-test-all':
    - match: grain
    - units.new-juggler-client
  'c:avatars-mds-test-proxy':
    - match: grain
    - avatars-mds-test-proxy
  "c:avatars-all":
    - match: grain
  'c:avatars-mds-proxy':
    - match: grain
    - avatars-mds-proxy
  'c:avatars-test-proxy':
    - match: grain
    - avatars-test-proxy
  'c:avatars-proxy':
    - match: grain
    - avatars-proxy
  'c:avatars-storage':
    - match: grain
    - avatars-storage
  'c:avatars-recovery':
    - match: grain
  'c:cdn-storage-regional':
    - match: grain
    - cdn-storage-regional

# RESIZER
  'c:resizer-test-front':
    - match: grain
    - resizer-test-front
  'resize.load.yandex.net':
    - units.new-juggler-client
    - units.resizer-secrets
  "c:resizer-front":
    - match: grain
    - parsers
    - resizer-front
    - push-client.resizer-front
    - ssl
  "c:resizer-intra-front":
    - match: grain
    - ssl

# APE_PROD
  'c:ape-cloud-12':
    - match: grain
    - ape-cloud-12
  'c:ape-cloud-12-prestable':
    - match: grain
    - ape-cloud-12-prestable
  'c:ape-front-12':
    - match: grain
    - ape-front-12
  'c:ape-front-12-prestable':
    - match: grain
    - ape-front-12

# APE
  'conductor:groups:ape-test-front-12':
    - match: grain
    - ape-test-front-12
  'c:ape-test-cloud-12':
    - match: grain
    - ape-test-cloud-12
