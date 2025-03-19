{% import 'components/hadoop/macro.sls' as m with context %}
{% set roles = m.roles() %}

{% set version = m.version('zeppelin') %}
{% set packages = {
    'masternode': {
        'zeppelin': version
    },
    'datanode': {},
    'computenode': {}

} %}

{{ m.pkg_present('zeppelin', packages) }}
