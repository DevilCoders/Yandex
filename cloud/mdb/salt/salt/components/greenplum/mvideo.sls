{% from "components/greenplum/map.jinja" import gpdbvars with context %}

{% set mvideo_dir = '/usr/local/mvideo'   %}
{% set mvideo_cron_file = '/etc/cron.d/mvideo' %}

{{ mvideo_dir }}:
  file.directory:
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - mode: 0755
    - makedirs: True

{{ mvideo_dir }}/idleconn_kill.sh:
  file.managed:
    - source: salt://{{ slspath }}/conf/mvideo/idleconn_kill.sh
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - makedirs: True
    - mode: 0755
    - template: jinja
    - require:
      - file: {{ mvideo_dir }}

{{ mvideo_dir }}/kill_idle_in_transaction.sh:
  file.managed:
    - source: salt://{{ slspath }}/conf/mvideo/kill_idle_in_transaction.sh
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - makedirs: True
    - mode: 0755
    - template: jinja
    - require:
      - file: {{ mvideo_dir }}

{{ mvideo_dir }}/long_query_time_kill.sh:
  file.managed:
    - source: salt://{{ slspath }}/conf/mvideo/long_query_time_kill.sh
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - makedirs: True
    - mode: 0755
    - template: jinja
    - require:
      - file: {{ mvideo_dir }}

{{ mvideo_dir }}/spill_file_history.sh:
  file.managed:
    - source: salt://{{ slspath }}/conf/mvideo/spill_file_history.sh
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - makedirs: True
    - mode: 0755
    - template: jinja
    - require:
      - file: {{ mvideo_dir }}

{{ mvideo_cron_file }}:
  file.managed:
    - source: salt://{{ slspath }}/conf/mvideo/mvideo_crontab
    - user: root
    - group: root
    - mode: 0644
    - template: jinja
    - context:
      mvideo_dir: {{ mvideo_dir }} 

/etc/logrotate.d/mvideo:
  file.managed:
    - source: salt://{{ slspath }}/conf/mvideo/mvideo.logrotate
    - template: jinja
    - user: root
    - group: root
    - mode: 0644
