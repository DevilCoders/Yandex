{% if grains['virtual'] == 'physical' %}
{% set coredump_cpuaffinity = salt['cmd.shell']('/usr/bin/python3 /usr/bin/isolate_cpu.py show --type system --string') %}
{% else %}
{% set coredump_cpuaffinity = "0,1" %}
{% endif %}

systemd-coredump:
  yc_pkg.installed:
    - pkgs:
      - systemd-coredump
  file.managed:
    - source: salt://{{ slspath }}/files/coredump.conf
    - name: /etc/systemd/coredump.conf
    - require:
      - yc_pkg: systemd-coredump

/etc/sysctl.d/90-coredumpctl.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/90-coredumpctl.conf
    - require:
      - yc_pkg: systemd-coredump
  cmd.run:
    - name: sysctl -p /etc/sysctl.d/90-coredumpctl.conf
    - require:
      - file: /etc/sysctl.d/90-coredumpctl.conf
    - onchanges:
      - file: /etc/sysctl.d/90-coredumpctl.conf

systemd_coredump_unit_drop-in:
  file.managed:
    - name: /etc/systemd/system/systemd-coredump@.service.d/10-cpu_memory-limit.conf
    - source: salt://{{ slspath }}/files/10-cpu_memory-limit.conf
    - template: jinja
    - makedirs: True
    - defaults:
        coredump_cpuaffinity: {{ coredump_cpuaffinity }}
        coredump_memory_limit_mb: 256
    - require:
      - yc_pkg: systemd-coredump

remove_old_coredump_dir:
  file.absent:
    - name: /var/lib/systemd/coredump
    - unless:
      - test -L /var/lib/systemd/coredump
    - require:
      - yc_pkg: systemd-coredump

/var/crashes:
  file.directory:
    - makedirs: True
    - mode: '0777'

/var/lib/systemd/coredump:
  file.symlink:
    - name: /var/lib/systemd/coredump
    - target: /var/crashes
    - require:
      - file: remove_old_coredump_dir
      - file: /var/crashes

/usr/local/lib/python3.5/dist-packages/coredump.py:
  file.managed:
    - source: salt://{{ slspath }}/files/coredump.py
    - mode: 644
    - makedirs: true
    - require:
      - yc_pkg: python3-systemd

python3-systemd:
  yc_pkg.installed:
    - name: python3-systemd

{% from slspath+"/monitoring.yaml" import monitoring %}
{% include "common/deploy_mon_scripts.sls" %}
