{% set yaenv = grains['yandex-environment'] %}
{% set fqdn = grains['fqdn'] %}

{% if 'kino-back01h.dev.kino.yandex.net' in fqdn %}
{% set type = 'bo' %}
{% elif 'back' in fqdn %}
{% set type = 'api' %}
{% elif 'bko' in fqdn %}
{% set type = 'bo' %}
{% else %}
{% set type = 'bo' %}
{% endif %}

{% if 'tv' in fqdn %}
{% set prj = 'tv' %}
{% elif 'kino' in fqdn %}
{% set prj = 'kino' %}
{% else %}
{% set prj = '' %}
{% endif %}

{% set secret_part = [prj, type, yaenv]|join('_') %}

{% if prj == 'tv' %}
{% set secret = 'sec-01d60gn2xjpm270gzgysktwq1c' if yaenv in ['production', 'prestable'] else 'sec-01d31c86f936fmvp3fv6bhhy04' %}
{% elif prj == 'kino' %}
{% set secret = 'sec-01d31mcg0p6r8erqbch3zbggeh' if yaenv in ['production', 'prestable'] else 'sec-01d60j7jpjke78wg8qh0km82h5' %}
{% else %}
{% set secret = '' %}
{% endif %}
{%set yandex_jdk8_key = 'yandex_jdk8_production' if  yaenv in ['production', 'prestable']  else 'yandex_jdk8_testing'%}
secrets:
  external_config: {{salt.yav.get(secret)[secret_part]|json}}
{% if prj == 'tv' %}
  yandex_jdk8_version: {{salt.yav.get(secret)[yandex_jdk8_key]|json}}
{% endif %}

