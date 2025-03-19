{% set fqdn = grains['fqdn'] %}

# SEE https://st.yandex-team.ru/MDS-7301 / https://st.yandex-team.ru/MDS-7302 for detail.
# MUST be deprecated in future!

{% if fqdn in pillar['new_decaps_enabled'] %}

/etc/netconfig.d/97-new-decaps.py:
  file.managed:
    - source: salt://units/new_decaps/files/97-new-decaps.py
    - user: root
    - group: root
    - mode: 755

{% endif %}

