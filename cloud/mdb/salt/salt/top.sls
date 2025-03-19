{% set common = 'common' %}
{% set common = 'common-windows' if salt['grains.get']('os') == 'Windows' else common %}
{% set is_aws = salt.get('dbaas.is_aws') %}
{% set common = 'datacloud.common' if is_aws and is_aws() else common %}

base:
    '*':
        - common

dev:
    'I@yandex:environment:dev':
        - match: compound
        - components.{{common}}
        - components.runner

qa:
    'I@yandex:environment:qa':
        - match: compound
        - components.{{common}}
        - components.runner

load:
    'I@yandex:environment:load':
        - match: compound
        - components.{{common}}
        - components.runner

prod:
    'I@yandex:environment:prod':
        - match: compound
        - components.{{common}}
        - components.runner

compute-prod:
    'I@yandex:environment:compute-prod':
        - match: compound
        - components.{{common}}
        - components.runner
