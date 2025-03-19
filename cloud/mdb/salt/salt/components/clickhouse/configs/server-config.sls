clickhouse-server-config-req:
    test.nop

clickhouse-server-config-ready:
    test.nop:
        - on_changes_in:
            - mdb_clickhouse: clickhouse-reload-config

/etc/clickhouse-server/config.xml:
    file.managed:
        - template: jinja
        - source: salt://components/clickhouse/etc/clickhouse-server/config.xml
        - user: root
        - group: clickhouse
        - mode: 644
        - makedirs: True
        - require:
            - test: clickhouse-server-config-req
        - watch_in:
            - test: clickhouse-server-config-ready

{% if salt.mdb_clickhouse.has_separated_keeper() %}
/etc/clickhouse-keeper/config.xml:
    file.managed:
        - template: jinja
        - source: salt://components/clickhouse/etc/clickhouse-keeper/config.xml
        - user: root
        - group: clickhouse
        - mode: 644
        - makedirs: True
        - require:
            - test: clickhouse-server-config-req
        - watch_in:
            - test: clickhouse-server-config-ready
{% endif %}

/etc/clickhouse-server/config.d:
    file.directory:
        - user: root
        - group: clickhouse
        - makedirs: True
        - mode: 750
        - require:
            - test: clickhouse-server-config-req
        - watch_in:
            - test: clickhouse-server-config-ready

{% set ca_path = salt.mdb_clickhouse.ca_path() %}
{% set kafka_settings = salt.mdb_clickhouse.kafka_settings() %}

{% if kafka_settings.get('ca_cert') %}
{{ salt.mdb_clickhouse.kafka_ca_path() }}:
    file.managed:
        - user: root
        - group: clickhouse
        - mode: 640
        - makedirs: True
        - contents_pillar: data:clickhouse:config:kafka:ca_cert
{% endif %}

{% for topic in salt.mdb_clickhouse.kafka_topics() %}
{%     if topic.settings.get('ca_cert') %}
{{ salt.mdb_clickhouse.kafka_topic_ca_path(topic.name) }}:
    file.managed:
        - user: root
        - group: clickhouse
        - mode: 640
        - makedirs: True
        - contents: "{{ topic.settings.get('ca_cert') }}"

{%     endif %}
{% endfor %}
