{% set cgroup = grains['conductor']['group'] %}
{% set cgroups = grains['conductor']['groups'] %}

#ELLIPTICS:S3CLEANUP
{% if 's3cleanup' in cgroup %}

include:
  - units.s3.base_config
  - units.s3.pkgs
  - units.s3.background
  - units.s3.billing
  - units.s3.cleanup
  - units.s3.maintain

#ELLIPTICS:PROXY
{% elif 'elliptics-proxy' in cgroups %}

include:
  - units.s3.base_config
  - units.s3.pkgs
  # 'ppb' is 'Proxy Private'
  - units.s3.pp
  - units.s3.quota_control

{% elif 'elliptics-test-proxies' in cgroups %}

include:
  - units.s3.base_config
  - units.s3.pkgs
  - units.s3.pp
  - units.s3.billing
  - units.s3.background
  - units.s3.quota_control

#ELLIPTICS:CLOUD
{% elif cgroup == 'elliptics-test-cloud' %}

include:
  - units.s3.base_config
  - units.s3.pkgs
  - units.s3.cleanup
  - units.s3.maintain

{% else %}

    test.fail_without_changes

{% endif %}

