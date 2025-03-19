{% if salt['ydputils.check_roles'](['masternode']) and not salt['ydputils.is_presetup']() %}
{% set path = '/opt/yandex/dataproc-agent' %}

{{ path }}/bin/dataproc-agent:
    dataproc-agent.updated

{% if not salt['pillar.get']('data:flags:disable_agent', False) %}
{% if salt['pillar.get']('data:dataproc-manager:cert') %}
{{ path }}/dataproc.pem:
    file.managed:
        - contents_pillar: data:dataproc-manager:cert
        - mode: 600

{% else %}
{{ path }}/dataproc.pem:
    file.managed:
        - source: /usr/local/share/ca-certificates/yandex-cloud-ca.crt
{% endif %}

/etc/supervisor/conf.d/dataproc-agent.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/supervisor.conf

/var/log/yandex/dataproc-agent/jobs:
    file.directory:
        - makedirs: True
        - mode: 755

/etc/cron.d/clean_job_logs:
    file.managed:
        - user: root
        - source: salt://{{ slspath }}/conf/clean_job_logs

{{ path }}/etc/dataproc-agent.yaml:
    file.managed:
        - template: jinja
        - makedirs: True
        - source: salt://{{ slspath }}/conf/dataproc-agent.yaml

dataproc-agent-supervised:
    supervisord.running:
        - name: dataproc-agent
        - update: True
        - require:
            - dataproc-agent: {{ path }}/bin/dataproc-agent
            - file: /opt/yandex/dataproc-agent/etc/dataproc-agent.yaml
            - file: /etc/supervisor/conf.d/dataproc-agent.conf
            - file: /var/log/yandex/dataproc-agent/jobs
        - watch:
            - file: /opt/yandex/dataproc-agent/etc/dataproc-agent.yaml
            - file: /etc/supervisor/conf.d/dataproc-agent.conf
{% endif %}
{% endif %}
