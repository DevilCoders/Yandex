{% set is_logstore_host = salt['cmd.shell']('/usr/bin/lxc ls | /bin/grep logstore | wc -l') %}

/etc/yandex/disk-util-check/config.yaml:
  file.managed:
    - user: root
    - group: root
    - mode: 644
    - makedirs: True
    - contents: |
        crit_val: 99.9
        {% if is_logstore_host != '0' -%}
        sample_time: 30
        sample_count: 4
        {% else -%}
        sample_time: 120 
        sample_count: 1
        {% endif -%}
        enabled: True
        epsilon: 40
        inner_epsilon: 40


