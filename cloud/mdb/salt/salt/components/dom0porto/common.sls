{% set porto_version = salt.pillar.get('data:porto_version', '4.18.21') %}

porto-group:
    group.present:
        - name: porto

yandex-porto:
    pkg.installed:
        - version: {{ porto_version }}
        - require:
            - group: porto-group
        - prereq_in:
            - cmd: repositories-ready
        - onchanges_in:
            - module: systemd-reload
    service.running:
        - enable: True
        - reload: True
        - require:
            - pkg: yandex-porto

/etc/init.d/yandex-porto:
    file.absent:
        - require:
            - pkg: yandex-porto
        - require_in:
            - service: yandex-porto

/etc/init/yandex-porto.conf:
    file.absent:
        - require:
            - pkg: yandex-porto
        - require_in:
            - service: yandex-porto

porto-packages:
    pkg.installed:
        - pkgs:
            - python-portopy: {{ porto_version }}
            - python-requests
            - python-protobuf
            - debootstrap
            - yandex-internal-root-ca
            - yandex-netconfig: '0.3-7778882'
            - lldpd   # required by yandex-netconfig MDB-1937
            - ndisc6  # required by yandex-netconfig MDB-1937
            - fio     # required for disk speed measurements
        - prereq_in:
            - cmd: repositories-ready
        - require:
            - group: porto-group
            - pkg: yandex-porto

/usr/local/yandex/porto:
    file.recurse:
        - source: salt://{{ slspath }}/bootstrap
        - user: root
        - group: root
        - dir_mode: 755
        - file_mode: 755
        - template: jinja
        - makedirs: True

{% for img in salt['pillar.get']('data:images') %}
{{ img.get('bootstrap_cmd', '/usr/local/yandex/porto/mdb_dbaas_%s_bionic.sh' % img.short_name) }}:
    file.managed:
        - source: salt://{{ slspath }}/conf/mdb_dbaas_bionic.sh
        - user: root
        - group: root
        - mode: '0755'
        - template: jinja
        - require:
            - /usr/local/yandex/porto
        - context:
            name: {{ img.short_name }}
{% endfor %}

/etc/portod.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/portod.conf
        - mode: 644
        - context:
            porto_version: '{{ porto_version }}'
        - watch_in:
            - service: yandex-porto

/etc/portod.conf.d/cores.conf:
    file.managed:
        - source: salt://{{ slspath }}/conf/portod-cores.conf
        - mode: 644
        - require:
            - pkg: yandex-porto
        - watch_in:
            - service: yandex-porto

/etc/sysctl.d/30-net-porto.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/sysctl-porto.conf
        - mode: 644
    cmd.wait:
        - name: sysctl -p /etc/sysctl.d/30-net-porto.conf
        - watch:
            - file: /etc/sysctl.d/30-net-porto.conf

/etc/sysctl.d/40-postgres.conf:
    file.managed:
        - template: jinja
        - source: salt://components/postgres/conf/sysctl-postgres.conf
        - mode: 644
    cmd.wait:
        - name: sysctl -p /etc/sysctl.d/40-postgres.conf
        - watch:
            - file: /etc/sysctl.d/40-postgres.conf

/usr/local/sbin/mark_orphans.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/mark_orphans.py
        - mode: 755

/usr/local/sbin/remove_orphan.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/remove_orphan.py
        - mode: 755

/etc/rc.local:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/rc.local
        - user: root
        - group: root
        - mode: 0755

{% set kernel_modules = salt['pillar.get']('data:kernel_modules', ['ip_tables', 'ip6_tables']) %}

/etc/modules:
    file.append:
        - text:
{% for module in kernel_modules %}
            - {{ module }}
{% endfor %}

/etc/sudoers.d/monitor_dom0porto:
    file.managed:
        - source: salt://{{ slspath }}/conf/monitor_dom0porto.sudoers
        - mode: 440

/etc/cron.d/dom0hostname:
    file.managed:
        - source: salt://{{ slspath }}/conf/dom0hostname.cron.d
        - mode: 644

/etc/cron.d/remove_old_backups:
    file.managed:
        - source: salt://{{ slspath }}/conf/remove_old_backups.cron.d
        - mode: 644

{% if salt['pillar.get']('data:config:dbm_heartbeat_override', False) %}
/etc/dbm_heartbeat_override.json:
    file.managed:
        - source: salt://{{ slspath }}/conf/heartbeat_override.json
        - template: jinja
        - mode: 600
{% endif %}

/etc/cron.d/heartbeat:
    file.managed:
        - source: salt://{{ slspath }}/conf/heartbeat.cron.d
        - mode: 644

/etc/cron.yandex/heartbeat.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/heartbeat.py
        - template: jinja
        - mode: 700

{% if salt['pillar.get']('data:config:fill_page_cache', False) %}
/etc/cron.d/fill_page_cache:
    file.managed:
        - contents: '0 */3 * * * root sleep $((RANDOM \% 1800)); /usr/sbin/portoctl exec fill_page_cache command="/usr/bin/ionice -c 3 /etc/cron.yandex/fill_page_cache.py" io_limit=100MB 2>&1 | logger -t "fill_page_cache"'
        - mode: 644

/etc/cron.yandex/fill_page_cache.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/fill_page_cache.py
        - template: jinja
        - mode: 700
{% else %}
/etc/cron.yandex/fill_page_cache.py:
    file.absent

/etc/cron.d/fill_page_cache:
    file.absent
{% endif %}

/usr/local/yandex/remove_old_backups.sh:
    file.managed:
        - source: salt://{{ slspath }}/conf/remove_old_backups.sh
        - template: jinja
        - mode: 744

/usr/local/yandex/dom0_mdbuild.sh:
    file.managed:
        - source: salt://{{ slspath }}/conf/dom0_mdbuild.sh
        - mode: 700

build_raw_disks_arrays:
    cmd.run:
        - name: /usr/local/yandex/dom0_mdbuild.sh
        - require:
            - file: /usr/local/yandex/dom0_mdbuild.sh
        - onlyif:
            - /usr/local/yandex/dom0_mdbuild.sh check

/usr/sbin/move_container.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/move_container.py
        - mode: 755

/root/.mdb_move_container.conf:
    file.managed:
        - source: salt://{{ slspath }}/conf/move_container.conf
        - template: jinja
        - mode: 600

/usr/sbin/containers_restart.sh:
    file.managed:
        - source: salt://{{ slspath }}/conf/containers_restart.sh
        - mode: 755

/usr/sbin/containers_fix_systemd.sh:
    file.managed:
        - source: salt://{{ slspath }}/conf/containers_fix_systemd.sh
        - mode: 755
    cmd.wait:
        - require:
            - file: /usr/sbin/containers_fix_systemd.sh
        - watch:
            - service: yandex-porto

/data:
    mount.mounted:
        - device: {{ salt['pillar.get']('data:array_for_data', '/dev/md/2') }}
        - fstype: ext4
        - mkmnt: True
        - opts:
            - defaults
            - noatime
            - errors=remount-ro
        - onlyif:
            - fgrep {{ salt['pillar.get']('data:array_for_data', '/dev/md/2') }} /etc/mdadm/mdadm.conf
        - require_in:
            - pkg: porto-packages

{% set root_device=salt['cmd.shell']('blkid -s UUID -o export $(awk \'$2 == "/" {print $1}\' /proc/mounts) | grep UUID')  %}

root_mounted:
    mount.mounted:
        - name: /
        - device: {{ root_device }}
        - fstype: ext4
        - mkmnt: False
        - opts:
            - errors=remount-ro
        - require_in:
            - pkg: porto-packages

{% if salt['fs.path_exists']('/disks') %}
{% set raw_disks_uuids=salt['cmd.shell']('ls /disks').split('\n') %}
{% for disk_uuid in raw_disks_uuids %}
{% if salt['cmd.shell']('mount | grep ' + disk_uuid) %}

mounted_raw_disk_{{ disk_uuid }}:
    mount.mounted:
        - name: /disks/{{ disk_uuid }}
        - device: UUID={{ disk_uuid }}
        - fstype: ext4
        - mkmnt: True
        - opts:
            - noatime
            - errors=remount-ro
        - require_in:
            - pkg: porto-packages

{% endif %}
{% endfor %}
{% endif %}

set-data-reserved-blocks:
    cmd.run:
        - name: tune2fs -r 8192 $(grep "/data" /proc/mounts | awk '{print $1};')
        - unless:
            - test "$(dumpe2fs -h $(grep "/data" /proc/mounts | awk '{print $1};') 2>/dev/null | grep 'Reserved block count:' | awk '{print $NF};')" = "8192"
        - onlyif:
            - fgrep {{ salt['pillar.get']('data:array_for_data', '/dev/md/2') }} /etc/mdadm/mdadm.conf
        - require:
            - mount: /data

{% set slow_array = salt['pillar.get']('data:array_for_sata', '') %}
{% if slow_array != '' %}
/slow:
    mount.mounted:
        - device: {{ slow_array }}
        - fstype: ext4
        - mkmnt: True
        - opts:
            - defaults
            - noatime
            - errors=remount-ro
        - onlyif:
            - fgrep {{ slow_array }} /etc/mdadm/mdadm.conf
    file.symlink:
        - name: /data/slow
        - target: /slow
        - require:
            - mount: /data
        - require_in:
            - pkg: porto-packages
{% endif %}

/etc/default/acct:
    file.managed:
        - source: salt://{{ slspath }}/conf/acct
        - user: root
        - group: root
        - mode: 0644

python-netconfig-purge:
    pkg.purged:
        - pkgs:
            - python-netconfig

/usr/share/yandex-hbf-agent/rules.d/20-manual.v6:
    file.managed:
        - source: salt://{{ slspath }}/conf/hbf-manual-rules
        - template: jinja
        - user: root
        - group: root
        - mode: 644
        - require:
            - pkg: hbf-agent
        - watch_in:
            - service: yandex-hbf-agent

/usr/share/yandex-hbf-agent/rules.d/42-noinet.v6:
    file.absent:
        - watch_in:
            - service: yandex-hbf-agent
