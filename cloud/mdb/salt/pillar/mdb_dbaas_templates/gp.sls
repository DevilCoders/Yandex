{% from "map.jinja" import gp_versions_package_map with context %}
{% from "mdb_dbaas_templates/init.sls" import env with context %}

{% set vtype = salt['grains.get']('test:vtype', 'porto') %}
{% set major_version = '6.17' %}
data:
    runlist:
{% if vtype == 'compute' %}
        - components.firewall
        - components.dbaas-compute.apparmor
        - components.dbaas-compute.scripts
        - components.dbaas-compute.network
        - components.cloud-init
        - components.linux-kernel
{% endif %}
        - components.greenplum
        - components.monrun2
    use_monrun: false
    monrun:
        disable: true
    versions:
      greenplum:
        edition: 'default'
        major_version: {{ major_version }}
        minor_version: {{ gp_versions_package_map[(env, major_version)]['minor_version'] | tojson }}
        package_version: {{ gp_versions_package_map[(env, major_version)]['package_version'] }}
    pxf:
        pkg_version: 5.16.2-7-yandex.1063.b7e35ae4
    pam_limits:
        nofile: 524288
        nproc: 131072
        core: 10737418240
