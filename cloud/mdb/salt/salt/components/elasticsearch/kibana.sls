{% set elasticsearch_version = salt.mdb_elasticsearch.version() %}

# OSS vs Licensed package
{% set package_name = 'kibana' if salt.mdb_elasticsearch.licensed_for('basic') else 'kibana-oss' %}

kibana-packages:
    pkg.installed:
        - pkgs:
            - {{package_name}}: {{ elasticsearch_version }}
            - libnss3
            - fontconfig

# just for add es-certs group
kibana-user:
    user.present:
        - name: kibana
        - groups:
            - kibana
            - es-certs
        - require_in:
            - test: kibana-service-req
        - require:
            - pkg: kibana-packages
            - group: es-certs

/var/log/kibana:
    file.directory:
        - user: kibana
        - group: kibana
        - mode: 755
        - makedirs: True
        - require_in:
            - test: kibana-service-req
        - require:
            - pkg: kibana-packages

/etc/default/kibana:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/kibana.default
        - makedirs: True
        - mode: 644
        - require:
            - pkg: kibana-packages
        - require_in:
            - test: kibana-service-req

/etc/systemd/system/kibana.service:
    file.managed:
        - template: jinja
        - require:
            - pkg: kibana-packages
        - require_in:
            - test: kibana-service-req
        - source: salt://{{ slspath }}/conf/kibana.service
        - mode: 644
        - onchanges_in:
            - module: systemd-reload

/lib/systemd/system/kibana.service:
    file.managed:
        - template: jinja
        - require:
            - pkg: kibana-packages
        - require_in:
            - test: kibana-service-req
        - source: salt://{{ slspath }}/conf/kibana.service
        - mode: 644
        - onchanges_in:
            - module: systemd-reload

# restart kibana < 7.11 periodically because of fd leaks
/etc/cron.d/restart-kibana:
{% if salt.pillar.get('data:elasticsearch:kibana:enabled', False) and salt.mdb_elasticsearch.version_lt('7.11') %}
    file.managed:
        - source: salt://{{ slspath }}/conf/restart-kibana.cron
        - template: jinja
        - mode: 644
{% else %}
    file.absent
{% endif %}

include:
    - .kibana-service

extend:
    /etc/kibana/kibana.yml:
        file.managed:
            - require:
                - pkg: kibana-packages
