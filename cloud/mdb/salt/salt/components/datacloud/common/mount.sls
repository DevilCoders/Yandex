{% if salt['pillar.get']('data:dbaas_aws:vdb_setup', True) %}
{%   set data_mount_point = salt.dbaas.data_mount_point() %}
{%   set data_disk = salt.dbaas.data_disk() %}

{% if salt['pillar.get']('data:encryption:enabled', False) %}
{%   set do_encrypt = 'yes' %}
{% else %}
{%   set do_encrypt = 'no' %}
{% endif %}

mount-data-directory:
    cmd.run:
        - name: /usr/local/yandex/mount_data_directory.sh "{{ data_mount_point }}" "-i 65536" {{ data_disk.device.name }} {{ data_disk.partition.name }} {{ do_encrypt }}
        - unless: grep -q "{{ data_mount_point }}" /proc/mounts
        - require:
            - file: /usr/local/yandex/mount_data_directory.sh
        - order: 1
        - failhard: True

set-reserved-blocks:
    cmd.run:
        - name: tune2fs -r 8192 $(grep "{{ data_mount_point }}" /proc/mounts | awk '{print $1};')
        - unless: test "$(dumpe2fs -h $(grep "{{ data_mount_point }}" /proc/mounts | awk '{print $1};') 2>/dev/null | grep 'Reserved block count:' | awk '{print $NF};')" = "8192"
        - require:
            - cmd: mount-data-directory
{% endif %}
