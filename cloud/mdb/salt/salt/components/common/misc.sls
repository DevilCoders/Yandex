{% set vtype = salt['pillar.get']('data:dbaas:vtype') %}
/etc/sudoers:
    file.managed:
        - source: salt://components/common/conf/etc/sudoers
        - mode: 440
        - user: root
        - group: root

/usr/local/sbin/errata_apply.sh:
    file.managed:
        - template: jinja
        - source: salt://components/common/conf/errata_apply.sh
        - mode: 755

/etc/cron.d/errata-apply:
{% if salt['pillar.get']('data:errata:cron', True) and not salt.dbaas.is_dataplane() %}
    file.managed:
        - source: salt://{{ slspath }}/conf/etc/cron.d/errata-apply
        - template: jinja
        - mode: 644
{% else %}
    file.absent
{% endif %}

# These files are from procps package
# and they are useless in our environment
# for all hosts
/etc/sysctl.d/10-ptrace.conf:
    file.absent

/etc/sysctl.d/10-network-security.conf:
    file.absent

/etc/sysctl.conf:
    file.managed:
        - template: jinja
        - source: salt://components/common/conf/etc/sysctl.conf
        - mode: 644
        - user: root
        - group: root
    cmd.wait:
        - name: 'sysctl -p /etc/sysctl.conf && if ls /etc/sysctl.d/*.conf 2>/dev/null; then sysctl -p /etc/sysctl.d/*.conf; fi'
        - require:
            - file: /etc/sysctl.d/10-ptrace.conf
            - file: /etc/sysctl.d/10-network-security.conf
        - watch:
            - file: /etc/sysctl.conf

/etc/ssh/ssh_config:
    file.managed:
        - source: salt://components/common/conf/etc/ssh/ssh_config
        - mode: 644
        - user: root

/etc/ssh/sshd_config:
    file.managed:
        - source: salt://components/common/conf/etc/ssh/sshd_config
        - mode: 644
        - user: root
        - group: root
        - template: jinja

sshd-service:
    service.running:
        - enable: True
        - name: ssh
        - reload: True
        - require:
            - service: ssh.socket-disable
        - watch:
            - file: /etc/ssh/sshd_config

ssh.socket-disable:
  service.dead:
    - name: ssh.socket
    - enable: False
    - onlyif:
        - test -f /etc/systemd/system/sockets.target.wants/ssh.socket

irqbalance-disable:
  cmd.run:
    - name: systemctl disable irqbalance.service || unlink /etc/systemd/system/multi-user.target.wants/irqbalance.service || true
    - runas: root
    - group: root
    - onlyif:
        - test -f /etc/systemd/system/multi-user.target.wants/irqbalance.service

change-sysctl-apply-order:
    file.managed:
        - name: /etc/init/procps.conf
        - source: salt://components/common/conf/etc/init/procps.conf

root:
    user.present:
        - password: '$6$AmLQ4fU3$/0kqz7HlZRatHL6wCvgoqTed6JdHdNAStxKF2x0fCgRSQGqnIWvpPtazGN.RtgAZpdWvPBUnt/k2aBrUyGInW/'

/var/cores:
    file.directory

/etc/cron.yandex:
    file.directory

{% if salt['grains.get']('osrelease') != '18.04' %}
/etc/profile.d/colorize.sh:
    file.managed:
        - source: salt://components/common/conf/etc/profile.d/colorize.sh
        - mode: 644
{% else %}
/etc/profile.d/colorize.sh:
    file.managed:
        - source: salt://components/common/conf/etc/profile.d/colorize-bionic.sh
        - mode: 644
{% endif %}

/root/.bashrc:
    file.managed:
        - name: /root/.bashrc
        - source: salt://{{ slspath }}/conf/root_bashrc
        - template: jinja

/root/.bashrc.salt.bak:
    file.absent

/etc/init/performance-tuner.conf:
    file.managed:
        - source: salt://components/common/conf/performance-tuner.conf
        - mode: 755
        - user: root
        - group: root

ondemand:
    service.disabled

{% if salt['grains.get']('virtual') == 'physical' %}
/usr/local/yandex/cpu-performance-scaling.sh:
    file.managed:
        - source: salt://components/common/conf/cpu-performance-scaling.sh
        - mode: 755
        - user: root
        - group: root
        - makedirs: True

cpu-performance-governor:
    cmd.run:
        - name: /usr/local/yandex/cpu-performance-scaling.sh
        - runas: root
        - group: root
        - require:
            - file: /usr/local/yandex/cpu-performance-scaling.sh
        - onlyif:
            - ls /sys/devices/system/cpu/cpu0/cpufreq
        - unless:
{% for i in range(0, salt['grains.get']('num_cpus')) %}
            - grep performance /sys/devices/system/cpu/cpu{{ i }}/cpufreq/scaling_governor
{% endfor %}

{% set io_scheduler = salt['pillar.get']('data:io_scheduler', 'bfq') %}

{% set disks = {} %}
{% set points = salt['grains.get']('points_info', {}) %}
{% for point in points %}
{%     for disk_info in points[point]['disks_info'] %}
{%         for key in disk_info %}
{%             do disks.update({key: True}) %}
{%         endfor %}
{%     endfor %}
{% endfor %}
{% for disk in disks %}
set-io-scheduler-{{ disk }}:
    cmd.run:
        - name: echo {{ io_scheduler }} > /sys/block/{{ disk }}/queue/scheduler
        - runas: root
        - group: root
        - onlyif:
            - grep -q {{ io_scheduler }} /sys/block/{{ disk }}/queue/scheduler
        - unless:
            - grep -q '\[{{ io_scheduler }}\]' /sys/block/{{ disk }}/queue/scheduler
{% endfor %}
{% endif %}

/usr/local/yandex:
    file.directory:
        - user: root
        - group: root
        - mode: 0755

/etc/cron.weekly/fstrim:
    file.absent

/etc/cron.d/apt-cache-cleanup:
    file.managed:
        - source: salt://{{ slspath }}/conf/etc/cron.d/apt-cache-cleanup
        - mode: 644

/etc/cron.d/salt-test-highstate:
{% if salt['pillar.get']('data:periodic_test_highstate', False) %}
    file.managed:
        - source: salt://{{ slspath }}/conf/etc/cron.d/salt-test-highstate
        - template: jinja
        - mode: 644
{% else %}
    file.absent
{% endif %}

{% if salt['grains.get']('virtual') == 'lxc' %}
/etc/cron.d/ugly-utmp-fix:
    file.managed:
        - source: salt://{{ slspath }}/conf/etc/cron.d/ugly-utmp-fix
        - mode: 644
{% endif %}

{% if salt['grains.get']('virtual') == 'physical' and salt['pillar.get']('data:disable_bitmap', True) %}
{% for md in salt['grains.get']('mdadm', []) %}
disable_internal_bitmap_{{ md }}:
    cmd.run:
        - name: "mdadm --grow --bitmap=none /dev/{{ md }}"
        - onlyif:
            - "mdadm --detail /dev/{{ md }} | fgrep 'Intent Bitmap : Internal'"
            - "fgrep idle /sys/block/{{ md }}/md/sync_action"
{% endfor %}

{% for point, info in salt['grains.get']('points_info', {}).items() %}
{%     set md=info['device']|replace('/dev/', '') %}
{%     if 'ssd' in info|string() %}
disable_read_ahead_kb_{{ md }}:
    cmd.run:
        - name: "echo 8 > /sys/block/{{ md }}/queue/read_ahead_kb"
        - unless:
            - "grep -q '^8$' /sys/block/{{ md }}/queue/read_ahead_kb"
{%     elif 'nvme' in info|string() %}
disable_read_ahead_kb_{{ md }}:
    cmd.run:
        - name: "echo 0 > /sys/block/{{ md }}/queue/read_ahead_kb"
        - unless:
            - "grep -q '^0$' /sys/block/{{ md }}/queue/read_ahead_kb"
{%     endif %}
{% endfor %}
{% endif %}

{% if salt['grains.get']('virtual') == 'physical' %}
/etc/default/mdadm:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/mdadm
        - user: root
        - group: root
        - mode: 644
{% endif %}

{% if salt.grains.get('virtual', 'physical') != 'lxc' and salt.grains.get('virtual_subtype', None) != 'Docker' %}
{% set thp_file = '/sys/kernel/mm/transparent_hugepage/enabled' %}
disable_thp:
    cmd.run:
        - name: 'echo never > {{ thp_file }}'
        - unless:
            - "fgrep '[never]' {{ thp_file }}"
{% endif %}

{% if salt['grains.get']('virtual', 'physical') != 'lxc' and salt['pillar.get']('data:network_autoconf', False) and salt['grains.get']('virtual_subtype', None) != 'Docker' %}
/etc/network/interfaces:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/interfaces
        - mode: 644
{% endif %}

{% if vtype == 'compute' %}
/etc/dhcp/dhclient.conf:
    file.managed:
        - source: salt://{{ slspath }}/conf/dhclient.conf
        - mode: '0644'
{% endif %}

/etc/cron.yandex/apt_fix_updates.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/apt_fix_updates.py
        - mode: 755
        - require:
            - file: /etc/cron.yandex

/etc/cron.d/apt-fix-updates:
    file.managed:
        - source: salt://{{ slspath }}/conf/etc/cron.d/apt-fix-updates
        - require:
            - file: /etc/cron.yandex/apt_fix_updates.py
