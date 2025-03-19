# browse history https://a.yandex-team.ru/arc/history/trunk/arcadia/cloud/mdb/dbaas_worker/
dbaas-worker-pkgs:
    pkg.installed:
        - pkgs:
            - dbaas-worker: '1.9774960'

/etc/dbaas-worker.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/dbaas-worker.conf' }}
        - mode: '0640'
        - user: root
        - group: web-api
        - require:
            - pkg: dbaas-worker-pkgs

/etc/cron.d/dbaas-worker-agent-cleanup:
    file.managed:
        - source: salt://{{ slspath }}/conf/dbaas-worker-agent-cleanup.cron

dbaas-worker-config-test:
    cmd.wait:
        - name: /opt/yandex/dbaas-worker/bin/dbaas-worker -c /etc/dbaas-worker.conf --test
        - require:
            - file: /etc/dbaas-worker.conf
        - watch:
            - pkg: dbaas-worker-pkgs
            - file: /etc/dbaas-worker.conf

{% if salt['pillar.get']('data:disable_worker', False) %}
dbaas-worker-supervised:
    supervisord.dead:
        - name: dbaas-worker

/etc/supervisor/conf.d/dbaas-worker.conf:
    file.absent:
        - require:
            - supervisord: dbaas-worker
{% else %}
/etc/supervisor/conf.d/dbaas-worker.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/dbaas-worker-supervisor.conf
        - require:
            - pkg: dbaas-worker-pkgs

dbaas-worker-supervised:
    supervisord.running:
        - name: dbaas-worker
        - update: True
        - require:
            - cmd: dbaas-worker-config-test
            - service: supervisor-service
            - user: web-api-user
        - watch:
            - pkg: dbaas-worker-pkgs
            - file: /etc/dbaas-worker.conf
            - file: /etc/supervisor/conf.d/dbaas-worker.conf
{% endif %}

/opt/yandex/CA.pem:
    file.symlink:
        - target: /opt/yandex/allCAs.pem
        - force: True
        - require:
            - file: /opt/yandex/allCAs.pem

{{ salt['pillar.get']('data:dbaas_worker:compute:ssh_private_key', '/opt/yandex/dbaas-worker/ssh_key') }}:
    file.managed:
        - mode: '0600'
        - user: web-api
        - group: web-api
        - encoding: utf-8
        - contents_pillar: 'data:dbaas_worker:ssh_private_key'

/etc/logrotate.d/dbaas-worker:
    file.managed:
        - source: salt://{{ slspath }}/conf/dbaas-worker-logrotate.conf
        - mode: 644
        - makedirs: True

include:
    - components.monrun2.dbaas-worker
