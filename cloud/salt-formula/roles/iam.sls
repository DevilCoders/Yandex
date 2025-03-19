{% set environment = grains['cluster_map']['environment'] %}

include:
  - identity
  - roles.access-service  # CLOUD-22080
  - roles.resource-manager  # CLOUD-22080
{% if environment in ("prod", "pre-prod", "testing") %}
  - iam-takeout-agent
  - resource-manager-takeout-agent
{% endif %}
