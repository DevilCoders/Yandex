{% import 'components/hadoop/macro.sls' as m with context %}

{% set version = 'any' %}
{% set packages = {
    'masternode': {
        'postgresql-11': version
    }
} %}

{{ m.pkg_present('postgres', packages) }}
