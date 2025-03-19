{% macro tls_check_certificate(crt_path) %}
{% if 'components.monrun2.http-tls' in salt['pillar.get']('data:runlist', []) %}
extend:
    monrun-http-tls-confs:
        file.recurse:
            - context:
                crt_path: {{ crt_path }}

/etc/sudoers.d/monitor_http_tls:
    file.managed:
        - mode: '0640'
        - template: jinja
        - source: salt://components/monrun2/http-tls/sudoers.d/monitor_http_tls
        - include_empty: True
        - watch_in:
            - cmd: monrun-jobs-update
        - context:
            crt_path: {{ crt_path }}
{% endif %}
{% endmacro %}
