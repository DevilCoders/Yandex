{% import 'components/hadoop/macro.sls' as m with context %}

{% set packages = {
    'masternode': {
        'hive-metastore': 'any',
        'hive-server2': 'any',
        'hive-webhcat-server': 'any'
    }
} %}

hive_packages:
    pkg.installed:
        - refresh: False
        - retry:
            attempts: 5
            until: True
            interval: 60
            splay: 10
        - pkgs:
            - hive
            - hive-hcatalog
            - hive-jdbc
            - hive-webhcat

# This hack will only download packages for hive services
# For actual install see servises.sls after metastore-init
{% if salt['ydputils.is_presetup']() %}
{{ m.pkg_present('hive-services', packages) }}
{% endif %}
