{% if salt['pillar.get']('data:dbaas_compute:vdb_setup', True) %}
{%     set data_mount_point = salt.dbaas.data_mount_point() %}
{%     set large_disk_mkfs_options = '-i 65536' %}
mount-data-directory:
    cmd.run:
        - name: /usr/local/sbin/mount_data_directory.sh "{{ data_mount_point }}" "{{ large_disk_mkfs_options }}"
        - unless: grep -q "{{ data_mount_point }}" /proc/mounts
        - require:
            - file: /usr/local/sbin/mount_data_directory.sh
        - order: 1
        - failhard: True

set-reserved-blocks:
    cmd.run:
        - name: tune2fs -r 8192 $(grep "{{ data_mount_point }}" /proc/mounts | awk '{print $1};')
        - unless: test "$(dumpe2fs -h $(grep "{{ data_mount_point }}" /proc/mounts | awk '{print $1};') 2>/dev/null | grep 'Reserved block count:' | awk '{print $NF};')" = "8192"
        - require:
            - cmd: mount-data-directory
{% endif %}
