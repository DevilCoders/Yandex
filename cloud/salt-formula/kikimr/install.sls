{% set hostname = grains['nodename'] %}
{% set host_roles = grains['cluster_map']['hosts'][hostname]['roles'] %}

{% if 'kikimr' in host_roles %}
kikimr_packages:
  yc_pkg.installed:
    - pkgs:
      - yc-kikimr-systemd
      - yandex-search-kikimr-kikimr-bin
      - yandex-search-kikimr-yql-udfs-kikimr
      - yandex-search-kikimr-python-lib
    - disable_update: True

/etc/security/limits.d/kikimr.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/limits_kikimr.conf
    - require:
      - yc_pkg: kikimr_packages

  {% from slspath+"/monitoring.yaml" import monitoring %}
  {% include "common/deploy_mon_scripts.sls" %}
{% endif %}
