{%- set scripts = ['clear_all_ipbroker_allocation.sh', 'missing_backends.sh', 'disk_handle_phy.sh', 'host_disks.sh', 'disk_slot_num.sh'] %}
{%- for s in scripts %}

/usr/local/bin/{{ s }}:
  yafile.managed:
    - source: salt://files/storage/{{ s }}
    - mode: 755
    - user: root
    - group: root
    - makedirs: True
{%- endfor %}
