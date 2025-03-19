windows-cluster-req:
    test.nop:
        - require:
            - test: windows-firewall-ready
            
windows-cluster-ready:
    test.nop

{% set hostname = salt['mdb_windows.get_fqdn']() %}
{% if not salt['pillar.get']('replica') and not salt['mdb_windows.is_witness'](hostname) %}
os_clustered:
    mdb_windows.os_clustered:
        - name: {{ salt['pillar.get']('data:dbaas:cluster_id', 'default_cluster_name')|yaml_encode }}
        - require:
            - test: windows-cluster-req
        - require_in:
            - test: windows-cluster-ready

{% for host in salt['pillar.get']('data:dbaas:shard_hosts', {}) %}
node_present_{{ host }}:
    mdb_windows.node_present:
        - name: {{ host }}
        - timeout: 600
        - interval: 10
        - require: 
            - test: windows-cluster-req
            - mdb_windows: os_clustered
        - require_in:
            - mdb_windows: wait_current_node_joined
            - test: windows-cluster-ready
{% endfor %}
{% endif %}

wait_current_node_joined:
    mdb_windows.wait_node_joined:
        - name: {{ salt['pillar.get']('data:dbaas:fqdn', salt['mdb_windows.get_fqdn']()) }}
        - timeout: 600
        - interval: 10
        - require_in:
            - test: windows-cluster-ready
