{% set elasticsearch_version = salt.mdb_opensearch.version() %}

dashboards-packages:
    pkg.installed:
        - pkgs:
            - opensearch-dashboards: 2.1.0

# just for add es-certs group
dashboards-user:
    user.present:
        - name: opensearch-dashboards
        - groups:
            - opensearch-dashboards
            - es-certs
        - require_in:
            - test: dashboards-service-req
        - require:
            - pkg: dashboards-packages
            - group: es-certs

/var/log/dashboards:
    file.directory:
        - user: opensearch-dashboards
        - group: opensearch-dashboards
        - mode: 755
        - makedirs: True
        - require_in:
            - test: dashboards-service-req
        - require:
            - pkg: dashboards-packages

/etc/default/opensearch-dashboards:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/etc/default/opensearch-dashboards
        - makedirs: True
        - mode: 644
        - require:
            - pkg: dashboards-packages
        - require_in:
            - test: dashboards-service-req

/etc/systemd/system/opensearch-dashboards.service:
    file.managed:
        - template: jinja
        - require:
            - pkg: dashboards-packages
        - require_in:
            - test: dashboards-service-req
        - source: salt://{{ slspath }}/conf/opensearch-dashboards.service
        - mode: 644
        - onchanges_in:
            - module: systemd-reload

# restart kibana < 7.11 periodically because of fd leaks
/etc/cron.d/restart-kibana:
{% if salt.pillar.get('data:opensearch:dashboards:enabled', False) and salt.mdb_opensearch.version_lt('7.11') %}
    file.managed:
        - source: salt://{{ slspath }}/conf/restart-kibana.cron
        - template: jinja
        - mode: 644
{% else %}
    file.absent
{% endif %}

include:
    - .dashboards-service

extend:
    dashboards-configuration:
        file.recurse:
            - require:
                - pkg: dashboards-packages
