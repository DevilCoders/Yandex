{%- from slspath + "/map.jinja" import mm_vars with context -%}

{% if pillar.get('is_cloud', False) %}
/etc/elliptics/mastermind-resizer.conf:
  yafile.managed:
    - source: salt://{{ slspath }}/files/etc/elliptics/mastermind-resizer.conf
    - template: jinja
    - context:
      vars: {{ mm_vars }}

mastermind-resizer:
  monrun.present:
    - command: "monrun-resizer-and-reporter.sh -u ping -p 8082 -l /var/log/mastermind/resizer.log -t 120 -r 1"
    - execution_interval: 300
    - execution_timeout: 180
    - type: mastermind
{% endif %}
