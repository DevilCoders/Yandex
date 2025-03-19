{{saltenv|default('base')}}:
  'kernel:Linux':
    - units.auto.include

  'G@virtual:physical and G@kernel:Linux':
    - match: compound
    - templates.hw_watcher

  'c:nocdev-*':
    - match: grain
    - templates.timezone
    - templates.repos
    - units.common

  'G@c:nocdev-test-* and not G@c:nocdev-test-staging':
    - match: compound
    - units.testing

  'c:nocdev-prestable-*':
    - match: grain
    - units.prestable

  'c:nocdev-test-valve':
    - match: grain
    - nocdev-test-valve
  'c:nocdev-test-zk':
    - match: grain
    - nocdev-test-zk

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

  'c:nocdev-test-hbf':
    - match: grain
    - nocdev-test-hbf
  'c:nocdev-test-hbf-validator':
    - match: grain
    - nocdev-test-hbf-validator

  'c:nocdev-hbf-validator':
    - match: grain
    - nocdev-hbf-validator
  'c:nocdev-hbf-prestable':
    - match: grain
    - nocdev-hbf-prestable
  'c:nocdev-hbf-myt':
    - match: grain
    - nocdev-hbf-myt
  'c:nocdev-hbf-iva':
    - match: grain
    - nocdev-hbf-iva
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
  'c:nocdev-nocauth':
    - match: grain
    - nocdev-nocauth
  'c:nocdev-egress':
    - match: grain
    - nocdev-egress

  'c:nocdev-test-cvs':
    - match: grain
    - nocdev-test-cvs
  'c:nocdev-cvs':
    - match: grain
    - nocdev-cvs


  'c:nocdev-slbcloghandler':
    - match: grain
    - nocdev-slbcloghandler
  'c:nocdev-cumulus':
    - match: grain
    - nocdev-cumulus
  'c:nocdev-tardis':
    - match: grain
    - nocdev-tardis
  'c:nocdev-dom0-lxd':
    - match: grain
    - nocdev-dom0-lxd
  'c:nocdev-test-dom0-lxd':
    - match: grain
    - nocdev-test-dom0-lxd
  'c:nocdev-dom0-lxd-kernel':
    - match: grain
    - nocdev-dom0-lxd-kernel
  'c:nocdev-dom0-lxd-ix':
    - match: grain
    - nocdev-dom0-lxd-ix
  'c:nocdev-dom0-lxd-m9':
    - match: grain
    - nocdev-dom0-lxd-m9
  'c:nocdev-ck':
    - match: grain
    - nocdev-ck
  'c:nocdev-zk':
    - match: grain
    - nocdev-zk
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
  'c:nocdev-decapinger':
    - match: grain
    - nocdev-decapinger
  'c:nocdev-mongodb':
    - match: grain
    - nocdev-mongodb
  'c:nocdev-test-mysql':
    - match: grain
    - units.mysql
    - units.robot-nocdev-mysql
    - nocdev-test-mysql
  'c:nocdev-prestable-mysql':
    - match: grain
    - units.mysql
    - units.robot-nocdev-mysql
    - nocdev-prestable-mysql
  'c:nocdev-mysql':
    - match: grain
    - units.juggler-checks.common
    - units.mysql
  'c:nocdev-puncher':
    - match: grain
    - nocdev-puncher

  'c:nocdev-rt':
    - match: grain
    - nocdev-rt
  'E@noc-(myt|sas)\.yandex\.net':
    - match: compound
    - units.racktables.rsyslog-configs
    - units.racktables.bsd
  'c:nocdev-prestable-rt':
    - match: grain
    - nocdev-prestable-rt

  'c:nocdev-pahom':
    - match: grain
    - nocdev-pahom
  'c:nocdev-syslog':
    - match: grain
    - nocdev-syslog
  'c:nocdev-l3mon':
    - match: grain
    - nocdev-l3mon
  'c:nocdev-rs':
    - match: grain
    - nocdev-rs
  'c:nocdev-salt':
    - match: grain
    - nocdev-salt
  'c:nocdev-susanin':
    - match: grain
    - nocdev-susanin
  'c:nocdev-prestable-susanin':
    - match: grain
    - nocdev-susanin
  'c:nocdev-test-susanin':
    - match: grain
    - nocdev-test-susanin
  'c:nocdev-ufm':
    - match: grain
    - noc-osm

  'c:noc-fw':
    - match: grain
    - noc-fw
  'c:noc-test-fw':
    - match: grain
    - noc-test-fw
    - templates.repos
  'c:noc-prestable-fw':
    - match: grain
    - templates.repos

  'c:noc-decap':
    - match: grain
    - noc-decap

  'c:nocdev-cmdb':
    - match: grain
    - nocdev-cmdb
  'c:nocdev-test-cmdb':
    - match: grain
    - nocdev-cmdb

  'c:nocdev-test-staging':
    - match: grain
    - units.development
    - nocdev-staging

  'c:nocdev-staging':
    - match: grain
    - units.stage
    - nocdev-staging

  'c:nocdev-staging-red':
    - match: grain
    - nocdev-test-dom0-lxd
    - units.stage
    - nocdev-staging

  'c:nocdev-test-grads':
    - match: grain
    - nocdev-test-grads

  'c:grad_vs':
    - match: grain
    - nocdev-grad-server

  'c:nocdev-metridat-client':
    - match: grain
    - nocdev-metridat-client

  'c:nocdev-solomon-proxy':
    - match: grain
    - nocdev-solomon-proxy

  'c:nocdev-metridat-collector':
    - match: grain
    - nocdev-metridat-collector

  'c:nocdev-matilda':
    - match: grain
    - nocdev-matilda

  'c:nocdev-grads':
    - match: grain
    - nocdev-grad-server

  'c:megaping':
    - match: grain
    - megaping

  'c:noc-onmsi':
    - match: grain
    - noc-onmsi

  'c:nocdev-bmp':
    - match: grain
    - nocdev-bmp

  'c:nocdev-whois':
    - match: grain
    - nocdev-whois

  'c:nocdev-snmptrap':
    - match: grain
    - nocdev-snmptrap

  'c:nocdev-bang':
    - match: grain
    - nocdev-bang

  'c:nocdev-metridat-aggregator':
    - match: grain
    - nocdev-metridat-aggregator

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
