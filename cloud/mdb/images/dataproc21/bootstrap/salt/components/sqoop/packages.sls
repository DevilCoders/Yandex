{% import 'components/hadoop/macro.sls' as m with context %}
{% set roles = m.roles() %}

{% set version = m.version('sqoop') %}
{% set packages = {
    'masternode': {
        'sqoop': version,
    },
    'datanode': {
       'sqoop': version,
    },
    'computenode': {
       'sqoop': version,
    }
} %}

{{ m.pkg_present('sqoop', packages) }}
