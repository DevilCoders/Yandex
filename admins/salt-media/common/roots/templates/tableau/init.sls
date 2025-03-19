{%- if not salt['file.file_exists']('/opt/tableau_do_not_delete_me.txt') %}

include:
  - .common_packages
  - .clickhouse_driver
  - .postgresql-driver
  - .setup

create-installation-lock-file:
  file.managed:
    - name: /opt/tableau_do_not_delete_me.txt

{% endif %}
