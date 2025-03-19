{% from slspath + "/map.jinja" import config_merged_with_user_pillars with context %}

media-graphite-sender-template:
  pkg.installed:
    - name: media-graphite-sender
  service.running:
    - name: media-graphite-sender
    - watch:
      - file: /etc/graphite-sender/config.yaml
  file.managed:
    - name: /etc/graphite-sender/config.yaml
    - require:
      - pkg: media-graphite-sender
    - user: media-graphite-sender
    - group: media-graphite-sender
    - makedirs: True
    - mode: 644
    - contents: |
        {{config_merged_with_user_pillars|yaml(False)|indent(8)}}

/etc/yandex-pkgver-ignore.d/yabs-graphtie-sender:
  file.managed:
    - makedirs: True
    - contents: yabs-graphite-sender

# cleanup file from config-media-graphite package
/etc/monrun/conf.d/monrun_graphite.conf:
  file.absent

{% set reserved_str = salt.sysctl.get("net.ipv4.ip_local_reserved_ports") %}
{% set reserved = reserved_str.split(",") %}
{% set sender_port = config_merged_with_user_pillars.sender.port|string %}
{% if sender_port == "42000" and "42000" not in reserved %}
{% if reserved_str %}
  {% do reserved.append("42000") %}
{% else %}
  {% set reserved = ["42000"] %}
{% endif %}
net.ipv4.ip_local_reserved_ports:
  sysctl.present:
    - value: {{reserved|join(",")}}
{% endif %}
