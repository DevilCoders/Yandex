{% set unit = 'disk-logbackup' %}

{% for file in pillar.get('disk-logbackup-sh') %}
{{file}}:
  yafile.managed:
    - source: salt://{{ slspath }}/files{{ file }}
    - mode: 755
    - user: root
    - group: root
    - makedirs: True
{% endfor %}

{% for file in pillar.get('disk-logbackup-cron') %}
{{file}}:
  yafile.managed:
    - source: salt://{{ slspath }}/files{{ file }}
    - mode: 644
    - user: root
    - group: root
    - makedirs: True
{% endfor %}

{% for file in pillar.get('disk-logbackup-sec') %}
{{file}}:
  yafile.managed:
    - source: salt://{{ slspath }}/files{{ file }}
    - mode: 600
    - user: root
    - group: root
    - makedirs: True
{% endfor %}

downloads_counter_check:
  monrun.present:
    - command: "/usr/bin/downloads_counter_check.sh"
    - execution_interval: 600
    - execution_timeout: 3600
