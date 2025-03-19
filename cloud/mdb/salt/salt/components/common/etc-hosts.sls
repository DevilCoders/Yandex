{% if salt.dbaas.is_porto() and salt.mdb_network.is_in_user_project_id() %}
{%     set self_address = {'value': salt.mdb_network.ip6_porto_user_addr()} %}
{% else %}
{%     set self_address = {'value': None} %}
{%     for interface in salt['grains.get']('ip_interfaces', [])|sort %}
{%         if interface.startswith('eth') %}
{%             for addr in salt['grains.get']('ip_interfaces:' + interface, []) %}
{%                 if not addr.startswith('fe80') and not addr.startswith('::') and not self_address['value'] %}
{%                     do self_address.update({'value': addr}) %}
{%                 endif %}
{%             endfor %}
{%         endif %}
{%     endfor %}
{% endif %}

fqdn-in-etc-hosts:
    host.present:
        - name: {{ salt['grains.get']('id') }}
{% if self_address['value'] %}
        - ip: {{ self_address['value'] | json }}
{% else %}
        - ip: {{ salt['grains.get']('fqdn_ip6') | json }}
{% endif %}
        - clean: True

{% for ip, hostnames in salt['pillar.get']('data:etc_hosts', {}).items() %}
{{ ip }}-in-etc-hosts:
    host.only:
        - name: {{ ip }}
        - hostnames: {{ hostnames }}
{% endfor %}
