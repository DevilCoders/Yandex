include:
  - lxd-dom0-common
  - common.hw_watcher

/etc/yandex-autodetect-root-email-stop:
  file.touch:
    - unless: test -e /etc/yandex-autodetect-root-email-stop

kill_all_pending_updates:
  cmd.run:
    - name: "pkill --ns 1 -f '[/]usr/local/sbin/autodetect_root_email.sh'"
    - onlyif: pgrep --ns 1 -f '[/]usr/local/sbin/autodetect_root_email.sh'
    - require:
      - file: /etc/yandex-autodetect-root-email-stop

/etc/aliases:
  file.replace:
    - pattern: 'media-root@yandex-team.ru'
    - repl: '/dev/null'
    - require:
      - cmd: kill_all_pending_updates

newaliases:
  cmd.run:
    - require:
      - file: /etc/aliases
    - onchanges:
      - /etc/yandex-autodetect-root-email-stop

/etc/yandex-pkgver-ignore.d/ipmimonitoring-metrika-yandex:
  file.managed:
    - contents: |
        ipmimonitoring-metrika-yandex

/usr/bin/lxd-conduct.sh:
  file.managed:
    - mode: 755
    - source: salt://{{slspath}}/lxd-conduct.sh
/usr/bin/add-veth-route:
  file.managed:
    - mode: 755
    - source: salt://{{slspath}}/add-veth-route
/etc/udev/rules.d/50-add-route-to-veth.rules:
  file.managed:
    - mode: 644
    - source: salt://{{slspath}}/50-add-route-to-veth.rules

/etc/monitoring/unispace.conf:
    file.managed:
        - contents: |
            # effectively ignore check in dom0, rely on container's check
            /opt/pool-sys1=0.99
            /opt/pool-ind1=0.99
            /opt/pool-sys2=0.99
            /opt/pool-ind2=0.99
            /opt/pool-mongo=0.99
            /opt/pool-cass=0.99


/etc/yandex/disk-util-check/config.yaml:
  file.managed:
    - contents: |
        crit_val: 0
        sample_count: 0
        sample_time: 0
        enabled: False
    - onlyif: /snap/bin/lxc ls | grep -e mongorestorer -e cass...--music--qa -e main-dbm--music--qa
or_cleanup_/etc/yandex/disk-util-check/config.yaml:
  file.absent:
    - name: /etc/yandex/disk-util-check/config.yaml
    - unless: /snap/bin/lxc ls | grep -e mongorestorer -e cass...--music--qa -e main-dbm--music--qa

net.ipv6.conf.vlan688.proxy_ndp:
  sysctl.present:
    - value: 0

{# раскладывается через common.hw_watcher #}
/etc/hw_watcher/conf.d/music.conf:
  file.absent
