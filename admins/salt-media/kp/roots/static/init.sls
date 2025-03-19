{% set is_prod = grains["yandex-environment"] in ["production"] %}

include:
  - templates.nginx
  - templates.disable_rp_filter
  - templates.push-client
  - templates.packages
  - .monrun
  - .nginx
  - .ssh-keys
{% if is_prod %}
  - templates.rsyncd
  - .rsyncd_secrets
  - .antiblock
{% endif %}
