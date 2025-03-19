{%- set unit = 'zcore' -%}
{%- set cgroups = grains['conductor']['groups'] -%}


{%- if 'elliptics-storage' not in cgroups and 'elliptics-test-storage' not in cgroups and 'elliptics_ycloud-storage' not in cgroups %}
/etc/sysctl.d/zcore_pattern.conf:
  file.managed:
    - mode: 644
    - user: root
    - group: root
    - makedirs: True
    - contents: |
        kernel.core_pattern = |/usr/bin/pzstd -8 -p16 -o /var/tmp/core.%e.%i.%p.zst
{% endif -%}

/usr/bin/coredump_monitor.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/coredump_monitor.sh
    - makedirs: True
    - user: root
    - group: root
    - mode: 755

core-to-666:
  file.managed:
    - name: /etc/cron.d/change-chmod-coredump
    - contents: >
        10 * * * * root /bin/bash -c '/usr/bin/timeout -s SIGKILL 10 chmod 666 /var/tmp/core*  2>/dev/null 1>/dev/null' 1>/dev/null 2>/dev/null
    - user: root
    - group: root
    - mode: 755
