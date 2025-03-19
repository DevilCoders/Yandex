{{saltenv|default('base')}}:
  '*':
    - common-secrets
  'kernel:Linux':  # all but freebsd
    - match: grain
    - juggler-checks
  'virtual:physical':
    - match: grain
    - units.hw_watcher

  'c:noc-test-fw':
    - match: grain
    - units.repos
    - noc-test-fw

  'c:nocdev-*':
    - match: grain
    - units.salt-state
    - units.secure
    - units.juggler-client
    - units.repos

  'c:nocdev-valve':
    - match: grain
    - nocdev-valve
  'c:nocdev-test-macro':
    - match: grain
    - nocdev-test-macro
  'c:nocdev-macro':
    - match: grain
    - nocdev-macro
  'c:nocdev-netegress':
    - match: grain
    - nocdev-netegress
  'c:nocdev-test-valve':
    - match: grain
    - nocdev-test-valve

  'c:nocdev-puncher':
    - match: grain
    - nocdev-puncher
  'c:nocdev-prestable-rt':
    - match: grain
    - nocdev-prestable-rt
  'G@c:nocdev-rt or E@noc-(myt|sas)\.yandex\.net':
    - match: compound
    - nocdev-rt
  'c:noc_rt':
    - match: grain
    - nocdev-rt-w-mysql
    - units.salt-state
    - units.secure
    - units.juggler-client
    - units.repos
  'c:nocdev-nocauth':
    - match: grain
    - nocdev-nocauth
  'c:nocdev-test-cvs':
    - match: grain
    - nocdev-test-cvs
  'c:nocdev-cvs':
    - match: grain
    - nocdev-cvs
  'c:nocdev-egress':
    - match: grain
    - nocdev-egress

  'c:nocdev-slbcloghandler':
    - match: grain
    - nocdev-slbcloghandler
  'c:nocdev-hbf-validator':
    - match: grain
    - nocdev-hbf-validator
  'c:nocdev-hbf-prestable':
    - match: grain
    - nocdev-hbf-prestable
  'c:nocdev-hbf-myt':
    - match: grain
    - nocdev-hbf-myt
  'c:nocdev-hbf-vla':
    - match: grain
    - nocdev-hbf-vla
  'c:nocdev-hbf-vlx':
    - match: grain
    - nocdev-hbf-vlx
  'c:nocdev-hbf-man':
    - match: grain
    - nocdev-hbf-man
  'c:nocdev-hbf-sas':
    - match: grain
    - nocdev-hbf-sas
  'c:nocdev-hbf-iva':
    - match: grain
    - nocdev-hbf-iva

  'c:nocdev-test-hbf':
    - match: grain
    - nocdev-test-hbf
  'c:nocdev-test-hbf-validator':
    - match: grain
    - nocdev-test-hbf-validator


  'c:nocdev-dom0-lxd':
    - match: grain
    - nocdev-dom0-lxd
  'c:nocdev-test-dom0-lxd':
    - match: grain
    - nocdev-test-dom0-lxd
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
  'c:nocdev-tardis':
    - match: grain
    - nocdev-tardis
  'c:nocdev-packfw':
    - match: grain
    - nocdev-packfw
  'c:nocdev-vcs':
    - match: grain
    - nocdev-vcs
  'c:nocdev-decapinger':
    - match: grain
    - nocdev-decapinger
  'c:nocdev-mongodb':
    - match: grain
    - nocdev-mongodb
  'c:nocdev-pahom':
    - match: grain
    - nocdev-pahom
  'c:nocdev-susanin':
    - match: grain
    - nocdev-susanin
  'c:nocdev-prestable-susanin':
    - match: grain
    - nocdev-susanin
  'c:nocdev-test-susanin':
    - match: grain
    - nocdev-test-susanin
  'c:nocdev-syslog':
    - match: grain
    - nocdev-syslog
  'c:nocdev-mysql':
    - match: grain
    - nocdev-mysql
  'c:nocdev-l3mon':
    - match: grain
    - nocdev-l3mon
  'c:nocdev-test-mysql':
    - match: grain
    - nocdev-test-mysql
  'c:nocdev-prestable-mysql':
    - match: grain
    - nocdev-prestable-mysql
  'c:nocdev-ufm':
    - match: grain
    - noc-osm
  'c:nocdev-whois':
    - match: grain
    - nocdev-whois

  'c:noc-fw':
    - match: grain
    - noc-fw
  'c:noc-decap':
    - match: grain
    - noc-decap
  'c:nocdev-test-grads':
    - match: grain
    - nocdev-grad-server
  'c:grad_vs':
    - match: grain
    - nocdev-grad-server
  'c:nocdev-cmdb':
    - match: grain
    - nocdev-cmdb
  'c:nocdev-test-cmdb':
    - match: grain
    - nocdev-test-cmdb
  'E@\w+-rt-staging\d?.net.yandex.net or P@c:nocdev-(test-)?staging':
    - match: compound
    - nocdev-staging
  'c:nocdev-staging-red':
    - match: grain
    - nocdev-test-dom0-lxd
    - nocdev-staging
  'c:nocdev-metridat-client':
    - match: grain
    - nocdev-metridat-client
  'c:nocdev-metridat-collector':
    - match: grain
    - nocdev-metridat-collector
  'c:nocdev-metridat-aggregator':
    - match: grain
    - nocdev-metridat-aggregator
  'c:nocdev-solomon-proxy':
    - match: grain
    - nocdev-solomon-proxy
  'c:nocdev-matilda':
    - match: grain
    - nocdev-matilda

  'c:nocdev-grads':
    - match: grain
    - nocdev-grad-server

  'c:nocdev-bmp':
    - match: grain
    - nocdev-bmp

  'c:megaping':
    - match: grain
    - megaping

  'c:nocdev-snmptrap':
    - match: grain
    - nocdev-snmptrap

  'c:nocdev-bang':
    - match: grain
    - nocdev-bang

  'c:nocdev-k8s-sas':
    - match: grain
    - nocdev-k8s-sas
  'c:nocdev-nodes-sas':
    - match: grain
    - nocdev-nodes-sas
  'c:nocdev-test-k8s':
    - match: grain
    - nocdev-test-k8s
  'c:nocdev-test-nodes':
    - match: grain
    - nocdev-test-nodes

  'c:nocdev-netmap':
    - match: grain
    - nocdev-netmap

  'c:nocdev-test-netmap':
    - match: grain
    - nocdev-netmap

  'c:nocdev-gfglister':
    - match: grain
    - nocdev-gfglister
