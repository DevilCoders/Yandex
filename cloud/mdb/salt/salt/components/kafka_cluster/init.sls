include:
{% if salt.dbaas.is_compute() %}
    - .compute
{% elif salt.dbaas.is_porto() %}
    - .porto
{% elif salt.dbaas.is_aws() %}
    - .aws
{% endif %}
    - .common
