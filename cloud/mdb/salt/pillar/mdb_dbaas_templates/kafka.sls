{% set vtype = salt['grains.get']('test:vtype', 'porto') %}
{% set salt_version = salt['grains.get']('test:salt_version', '3002.7+ds-1+yandex0') %}
{% set salt_py_version = salt['grains.get']('test:salt_py_version', 3) %}
data:
    runlist:
{% if vtype == 'compute' %}
        - components.firewall
        - components.dbaas-compute.apparmor
        - components.dbaas-compute.apparmor.kafka
        - components.dbaas-compute.scripts
        - components.dbaas-compute.network
        - components.cloud-init
        - components.linux-kernel
{% endif %}
        - components.kafka
    mdb_metrics:
        enabled: True
    use_yasmagent: False
    kafka:
        version: '2.8'
        package_version: '2.8.1-java11'
{% if salt_version %}
    salt_version: {{ salt_version }}
{% endif %}
{% if salt_py_version %}
    salt_py_version: {{ salt_py_version }}
{% endif %}
