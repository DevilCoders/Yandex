{% from slspath + "/map.jinja" import galera with context %}
include:
  - .services

{% set fqdn = grains['fqdn'] %}

{% if galera.service == 'garbd' %}
/var/log/garbd:
  file.directory:
    - user: nobody
    - dir_mode: 755

garbd_config:
  file.managed:
    - name: /etc/default/garbd
    - source: salt://templates/galera/files/garbd.conf
    - template: jinja
    - makedirs: True
    - context:
        cluster_name: {{ grains['conductor']['group'] }}
        slsname: {{ slspath }}
        config: {{ galera }}
    - require:
      - file: /var/log/garbd
    - watch_in:
      - service: {{ galera.service }}
{% else %}
cluster_config:
  file.managed:
    - name: /etc/mysql/conf.d/cluster.cnf
    - source: salt://templates/galera/files/cluster.cnf
    - template: jinja
    - makedirs: True
    - context:
        cluster_name: {{ grains['conductor']['group'] }}
        slsname: {{ slspath }}
        cluster_addr: {{ galera.cluster_nodes }}
        config: {{ galera }}
        me: {{ fqdn }}
{% endif %}
