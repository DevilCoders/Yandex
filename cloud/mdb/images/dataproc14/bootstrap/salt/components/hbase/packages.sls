{% import 'components/hadoop/macro.sls' as m with context %}
{% set services = salt['pillar.get']('data:services') %}

{% set version = m.version('hbase') %}
{% set packages = {
    'masternode': {
        'hbase': version
    },
    'datanode': {
        'hbase': version
    },
    'computenode': {
        'hbase': version
    }
}%}

{% if 'hive' in services %}
{% do packages['masternode'].update({'hive-hbase': version}) %}
{% endif %}


{{ m.pkg_present('hbase', packages) }}

{% if salt['ydputils.is_presetup']() %}
{% set download_packages = {
    'masternode': {
        'hbase-master': version,
        'hbase-rest': version,
        'hbase-thrift': version,
        'hbase-regionserver': version
    }
}%}
{{ m.pkg_present('hbase-services', download_packages) }}
{% endif %}
