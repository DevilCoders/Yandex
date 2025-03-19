{% set project = 'tv' if 'tv' in grains['fqdn'] else 'kino' %}

/etc/yandex/external-config.conf:
  file.managed:
    - template: jinja
    - user: root
    - group: root
# in dev-env applications may start under developers accounts
{% if grains['yandex-environment'] == 'development'%}
    - mode: 0644
{% else %}
    - mode: 0600
{% endif %}
    - makedirs: True
    - contents: {{ pillar['secrets']['external_config'] | json }}

{% if project == 'tv'%}
/etc/yandex/check_group_pkg_ver_yandex-jdk8.conf:
  file.managed:
    - template: jinja
    - user: root
    - group: root
    - mode: 0640
    - contents: {{ pillar['secrets']['yandex_jdk8_version'] | json }}
{% endif %}
