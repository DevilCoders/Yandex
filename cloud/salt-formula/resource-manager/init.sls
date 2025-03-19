{% set hostname = grains['nodename'] %}
{% set base_role = grains['cluster_map']['hosts'][hostname]['base_role'] %}
{% set environment = grains['cluster_map']['environment'] %}

{% set ids_filename = salt['file.normpath'](slspath + '/../common/endpoint-ids/files/ids-{}.yaml'.format(environment)) %}
{% import_yaml ids_filename as ids %}

{% set yrm_id = {'value': 'undefined'} %}
{% for id in ids %}
  {% if id.service == "yrm" %}
    {% if yrm_id.update({'value': id.id}) %} {% endif %}
  {% endif %}
{% endfor %}

{# FIXME(CLOUD-19956): hw-labs do not have api adapter, can't install resource-manager #}
{% if environment != 'dev' or base_role == 'cloudvm' %}

yc-resource-manager:
  yc_pkg.installed:
    - pkgs:
      - yc-resource-manager
  service.running:
    - enable: True
    - name: yc-resource-manager
    - watch:
      - yc_pkg: yc-resource-manager
      - file: /etc/yc/resource-manager/config.env
      - file: /etc/yc/resource-manager/config.yaml
      - file: /etc/yc/resource-manager/log4j2.yaml
      - file: /etc/yc/resource-manager/server.pem
      - file: /var/lib/yc/resource-manager/system_account.json

/etc/yc/resource-manager/config.env:
  file.managed:
    - source: salt://{{ slspath }}/files/config.env
    - makedirs: True
    - template: jinja
    - user: root
    - group: root
    - require:
      - yc_pkg: yc-resource-manager

/etc/yc/resource-manager/config.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/config.yaml
    - template: jinja
    - makedirs: True
    - user: root
    - group: root
    - require:
      - yc_pkg: yc-resource-manager
    - defaults:
      yrm_id: {{ yrm_id.value }}

/etc/yc/resource-manager/log4j2.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/log4j2.yaml
    - template: jinja
    - makedirs: True
    - user: root
    - group: root
    - require:
      - yc_pkg: yc-resource-manager

/etc/yc/resource-manager/server.pem:
  file.managed:
    - source: salt://{{ slspath }}/files/stub-certificate.pem
    - makedirs: True
    - replace: False
    - user: yc-resource-manager
    - group: yc-resource-manager
    - mode: 0400

/var/log/yc/resource-manager:
  file.directory:
    - user: yc-resource-manager
    - group: yc-resource-manager
    - mode: 755

/var/lib/yc/resource-manager/system_account.json:
  file.managed:
    - source: salt://{{ slspath }}/system_account.json
    - user: yc-resource-manager
    - group: yc-resource-manager
    - mode: 0400
    - replace: False

{% endif %}
