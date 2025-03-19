{% import 'components/hadoop/macro.sls' as m with context %}
{% set roles = m.roles() %}

{% set version = m.version('hadoop') %}
{% set packages = {
    'masternode': {
        'hadoop-hdfs-namenode': version,
        'hadoop-hdfs-secondarynamenode': version
    },
    'datanode': {
        'hadoop-hdfs-datanode': version
    }
} %}


hdfs_packages:
    pkg.installed:
        - refresh: False
        - retry:
            attempts: 5
            until: True
            interval: 60
            splay: 10
        - pkgs:
            - hadoop-hdfs

# This hack will only download packages for hdfs services
# For actual install see services.sls
{% if salt['ydputils.is_presetup']() %}
{{ m.pkg_present('hdfs-services', packages) }}
{% endif %}
