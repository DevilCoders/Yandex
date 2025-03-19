{% import 'components/hadoop/macro.sls' as m with context %}
{% set roles = m.roles() %}

{% set packages = {
    'masternode': {
        'zookeeper': m.version('zookeeper'),
        'zookeeper-server': m.version('zookeeper')
     }
} %}

{{ m.pkg_present('zookeeper', packages) }}
