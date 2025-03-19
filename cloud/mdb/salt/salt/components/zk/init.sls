include:
{% if salt.dbaas.is_aws() %}
    - .init-aws
{% else %}
    - .init-yandex
{% endif %}
