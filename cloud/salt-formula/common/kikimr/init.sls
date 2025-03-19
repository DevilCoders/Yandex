{% set hostname = grains['nodename'] %}
{% set base_role = grains['cluster_map']['hosts'][hostname]['base_role'] %}
{% set kikimr_config = salt['grains.get']('cluster_map:hosts:%s:kikimr' % hostname, {}) %}
{% set environment = grains['cluster_map']['environment'] %}
{% set stand_name = grains['cluster_map']['stand_name'] %}
{% set kikimr_prefix = pillar.get('kikimr_prefix', '') %}

{% set use_config_packages = False %}
{# prod/pre-prod/testing kept as env's. See https://st.yandex-team.ru/CLOUD-23508 for details #}
{% if environment in ["hw-ci", "testing", "pre-prod", "prod"] or stand_name == "ext-testing" %}
  {% set use_config_packages = True %}
{% endif %}

{% set subdomains = kikimr_config.get("tenant", []) if kikimr_config else [] %}
# Need "global" for e2e on OCT SVM
{% set kikimr_cluster = salt['grains.get']('cluster_map:hosts:%s:kikimr:cluster_id' % hostname, 'global') %}
{% if is_nbs is defined %}
  {% set kikimr_cluster = kikimr_config["nbs_cluster_id"] %}
{% endif %}

{% set base_path = "/Berkanavt/kikimr" %}
{% set bin_path = "/Berkanavt/kikimr/bin" %}
{% set mailbox_path = "/etc/kikimr/clusters" %}

{% set snapshot_database = 'snapshot' %}
{% set iam_database = 'iam' %}
{% set compute_database = 'ycloud' %}
{% set billing_database = 'billing' %}
{% set scheduler_database = 'ycloud' %}
{% set loadbalancer_database = 'loadbalancer' %}
{% set e2e_database = 'ycloud' %}
{% set nbs_database = 'NBS' %}
{% set solomon_database = 'solomon' %}
{% set sqs_database = 'SQS' %}
{% set microcosm_database = 'SQS' %}
{% set s3proxy_database = 's3' %}

{% if base_role == 'cloudvm' %}
  {% set billing_database = 'ycloud' %}
  {% set compute_database = 'ycloud' %}
  {% set snapshot_database = 'ycloud' %}
  {% set iam_database = 'ycloud' %}
  {% set scheduler_database = 'ycloud' %}
  {% set loadbalancer_database = 'ycloud' %}
  {% set solomon_database = 'ycloud' %}
  {% set sqs_database = 'ycloud' %}
{% endif %}

{% set storage_types = [] %}
{% if kikimr_cluster is not none %}
  {% for database in grains['cluster_map']['kikimr']['clusters'][kikimr_cluster]['databases'].itervalues() %}
    {% for storage_type in database.storage %}
      {% do storage_types.append(storage_type) %}
    {% endfor %}
  {% endfor %}
{% endif %}
{% set need_encryption = 'ssdencrypted' in storage_types or 'rotencrypted' in storage_types %}

{% set ydb_domain = kikimr_prefix ~ kikimr_cluster %}

{% set proto_prefix = "http" %}
{% if environment in ["dev", "hw-ci", "testing"] or base_role == 'cloudvm' %}
{% set proto_prefix = "https" %}
{% endif %}
