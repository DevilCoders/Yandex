/etc/sysctl.d:
    file.directory:
        - user: root
        - group: root
        - mode: 755

/etc/sysctl.d/mongodb.conf:
    file.managed:
        - template: jinja
        - source: salt://{{slspath}}/conf/sysctl-mongodb.conf
        - mode: 644
    cmd.wait:
        - name: sysctl -p /etc/sysctl.d/mongodb.conf
        - watch:
            - file: /etc/sysctl.d/mongodb.conf

{% if salt.grains.get('virtual', 'physical') != 'lxc' and salt.grains.get('virtual_subtype', None) != 'Docker' %}
disable_THP:
    cmd.run:
        - name: 'echo never > /sys/kernel/mm/transparent_hugepage/enabled'
        - unless:
            - "fgrep -q '[never]' /sys/kernel/mm/transparent_hugepage/enabled"

disable_THP_defrag:
    cmd.run:
        - name: 'echo never > /sys/kernel/mm/transparent_hugepage/defrag'
        - unless:
            - "fgrep '[never]' /sys/kernel/mm/transparent_hugepage/defrag"

vm.nr_hugepages:
    sysctl.present:
        - value: {{ salt.pillar.get('data:sysctl:vm.nr_hugepages', '4500') }}
        - config: /etc/sysctl.d/mongodb-hugepages.conf
        - unless:
            - "fgrep {{ salt.pillar.get('data:sysctl:vm.nr_hugepages', '4500') }} /proc/sys/vm/nr_hugepages"
{% endif %}
