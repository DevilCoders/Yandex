{% import 'components/hadoop/macro.sls' as m with context %}
{% set roles = m.roles() %}

{% set packages = {
    'masternode': {
        'hadoop-mapreduce-historyserver': m.version('hadoop') 
    }
} %}

# This hack will only download packages for hdfs services
# For actual install see services.sls
{% if salt['ydputils.is_presetup']() %}
{{ m.pkg_present('mapred', packages) }}
{% endif %}
