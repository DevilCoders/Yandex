{% set data_disk = salt.dbaas.data_disk() %}

grow-partition:
    cmd.run:
{% if salt.pillar.get('data:encryption:enabled', False) %}
        - name: '/sbin/cryptsetup resize {{ data_disk.device.name.split('/')[-1] }}'
{% else %}
        - name: '/usr/bin/growpart {{ data_disk.device.name }} 1'
        # The exit status is 1 if the partition could not be grown
        # due to lack of available space
        #
        # growpart /dev/nvme1n1 1
        #    NOCHANGE: partition 1 is size 83883999. it cannot be grown
        - success_retcodes: [ 1 ]
{% endif %}

resize2fs:
    cmd.run:
        - name: '/sbin/resize2fs {{ data_disk.partition.name }}'
        - require:
              - cmd: grow-partition
