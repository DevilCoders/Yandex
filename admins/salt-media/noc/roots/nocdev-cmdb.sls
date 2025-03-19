include:
  - units.juggler-checks.common
  - units.nginx_conf
  - templates.certificates

{% for file in [
  "conf.d/request_id.conf",
  "sites-enabled/00-cmdb.conf",
] %}
/etc/nginx/{{ file }}:
  file.managed:
    - source: salt://files/nocdev-cmdb/etc/nginx/{{ file }}
    - template: jinja
    - makedirs: True
{% endfor %}

cmdb:
  service.running:
    - enable: True
    - watch:
        - file: /etc/default/cmdb
        - file: /etc/cmdb/config.yaml

/etc/default/cmdb:
  file.managed:
    - user: root
    - group: root
    - mode: 0640
    - template: jinja
    - contents: |
        PGPASSWORD={{ pillar["cmdb_pgpassword"] }}
        TVM_SECRET={{ pillar["cmdb_tvm_secret"] }}

/etc/cmdb/config.yaml:
  file.serialize:
    - dataset_pillar: cmdb_config
    - formatter: yaml
    - user: cmdb
    - group: cmdb
    - mode: 644
