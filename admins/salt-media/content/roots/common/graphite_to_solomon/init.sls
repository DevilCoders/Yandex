{% set yaenv = grains['yandex-environment'] %}
{% set fqdn = grains['fqdn'] %}
{% set group = grains['conductor']['group'] %}
{% set solomon_enable = "true" %}

{% if yaenv in ['production', 'prestable'] %}
{% set cluster_env = 'prod' %}
{% elif yaenv == 'testing' %}
{% set cluster_env = 'test' %}
{% else %}
{% set cluster_env = yaenv %}
{% endif %}

{% if 'tv' in fqdn %}
{% set prj = 'tv' %}
{% elif 'kino' in fqdn %}
{% set prj = 'kino' %}
{% else %}
{% set prj = '' %}
{% endif %}

{% if 'back' in fqdn %}
{% set type = 'back' %}
{% elif 'bko' in fqdn %}
{% set type = 'bko' %}
{% elif 'front' in fqdn %}
{% set type = 'front' %}
{% else %}
{% set type = '' %}
{% set solomon_enable = "false" %}
{% endif %}

{% set cluster_name = [prj, cluster_env, type]|join('-') if type else group %}

yandex-media-graphite-to-solomon-sender:
  pkg.installed

solomon_sender_conf:
  yafile.managed:
    - name: /etc/yandex-media-graphite-to-solomon-sender/solomon_sender_settings.json
    - source: salt://{{slspath}}/files/solomon_sender_settings.json
    - template: jinja
    - context: 
      cluster_name: {{cluster_name}}
      project: {{prj}}
      service: 'common'
      oauth: {{salt['pillar.get']('robot-media-admin:solomon_oauth') | json}}
      solomon_enable: "{{solomon_enable}}"
    - watch_in:
      - cmd: restart_solomon_sender
    - require:
      - pkg: yandex-media-graphite-to-solomon-sender 

pkgver_ignore_graphite:
  file.managed:
    - name: /etc/yandex-pkgver-ignore.d/yandex-media-graphite-to-solomon-sender
    - contents: |
        yabs-graphite-sender
        config-media-graphite

restart_solomon_sender:
  cmd.wait:
    - name: ubic restart graphite-to-solomon-sender
