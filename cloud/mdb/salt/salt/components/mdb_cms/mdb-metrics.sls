{% from "components/mdb-metrics/lib.sls" import deploy_configs with context %}
{% from "components/mdb-metrics/lib.sls" import yasmgetter_config with context %}
include:
    - components.mdb-metrics

{{ deploy_configs('mdb_cms') }}

{{ yasmgetter_config('mdb_cms-yasmagent-instance-getter-config') }}
