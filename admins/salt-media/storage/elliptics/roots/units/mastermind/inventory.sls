/etc/ubic/service/mastermind-inventory.json:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/ubic/service/mastermind-inventory.json

inventory-status:
  monrun.present:
    - command: RESULT=$(timeout 30 mastermind ping inventory 2>&1); [[ $RESULT == "0;Ok" ]] && echo '0;OK' || (echo "2;"; echo "${RESULT}" |tail -n1 ) | xargs
    - execution_interval: 300
    - execution_timeout: 200
    - type: mastermind

# MDS-19343
{% set federation = pillar.get('mds_federation', None) %}
{% if federation == 1 and grains['yandex-environment'] != "testing" %}
/var/tmp/inventory:
  yafile.managed:
    - source: salt://{{ slspath }}/files/var/tmp/inventory
    - template: jinja
{%- endif %}

# MDS-19440
{% set federation = pillar.get('mds_federation', None) %}
{% if federation == 3 and grains['yandex-environment'] == "testing" %}
/var/tmp/inventory:
  yafile.managed:
    - source: salt://{{ slspath }}/files/var/tmp/inventory-federation-3
    - template: jinja
{%- endif %}
