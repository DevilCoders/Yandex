pt-stalk:
  config:
    threshold: 150
    cycles: 3
    disk-bytes-free: '2G'
    sleep: 20
    plugin: '/etc/yandex/percona-toolkit/pt-stalk/plugin'
    defaults-file: '/root/.my.cnf'
    retention-time: 14
    dest: '/var/lib/pt-stalk/'
  kill: False

