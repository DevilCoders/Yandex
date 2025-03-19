{% if pillar['resizer-secrets'] is defined %}
{% for secret, secret_content in pillar['resizer-secrets'].items() %}
/etc/fastcgi2/secrets/{{ secret }}.xml:
  file.managed:
    - contents: {{ secret_content }}
    - user: www-data
    - group: www-data
    - mode: 600
    - makedirs: True
{% endfor %}
{% endif %}
