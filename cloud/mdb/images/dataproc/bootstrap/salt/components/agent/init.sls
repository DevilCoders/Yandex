{% set path = '/opt/yandex/dataproc-agent' %}

{% if salt['ydputils.check_roles'](['masternode']) %}

{{ path }}/bin/dataproc-agent:
    dataproc-agent.updated

{% if not salt['pillar.get']('data:properties:dataproc:disable_agent', False) and not salt['ydputils.is_presetup']() %}
dataproc-agent-user:
    user.present:
        - name: dataproc-agent
{% if salt['grains.get']('saltversioninfo')[0] >= 3001 %}
        - usergroup: True
{% else %}
        - gid_from_name: True
{% endif %}
        - system: true
        - empty_password: false
        - groups:
            - hadoop

{{ path }}/dataproc.pem:
    file.managed:
{% if salt['pillar.get']('data:dataproc-manager:cert') %}
        - contents_pillar: data:dataproc-manager:cert
{% else %}
        - source: /usr/local/share/ca-certificates/yandex-cloud-ca.crt
{% endif %}
        - mode: 600
        - user: dataproc-agent

/var/log/yandex/dataproc-agent/jobs:
    file.directory:
        - user: dataproc-agent
        - group: dataproc-agent
        - makedirs: True
        - mode: 755

/etc/cron.d/clean_job_logs:
    file.managed:
        - user: root
        - source: salt://{{ slspath }}/conf/clean_job_logs

{{ path }}/etc/dataproc-agent.yaml:
    file.managed:
        - template: jinja
        - makedirs: true
        - user: dataproc-agent
        - source: salt://{{ slspath }}/conf/dataproc-agent.yaml

/etc/logrotate.d/dataproc-agent:
    file.managed:
        - source: salt://{{ slspath }}/conf/logrotate.conf
        - mode: 644
        - makedirs: True

dataproc-agent.service:
  file.managed:
    - name: /lib/systemd/system/dataproc-agent.service
    - template: jinja
    - source: salt://{{ slspath }}/conf/dataproc-agent.service

/etc/sudoers.d/90-yandex-dataproc-agent:
  file.managed:
    - source: salt://{{ slspath }}/conf/90-yandex-dataproc-agent-sudoers

dataproc-agent-systemd-reload:
  cmd.run:
    - name: systemctl --system daemon-reload
    - onchanges:
      - file: dataproc-agent.service

dataproc-agent-service:
  service.running:
    - enable: true
    - name: dataproc-agent
    - require:
      - user: dataproc-agent
      - file: dataproc-agent.service
      - file: /var/log/yandex/dataproc-agent/jobs
      - file: {{ path }}/etc/dataproc-agent.yaml
      - file: /etc/sudoers.d/90-yandex-dataproc-agent
      - dataproc-agent: {{ path }}/bin/dataproc-agent
      # start agent when python environment is ready
      # otherwise some python tasks may fail without dependencies
      - pip: pip-environment
      - conda: conda-environment
{% if 'hbase' in salt['pillar.get']('data:services', []) %}
      - dataproc: hbase-rs-available
{% endif %}
{% if 'hdfs' in salt['pillar.get']('data:services', []) %}
      - dataproc: hdfs-available
{% endif %}
{% if 'oozie' in salt['pillar.get']('data:services', []) %}
      - dataproc: oozie-available
{% endif %}
{% if 'zeppelin' in salt['pillar.get']('data:services', []) %}
      - dataproc: zeppelin-available
{% endif %}
{% if 'livy' in salt['pillar.get']('data:services', []) %}
      - dataproc: livy-available
{% endif %}
    - watch:
      - dataproc-agent: {{ path }}/bin/dataproc-agent
    - onchanges_in:
       - cmd: dataproc-agent-systemd-reload

{% endif %}
{% endif %}
