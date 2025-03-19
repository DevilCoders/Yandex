{% from "components/mdb-metrics/map.jinja" import mdb_metrics with context %}

mdb-metrics-pkg:
    pkg.installed:
        - pkgs:
            - yandex-mdb-metrics: {{ mdb_metrics['version'] }}
            - libpython3.6
            - libpython3.6-minimal
            - libpython3.6-stdlib
            - python3.6
            - python3.6-minimal
            - python3.6-venv
            - libmpdec2
            - python3-distutils-extra
        - prereq_in:
            - cmd: repositories-ready
        - require_in:
            - service: mdb-metrics-service

/etc/mdb-metrics/conf.d/available:
    file.directory:
        - user: root
        - group: root
        - makedirs: True
        - mode: 755

/etc/mdb-metrics/conf.d/enabled:
    file.directory:
        - user: root
        - group: root
        - makedirs: True
        - mode: 755
        - require_in:
            - service: mdb-metrics-service

{% set send_backwards =  mdb_metrics['is_burst'] and mdb_metrics['cluster_type_supports_send_backwards'] %}
mdb-metrics-config:
    file.managed:
        - name: /etc/mdb-metrics/mdb-metrics.conf
        - template: jinja
        - source: salt://{{ slspath }}/conf/mdb-metrics.conf
        - defaults:
            send_backwards: {{ send_backwards }}
            send_backwards_interval: {{ mdb_metrics['send_backwards_interval'] }}
            send_backwards_times: {{ mdb_metrics['send_backwards_times'] }}
            set_mdbhealth_ttl: {{ send_backwards }}
            mdbhealth_ttl: {{ mdb_metrics['min_burst_interval'] * 2 }}
{% if salt['pillar.get']('yandex:environment', 'dev') == 'prod' %}
            environment: production
{% else %}
            environment: testing
{% endif %}
            yasm_tags_cmd: {{ mdb_metrics['main']['yasm_tags_cmd'] }}
            yasm_tags_db: {{ mdb_metrics['main'].get('yasm_tags_db', False) }}
        - require_in:
            - service: mdb-metrics-service

clean-mdb-metrics-senders-state-files:
    cmd.wait:
        - name: rm -rf {{ salt.mdb_metrics.get_sender_template().format('*') }}
        - watch:
            - file: mdb-metrics-config
            - pkg: mdb-metrics-pkg

mdb-metrics-plugins:
    file.recurse:
        - name: /etc/mdb-metrics/plugins
        - file_mode: '0644'
        - template: jinja
        - source: salt://{{ slspath }}/conf/plugins
        - include_empty: True
        - watch_in:
            - service: mdb-metrics-service
        - require_in:
            - service: mdb-metrics-service

{% if salt['pillar.get']('data:cluster_private_key') %}
cluster-key:
    file.managed:
        - name: /etc/mdb-metrics/cluster_key.pem
        - mode: '0640'
        - template: jinja
        - contents_pillar: data:cluster_private_key
        - watch_in:
            - service: mdb-metrics-service
{% endif %}

mdb-metrics:
    service.enabled:
        - name: mdb-metrics
        - require:
           - service: mdb-metrics-service

include:
    - .service
{% if not salt.dbaas.is_aws() %}
    - components.monrun2.mdb-metrics
{% endif %}

extend:
    mdb-metrics-service:
        service.running:
            - watch:
                - pkg: mdb-metrics-pkg
                - file: mdb-metrics-plugins
                - file: mdb-metrics-config
                - cmd: clean-mdb-metrics-senders-state-files
{% if salt['pillar.get']('data:use_yasmagent', True) and salt['pillar.get']('data:mdb_metrics:use_yasmagent', True) %}
            - require:
                - file: yasmagent-instance-getter
{% endif %}

/etc/cron.d/wd-mdb-metrics:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/wd-mdb-metrics

/lib/systemd/system/mdb-metrics.service:
    file.managed:
        - source: salt://{{ slspath }}/conf/mdb-metrics.service
        - require:
            - pkg: mdb-metrics-pkg
        - require_in:
            - service: mdb-metrics
        - onchanges_in:
            - module: systemd-reload

mdb-metrics-component-cleanup:
    file.absent:
        - names:
            - /etc/mdb-metrics/conf.d/available/ch_parts.conf
            - /etc/mdb-metrics/conf.d/enabled/ch_parts.conf
        - watch_in:
            - service: mdb-metrics-service
