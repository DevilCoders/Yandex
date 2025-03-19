{%- set test_mode = salt['grains.get']('overrides:test_mode', False) %}

{% if test_mode %}
include:
  - nginx
  - tests.automated
  - tests.billing
  - tests.compute
  - tests.identity
  - tests.sqs
  - tests.healthcheck
  - tests.api-gateway
  - tests.api-adapter
  - tests.bootstrap

{%- set nginx_configs = ['yc-images.conf', 'yc-bs-configs-store.conf'] %}
{%- include 'nginx/install_configs.sls' %}
{% endif %}
