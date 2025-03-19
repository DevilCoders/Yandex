{% from "components/postgres/pg.jinja" import pg with context %}

/etc/sysctl.d:
    file.directory:
        - user: root
        - group: root
        - mode: 755

/etc/sysctl.d/postgres.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/sysctl-postgres.conf
        - mode: 644
    cmd.wait:
        - name: sysctl -p /etc/sysctl.d/postgres.conf
        - watch:
            - file: /etc/sysctl.d/postgres.conf

{% if salt['grains.get']('virtual', 'physical') != 'lxc' and salt['grains.get']('virtual_subtype', None) != 'Docker' %}
vm.nr_hugepages:
    sysctl.present:
        - value: {{ salt['pillar.get']('data:sysctl:vm.nr_hugepages', '4500') }}
        - config: /etc/sysctl.d/postgres-hugepages.conf
        - unless:
            - "grep {{ salt['pillar.get']('data:sysctl:vm.nr_hugepages', '4500') }} /proc/sys/vm/nr_hugepages"
{% endif %}
