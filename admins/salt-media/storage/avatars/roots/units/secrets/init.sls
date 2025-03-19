{% if pillar['avatars-mds-secrets'] is defined %}
{% for secret, secret_content in pillar['avatars-mds-secrets'].items() %}
/etc/avatars-mds/secrets/{{ secret }}.xml:
  file.managed:
    - contents: {{ secret_content }}
    - user: www-data
    - group: www-data
    - mode: 600
    - makedirs: True
{% endfor %}
{% endif %}
{% if pillar['avatars-mds-secret-files'] is defined %}
{% for secret, secret_content in pillar['avatars-mds-secret-files'].items() %}
/etc/avatars-mds/secrets/{{ secret }}:
  file.managed:
    - contents: {{ secret_content | yaml_encode }}
    - user: www-data
    - group: www-data
    - mode: 600
    - makedirs: True
{% endfor %}
{% endif %}
