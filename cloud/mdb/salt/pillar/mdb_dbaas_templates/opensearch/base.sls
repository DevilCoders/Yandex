{% set vtype = salt['grains.get']('test:vtype', 'porto') %}
data:
    use_mdbsecrets: true
    runlist:
{% if vtype == 'compute' %}
        - components.firewall
        - components.dbaas-compute.scripts
        - components.dbaas-compute.network
        - components.cloud-init
        - components.linux-kernel
{% elif vtype == 'porto' %}
        - components.dbaas-porto
{% endif %}
        - components.opensearch
    opensearch:
        users:
            mdb_admin:
                name: mdb_admin
                password: password
        license:
            enabled: False
        cluster_name: template_cluster
        is_data:
            True
        is_master:
            False
    use_yasmagent: False
