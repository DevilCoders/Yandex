{% set models              = salt.mdb_clickhouse.ml_models() %}
{% set model_names         = models.keys() | list %}
{% set filter              = salt.pillar.get('target-model') %}
{% set use_service_account = salt.pillar.get('data:service_account_id', None) != None %}

clickhouse-models-req:
    test.nop

clickhouse-models-ready:
    test.nop

/var/lib/clickhouse/models:
    file.directory:
        - user: root
        - group: clickhouse
        - dir_mode: 750
        - require:
            - test: clickhouse-models-req
        - require_in:
            - test: clickhouse-models-ready

{% for model_name, model in models.items() %}
{%     if (not filter) or (model_name == filter) %}
model_{{ model_name }}:
    fs.file_present:
        - name: /var/lib/clickhouse/models/{{ model_name }}.bin
        - s3_cache_path: {{ salt.mdb_clickhouse.s3_cache_path('ml_model', model_name) }}
        - url: {{ model['uri'] }}
        - use_service_account_authorization: {{ use_service_account }}
        - skip_verify: True
        - user: root
        - group: clickhouse
        - mode: 640
        - makedirs: True
        - replace: False
        - require:
            - test: clickhouse-models-req
        - require_in:
            - test: clickhouse-models-ready

model_config_{{ model_name }}:
    fs.file_present:
        - name: /etc/clickhouse-server/models.d/{{ model_name }}.xml
        - contents_function: mdb_clickhouse.render_ml_model_config
        - contents_function_args:
            name: {{ model_name }}
        - user: root
        - group: clickhouse
        - mode: 640
        - makedirs: True
        - require:
            - test: clickhouse-models-req
        - require_in:
            - test: clickhouse-models-ready
{%     endif %}
{% endfor %}

cleanup_models:
    fs.directory_cleanedup:
        - name: /var/lib/clickhouse/models
        - expected: {{ model_names }}
        - suffix: '.bin'
        - require:
            - test: clickhouse-models-req
        - require_in:
            - test: clickhouse-models-ready

cleanup_model_configs:
    fs.directory_cleanedup:
        - name: /etc/clickhouse-server/models.d
        - expected: {{ model_names }}
        - suffix: '.xml'
        - require:
            - test: clickhouse-models-req
        - require_in:
            - test: clickhouse-models-ready

cleanup_preprocessed_model_configs:
    fs.directory_cleanedup:
        - name: /var/lib/clickhouse/preprocessed_configs
        - expected: {{ model_names }}
        - prefix: 'models.d_'
        - suffix: '.xml'
        - require:
            - test: clickhouse-models-req
        - require_in:
            - test: clickhouse-models-ready
