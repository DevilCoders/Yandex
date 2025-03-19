{% from slspath ~ "/map.jinja" import user, group, tls_folder with context %}

{% if salt.mdb_redis.tls_enabled() %}
{{ tls_folder }}:
    file.directory:
        - user: {{ user }}
        - group: {{ group }}
        - mode: 0755
        - makedirs: True
        - require:
            - user: redis-user

{{ tls_folder }}/server.key:
    file.managed:
        - contents_pillar: cert.key
        - user: {{ user }}
        - group: {{ group }}
        - mode: 600
        - require:
            - file: {{ tls_folder }}
        - require_in:
            - file: /usr/local/yandex/start-redis.sh
{% if salt.pillar.get('data:redis:config:cluster-enabled') != 'yes' %}
            - file: /usr/local/yandex/start-sentinel.sh
{% endif %}

{{ tls_folder }}/server.crt:
    file.managed:
        - contents_pillar: cert.crt
        - user: {{ user }}
        - group: {{ group }}
        - mode: 644
        - require:
            - file: {{ tls_folder }}
        - require_in:
            - file: /usr/local/yandex/start-redis.sh
{% if salt.pillar.get('data:redis:config:cluster-enabled') != 'yes' %}
            - file: /usr/local/yandex/start-sentinel.sh
{% endif %}

{{ tls_folder }}/ca.crt:
    file.managed:
        - contents_pillar: cert.ca
        - user: {{ user }}
        - group: {{ group }}
        - mode: 644
        - require:
            - file: {{ tls_folder }}
        - require_in:
            - file: /usr/local/yandex/start-redis.sh
{% if salt.pillar.get('data:redis:config:cluster-enabled') != 'yes' %}
            - file: /usr/local/yandex/start-sentinel.sh
{% endif %}
{% endif %}
