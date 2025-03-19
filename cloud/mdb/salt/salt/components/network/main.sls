
{% if salt['grains.get']('virtual') == 'lxc' and (salt['pillar.get']('data:network:l3_slb:virtual_ipv6') or salt['pillar.get']('data:network:eth0-mtu')) %}
/etc/network/interfaces:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/interfaces
        - mode: 644
{% endif %}

