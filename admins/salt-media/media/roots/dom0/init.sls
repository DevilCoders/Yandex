include:
  - common.hw_watcher

/etc/lxctl/conf.d/01-lxcfs.conf:
  file.managed:
    - source: salt://dom0/01-lxcfs.conf
    - user: root
    - group: root
    - mode: 755
    - makedirs: True

/usr/share/lxcfs/lxc.yamount.hook:
  file.managed:
    - source: salt://dom0/lxc.yamount.hook
    - user: root
    - group: root
    - mode: 755
    - makedirs: True

{% if grains['oscodename'] == 'trusty' %}
/etc/apt/sources.list.d/trusty.list:
  file.managed:
    - user: root
    - group: root
    - mode: 755
      contents: |
        deb http://archive.ubuntu.com/ubuntu trusty-backports main restricted universe multiverse
        deb http://yandex-trusty.dist.yandex.ru/yandex-trusty stable/all/
        deb http://yandex-trusty.dist.yandex.ru/yandex-trusty stable/$(ARCH)/
{% endif %}

lxcfs-pkg:
  pkg.latest:
    - pkgs:
      - lxcfs
    - require:
      - file: /etc/apt/sources.list.d/trusty.list

/etc/monitoring/la.conf:
  file.managed:
    - user: root
    - group: root
      contents: |
        100

/etc/yandex/disk-util-check/config.yaml:
  file.managed:
    - contents: |
        crit_val: 0
        sample_count: 0
        sample_time: 0
        enabled: False

/etc/monitoring/network_load.conf:
  file.managed:
    - contents: |
        CRIT_LIMIT=99
        WARN_LIMIT=90
        TIME=60
        NETWORK_STATS=recv

/etc/monrun/conf.d/network_load.conf:
  file.managed:
    - contents: |
        [network_load]
        execution_interval=300
        execution_timeout=270
        command=/usr/bin/network_load.sh
