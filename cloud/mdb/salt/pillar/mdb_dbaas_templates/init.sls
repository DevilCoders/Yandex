{% set hostname = salt.grains.get('id') %}
{% set hostname_prefix = hostname.split('-')[0] %}
{% set template_id = salt.grains.get('test:suite', hostname_prefix) %}
{% set vtype = salt.grains.get('test:vtype', 'porto') %}
{% set env = 'prod' %}
mine_functions:
    grains.item:
        - id
        - role
        - dbaas
        - ya
        - pg
        - virtual

data:
    running_on_template_machine: True
    cloud_type: template_cloud
    region_id: template_region
    dist:
        bionic:
            secure: True
        pgdg:
            absent: True
    sysctl:
        vm.nr_hugepages: 0
    monrun2: True
    ship_logs: True
    cauth_use: False
    dbaas:
        fqdn: template
        cluster_name: template
        cluster_id: template
        subcluster_name: subcluster_name_template
        subcluster_id: subcluster_id_template
        region: template
        cloud_provider: {{ vtype }}
        cluster_type: {{ template_id }}
        vtype: {{ vtype }}
        cloud:
            cloud_ext_id: template_cloud
        folder:
            folder_ext_id: folder_ext_id_template
    prometheus:
        address: prometheus_address_template
    s3:
        endpoint: template_endpoint
        access_key_id: template_access_key_id
        access_secret_key: template_access_secret_key
    s3_bucket: template_bucket
    pushclient:
        start_service: False
    billing:
        topic: test
        ident: test
        tvm_client_id: 1
        tvm_server_id: 1
        tvm_secret: 1
        ship_logs: False
    selfdns-api:
        token: dummy
{% if vtype == 'porto' %}
        plugins-enabled:
            - default
{% endif %}
    dbaas_compute:
        vdb_setup: False
    dbaas_aws:
        vdb_setup: False
    vector:
        address: template
{% if vtype == 'aws' %}
    access:
        yandex_nets:
            v4:
                - "0.0.0.0/0"
            v6:
                - "::/0"
{% endif %}

include:
    - envs.{{ env }}
    - mdb_dbaas_templates.{{ template_id }}
