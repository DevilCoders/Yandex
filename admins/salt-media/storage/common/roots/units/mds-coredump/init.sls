{%- set unit = "mds-coredump" -%}
{%- set ignore_pattern = pillar.get('mds-coredump-ignore-pattern', None) -%}

mds-coredump:
  file.managed:
    - name: /usr/bin/mds-coredump.sh
    - makedirs: True
    - source: salt://{{ slspath }}/files/mds-coredump.sh
    - mode: 755
  monrun.present:
    {% if ignore_pattern -%}
    - command: "/usr/bin/mds-coredump.sh -i '{{ ignore_pattern }}'"
    {%- else -%}
    - command: "/usr/bin/mds-coredump.sh"
    {%- endif %}
    - execution_interval: 300
    - execution_timeout: 60
