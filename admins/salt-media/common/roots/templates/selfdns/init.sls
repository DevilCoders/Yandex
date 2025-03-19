{% from slspath + "/map.jinja" import selfdns with context %}

selfdns_packages:
  pkg.installed:
    - pkgs:
      - yandex-selfdns-client
      {% if selfdns.plugin.package %}
      - {{ selfdns.plugin.package }}
      {% endif %}

/etc/yandex/selfdns-client/media.conf:
  file.managed:
    {% if selfdns.config %}
    - source: salt://{{ selfdns.config }}
    {% else %}
    - source: salt://{{ slspath }}/files/etc/yandex/selfdns-client/media.conf
    {% endif %}
    - user: root
    - group: root
    - mode: 0644
    - makedirs: True
    - template: jinja
    - context:
      token: {{ salt['pillar.get'](selfdns.token_from_pillar) if selfdns.token_from_pillar else selfdns.token }}
      plugin: {{ selfdns.plugin.name }}

/etc/cron.d/selfdns-client:
  file.managed:
    {% if selfdns.cron %}
    - source: salt://{{ selfdns.cron }}
    {% else %}
    - source: salt://{{ slspath }}/files/etc/cron.d/selfdns-client
    {% endif %}
    - user: root
    - group: root
    - mode: 0644
    - makedirs: True
