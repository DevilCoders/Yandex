{% import 'components/hadoop/macro.sls' as m with context %}
{% set roles = m.roles() %}

{% set version = m.version('hadoop') %}
{% set packages = {
    'masternode': {
        'hadoop-yarn-resourcemanager': version,
        'hadoop-yarn-timelineserver': version,
    },
    'datanode': {
        'hadoop-yarn-nodemanager': version
    },
    'computenode': {
        'hadoop-yarn-nodemanager': version
    }
} %}

# This hack will only download packages for yarn services
# For actual install see services.sls
{% if salt['ydputils.is_presetup']() %}
{{ m.pkg_present('yarn', packages) }}
{% endif %}
