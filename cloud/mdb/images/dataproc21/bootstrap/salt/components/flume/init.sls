{% import 'components/hadoop/macro.sls' as m with context %}

{% set roles = m.roles() %}

{% set version = 'any' %}
{% set packages = {
    'masternode': {
        'flume': version,
        'flume-agent': version
    },
    'datanode': {
        'flume': version,
        'flume-agent': version
    },
    'computenode': {
        'flume': version,
        'flume-agent': version
     }
}%}

{{ m.pkg_present('flume', packages) }}
